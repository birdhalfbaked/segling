package main

import (
	"encoding/json"
	"flag"
	"log"
	"net/http"
	"os"
	"time"

	"segling/interface/internal/storage"

	"github.com/gorilla/websocket"
)

var upgrader = websocket.Upgrader{
	ReadBufferSize:  1024,
	WriteBufferSize: 1024,
	CheckOrigin: func(r *http.Request) bool {
		return true
	},
}

func withCORS(next http.Handler) http.Handler {
	return http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Access-Control-Allow-Origin", "*")
		w.Header().Set("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
		w.Header().Set("Access-Control-Allow-Headers", "Content-Type")
		if r.Method == http.MethodOptions {
			w.WriteHeader(http.StatusNoContent)
			return
		}
		next.ServeHTTP(w, r)
	})
}

func main() {
	addr := flag.String("addr", ":8787", "HTTP listen address")
	statePath := flag.String("state", defaultStatePath(), "path to state.bin (mmap, read-only for server)")
	commandsPath := flag.String("commands", defaultCommandsPath(), "path to commands.bin (mmap, read-write)")
	flag.Parse()

	st, err := storage.Open(*statePath, *commandsPath)
	if err != nil {
		log.Fatalf("mmap storage: %v", err)
	}
	defer st.Close()
	log.Printf("mmap state %q (%d bytes), commands %q (%d bytes)",
		st.StatePath(), storage.TotalSize, st.CommandsPath(), storage.CommandsMapSize)

	mux := http.NewServeMux()
	mux.HandleFunc("/api/v1/health", func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		_ = json.NewEncoder(w).Encode(map[string]string{"status": "ok"})
	})
	mux.HandleFunc("/api/v1/state", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		snap, err := st.ReadSnapshot()
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		_ = json.NewEncoder(w).Encode(snap)
	})
	mux.HandleFunc("/api/v1/sensors", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		snap, err := st.ReadSnapshot()
		if err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		payload := map[string]any{
			"imu": map[string]any{
				"state":       snap.ImuState,
				"stateLabel":  snap.ImuStateLabel,
				"supportsCal": true,
			},
			"magnetometer": map[string]any{
				"state":       snap.MagnetometerState,
				"stateLabel":  snap.MagnetometerLabel,
				"supportsCal": true,
			},
			"headingDeg": snap.HeadingDeg,
			"pitchDeg":   snap.PitchDeg,
			"rollDeg":    snap.RollDeg,
		}
		w.Header().Set("Content-Type", "application/json")
		_ = json.NewEncoder(w).Encode(payload)
	})
	mux.HandleFunc("/api/v1/calibrate/imu", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		if err := st.QueueCalibrateIMU(); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		_ = json.NewEncoder(w).Encode(map[string]string{"queued": "imu"})
	})
	mux.HandleFunc("/api/v1/calibrate/compass", func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodPost {
			http.Error(w, "method not allowed", http.StatusMethodNotAllowed)
			return
		}
		if err := st.QueueCalibrateCompass(); err != nil {
			http.Error(w, err.Error(), http.StatusInternalServerError)
			return
		}
		w.Header().Set("Content-Type", "application/json")
		_ = json.NewEncoder(w).Encode(map[string]string{"queued": "compass"})
	})
	mux.HandleFunc("/api/v1/stream", func(w http.ResponseWriter, r *http.Request) {
		conn, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("websocket upgrade: %v", err)
			return
		}
		defer conn.Close()
		tick := time.NewTicker(50 * time.Millisecond)
		defer tick.Stop()
		for {
			select {
			case <-r.Context().Done():
				return
			case <-tick.C:
				snap, err := st.ReadSnapshot()
				if err != nil {
					log.Printf("read snapshot: %v", err)
					continue
				}
				if err := conn.WriteJSON(snap); err != nil {
					return
				}
			}
		}
	})

	log.Printf("listening on %s", *addr)
	log.Fatal(http.ListenAndServe(*addr, withCORS(mux)))
}

func defaultStatePath() string {
	if v := os.Getenv("SEGLING_STATE_PATH"); v != "" {
		return v
	}
	if _, err := os.Stat("/var/run/segling/state.bin"); err == nil {
		return "/var/run/segling/state.bin"
	}
	return "state.bin"
}

func defaultCommandsPath() string {
	if v := os.Getenv("SEGLING_COMMANDS_PATH"); v != "" {
		return v
	}
	if _, err := os.Stat("/var/run/segling/commands.bin"); err == nil {
		return "/var/run/segling/commands.bin"
	}
	return "commands.bin"
}
