## Segling Autopilot (Raspberry Pi)

An autopilot that is coupled to sensors onboard a Raspberry Pi. Will also couple the system to an actuator and an interface accessible via onboard Wi‑Fi on the vessel.

### Current plan

- **UI**: Vue (Vuetify)
- **Backend**: Go
- **Controller / sensor side**: C
- **Integration**: the Go backend interfaces with memory-mapped areas that are updated by the C code

### Repository layout (current)

- `controller/`: C code
- `interface/`: the frontend with Vue, and the backend server with golang for easiness
