package storage

import (
	"encoding/binary"
	"errors"
	"math"
)

// AHRS slot (file slot FileIDAHRS / 1): byte 0 is a seqlock (even = stable),
// bytes 1–127 are payload. Decoded layout matches controller ahrs_public_t
// (GCC typical ABI): ahrs_state_t × 2 as int-sized enums at payload offsets 0
// and 4, then IEEE754 binary64 fields LE from payload byte 8.
//
// Not backwards-compatible with the old float32-at-byte-4 layout.
const (
	ahrsPublicMmapMinBytes = 64

	ahrsPublicByteOffsetHeading       = 8
	ahrsPublicByteOffsetPitch         = 16
	ahrsPublicByteOffsetRoll          = 24
	ahrsPublicByteOffsetYaw           = 32
	ahrsPublicByteOffsetRotationRateX = 40
	ahrsPublicByteOffsetRotationRateY = 48
	ahrsPublicByteOffsetRotationRateZ = 56
)

func readLEFloat64(page []byte, off int) (float64, error) {
	end := off + 8
	if end > len(page) {
		return 0, errors.New("ahrs mmap: truncated float64 field")
	}
	u := binary.LittleEndian.Uint64(page[off:end])
	return math.Float64frombits(u), nil
}

// DecodeAHRSPublicV1 decodes the controller AHRS public blob into a Snapshot.
// Despite the name, this tracks the current mmap layout (double precision).
func DecodeAHRSPublicV1(page []byte) (Snapshot, error) {
	if len(page) < ahrsPublicMmapMinBytes {
		return Snapshot{}, errors.New("ahrs mmap: page too short")
	}

	out := Snapshot{
		ImuState:          uint8(binary.LittleEndian.Uint32(page[0:4])),
		MagnetometerState: uint8(binary.LittleEndian.Uint32(page[4:8])),
	}
	var err error
	if out.HeadingDeg, err = readLEFloat64(page, ahrsPublicByteOffsetHeading); err != nil {
		return Snapshot{}, err
	}
	if out.PitchDeg, err = readLEFloat64(page, ahrsPublicByteOffsetPitch); err != nil {
		return Snapshot{}, err
	}
	if out.RollDeg, err = readLEFloat64(page, ahrsPublicByteOffsetRoll); err != nil {
		return Snapshot{}, err
	}
	if out.YawDeg, err = readLEFloat64(page, ahrsPublicByteOffsetYaw); err != nil {
		return Snapshot{}, err
	}
	if out.RotationRateX, err = readLEFloat64(page, ahrsPublicByteOffsetRotationRateX); err != nil {
		return Snapshot{}, err
	}
	if out.RotationRateY, err = readLEFloat64(page, ahrsPublicByteOffsetRotationRateY); err != nil {
		return Snapshot{}, err
	}
	if out.RotationRateZ, err = readLEFloat64(page, ahrsPublicByteOffsetRotationRateZ); err != nil {
		return Snapshot{}, err
	}
	out.ImuStateLabel = stateLabel(out.ImuState)
	out.MagnetometerLabel = stateLabel(out.MagnetometerState)
	return out, nil
}
