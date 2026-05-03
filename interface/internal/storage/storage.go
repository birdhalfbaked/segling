package storage

import (
	"errors"
	"os"
	"sync"
	"sync/atomic"
	"unsafe"

	mmap "github.com/edsrzf/mmap-go"
)

const (
	FileSize  = 128
	MaxFiles  = 16
	TotalSize = FileSize * MaxFiles

	// Commands mailbox is a single file-sized page (commands.bin).
	CommandsMapSize = FileSize

	// SeqlockByte marks locked vs unlocked regions (matches controller storage.h).
	SeqlockByte = 1
	// Payload length after the seqlock prefix within a 128-byte slot/page.
	SeqlockPayloadBytes = FileSize - SeqlockByte

	maxSeqlockSpins = 1 << 22

	// File slots align with controller/include/storage.h for state.bin only.
	FileIDControllerState      = 0
	FileIDAHRS                 = 1
	FileIDGPS                  = 2
	FileIDBarometer            = 3
	FileIDAHRSCalibration      = 8
	FileIDBarometerCalibration = 9
	FileIDCommand              = 15 // logical id for API; mmap offset is always 0 in commands.bin
)

const (
	CmdNone               = 0
	CmdCalibrateIMU       = 1
	CmdCalibrateCompass   = 2
)

// AHRSState names align with controller ahrs_state_t.
const (
	StateInitializing = 0
	StateCalibrating  = 1
	StateActive       = 2
	StateDisabled     = 3
	StateFailed       = 4
)

// Snapshot is the decoded AHRS public page (file slot FileIDAHRS in state.bin).
type Snapshot struct {
	ImuState           uint8   `json:"imuState"`
	MagnetometerState  uint8   `json:"magnetometerState"`
	HeadingDeg         float64 `json:"headingDeg"`
	PitchDeg           float64 `json:"pitchDeg"`
	RollDeg            float64 `json:"rollDeg"`
	YawDeg             float64 `json:"yawDeg"`
	RotationRateX      float64 `json:"rotationRateX"`
	RotationRateY      float64 `json:"rotationRateY"`
	RotationRateZ      float64 `json:"rotationRateZ"`
	ImuStateLabel      string  `json:"imuStateLabel"`
	MagnetometerLabel  string  `json:"magnetometerStateLabel"`
}

func stateLabel(s uint8) string {
	switch s {
	case StateInitializing:
		return "initializing"
	case StateCalibrating:
		return "calibrating"
	case StateActive:
		return "active"
	case StateDisabled:
		return "disabled"
	case StateFailed:
		return "failed"
	default:
		return "unknown"
	}
}

// Store maps state.bin (read-only) and commands.bin (read-write).
type Store struct {
	statePath    string
	commandsPath string
	mu           sync.RWMutex
	state        mmap.MMap
	commands     mmap.MMap
}

func ensureFileSized(path string, minBytes int64, mode os.FileMode) error {
	f, err := os.OpenFile(path, os.O_RDWR|os.O_CREATE, mode)
	if err != nil {
		return err
	}
	st, err := f.Stat()
	if err != nil {
		_ = f.Close()
		return err
	}
	if st.Size() < minBytes {
		if err := f.Truncate(minBytes); err != nil {
			_ = f.Close()
			return err
		}
	}
	return f.Close()
}

// Open maps telemetry statePath read-only and commandsPath read-write (same layout as controller storage).
func Open(statePath, commandsPath string) (*Store, error) {
	if err := ensureFileSized(statePath, TotalSize, 0o644); err != nil {
		return nil, err
	}
	if err := ensureFileSized(commandsPath, CommandsMapSize, 0o644); err != nil {
		return nil, err
	}

	sf, err := os.OpenFile(statePath, os.O_RDONLY, 0)
	if err != nil {
		return nil, err
	}
	stateReg, err := mmap.Map(sf, mmap.RDONLY, 0)
	_ = sf.Close()
	if err != nil {
		return nil, err
	}

	cf, err := os.OpenFile(commandsPath, os.O_RDWR, 0)
	if err != nil {
		_ = stateReg.Unmap()
		return nil, err
	}
	cmdReg, err := mmap.Map(cf, mmap.RDWR, 0)
	_ = cf.Close()
	if err != nil {
		_ = stateReg.Unmap()
		return nil, err
	}

	return &Store{
		statePath:    statePath,
		commandsPath: commandsPath,
		state:        stateReg,
		commands:     cmdReg,
	}, nil
}

func (s *Store) Close() error {
	s.mu.Lock()
	defer s.mu.Unlock()
	var errSt, errCmd error
	if s.state != nil {
		errSt = s.state.Unmap()
		s.state = nil
	}
	if s.commands != nil {
		errCmd = s.commands.Unmap()
		s.commands = nil
	}
	if errSt != nil {
		return errSt
	}
	return errCmd
}

// Path returns state.bin path (preferred log identifier).
func (s *Store) Path() string { return s.statePath }

func (s *Store) StatePath() string    { return s.statePath }
func (s *Store) CommandsPath() string { return s.commandsPath }

// ReadSnapshot decodes the AHRS slot from state.bin using the seqlock prefix.
func (s *Store) ReadSnapshot() (Snapshot, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	if len(s.state) < TotalSize {
		return Snapshot{}, os.ErrInvalid
	}
	off := FileIDAHRS * FileSize
	page := s.state[off : off+FileSize]
	payload, err := readSeqlockedPayload(page)
	if err != nil {
		return Snapshot{}, err
	}
	return DecodeAHRSPublicV1(payload)
}

// QueueCalibrateIMU writes the command page for the controller loop.
func (s *Store) QueueCalibrateIMU() error {
	return s.writeCommand(CmdCalibrateIMU)
}

// QueueCalibrateCompass writes the command page for the controller loop.
func (s *Store) QueueCalibrateCompass() error {
	return s.writeCommand(CmdCalibrateCompass)
}

func (s *Store) writeCommand(cmd byte) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if len(s.commands) < CommandsMapSize {
		return os.ErrInvalid
	}
	page := s.commands[0:CommandsMapSize]
	if err := seqLockCommandsBegin(page); err != nil {
		return err
	}
	// Command file v1: logical opcode at payload[0] → mmap byte 1; bytes 2–3 reserved.
	page[1] = cmd
	page[2] = 0
	page[3] = 0
	if err := seqLockCommandsEnd(page); err != nil {
		return err
	}
	return s.commands.Flush()
}

func readSeqlockedPayload(page []byte) ([]byte, error) {
	if len(page) != FileSize {
		return nil, os.ErrInvalid
	}
	buf := make([]byte, SeqlockPayloadBytes)
	for spin := 0; spin < maxSeqlockSpins; spin++ {
		s1 := page[0]
		if s1&1 != 0 {
			continue
		}
		copy(buf, page[SeqlockByte:])
		s2 := page[0]
		if s1 == s2 && s2&1 == 0 {
			return buf, nil
		}
	}
	return nil, errors.New("storage: seqlock read retries exhausted")
}

func commandsSeqWord(page []byte) (*uint32, error) {
	if len(page) < 4 {
		return nil, os.ErrInvalid
	}
	p := unsafe.Pointer(&page[0])
	if uintptr(p)%4 != 0 {
		return nil, errors.New("storage: commands page not 4-byte aligned for seqlock CAS")
	}
	return (*uint32)(p), nil
}

// Toggle seq byte (low byte of first LE word) using CAS; odd means locked.
func seqLockCommandsBegin(page []byte) error {
	wptr, err := commandsSeqWord(page)
	if err != nil {
		return err
	}
	for spin := 0; spin < maxSeqlockSpins; spin++ {
		w := atomic.LoadUint32(wptr)
		b := byte(w)
		if b&1 != 0 {
			continue
		}
		next := (w & 0xffffff00) | uint32(b^1)
		if atomic.CompareAndSwapUint32(wptr, w, next) {
			return nil
		}
	}
	return errors.New("storage: command seqlock lock retries exhausted")
}

func seqLockCommandsEnd(page []byte) error {
	wptr, err := commandsSeqWord(page)
	if err != nil {
		return err
	}
	for spin := 0; spin < maxSeqlockSpins; spin++ {
		w := atomic.LoadUint32(wptr)
		b := byte(w)
		if b&1 == 0 {
			continue
		}
		next := (w & 0xffffff00) | uint32(b^1)
		if atomic.CompareAndSwapUint32(wptr, w, next) {
			return nil
		}
	}
	return errors.New("storage: command seqlock unlock retries exhausted")
}
