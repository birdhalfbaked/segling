#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

/// @defgroup STORAGE_types STORAGE types
/// @{

typedef enum {
  STORAGE_RESULT_OK = 0,
  STORAGE_RESULT_ERROR_UNKNOWN = -1,
  STORAGE_RESULT_ERROR_LOAD_FAILED = -2,
} storage_result_t;

/// Telemetry / controller-published slots (state.bin).
#define FILE_ID_CONTROLLER_STATE 0
#define FILE_ID_AHRS 1
#define FILE_ID_GPS 2
#define FILE_ID_BAROMETER 3
#define FILE_ID_AHRS_CALIBRATION 8
#define FILE_ID_BAROMETER_CALIBRATION 9
/// Legacy slot id for command mailbox reads/writes (backed by commands.bin).
#define FILE_ID_COMMAND 15

/// Memory map: logical "files" of 128 bytes within state.bin
#define STORAGE_MAX_FILES 16
#define STORAGE_FILE_SIZE 128

/// First byte of FILE_ID_AHRS slot and of commands.bin is a seqlock byte toggled
/// with XOR 1 around payload writes: odd = locked / inconsistent for readers;
/// even = stable snapshot. (Xor avoids uint8 wrap bugs vs monotonic increment.)
/// Payload follows immediately (127 bytes max in that slot/page).
#define STORAGE_SEQLOCK_BYTES 1
#define STORAGE_SLOT_PAYLOAD_BYTES (STORAGE_FILE_SIZE - STORAGE_SEQLOCK_BYTES)

/// Byte length of state mmap (same linear slot layout as before; slot 15 unused).
#define STORAGE_STATE_MAP_BYTES (STORAGE_MAX_FILES * STORAGE_FILE_SIZE)

/// Separate commands.bin holds one command page (same 128 B shape as a slot).
#define STORAGE_COMMAND_MAP_BYTES STORAGE_FILE_SIZE

#define STORAGE_STATE_PATH "/var/run/segling/state.bin"
#define STORAGE_COMMANDS_PATH "/var/run/segling/commands.bin"

/// File identifier within state.bin (not used for commands.bin offsets).
typedef uint8_t file_id_t;

/// Two mmap regions (state.bin + commands.bin). The controller must write the
/// commands page to clear opcodes after handling; use OS permissions so only the
/// interface process can write commands.bin and only read state.bin.
typedef struct {
  uint8_t *state;
  uint8_t *commands;
} storage_t;

/// @}
/// @defgroup STORAGE_methods STORAGE methods
/// @{
storage_result_t storage_init(storage_t *storage);
storage_result_t storage_deinit(storage_t *storage);

/// Writes state.bin for telemetry slots, or commands.bin when file_id is
/// FILE_ID_COMMAND. For FILE_ID_AHRS and FILE_ID_COMMAND the leading seqlock
/// byte is managed here; \p data is the logical payload (opcode at data[0] for
/// commands), STORAGE_SLOT_PAYLOAD_BYTES copied into the mmap tail.
storage_result_t storage_write(storage_t *storage, file_id_t file_id,
                               uint8_t *data);

/// Reads a telemetry slot from state.bin, or FILE_ID_COMMAND page from
/// commands.bin. Seqlocked slots copy payload into \p data with logical layout
/// (commands opcode at data[0]); byte data[STORAGE_FILE_SIZE-1] is zero when
/// unused.
storage_result_t storage_read(storage_t *storage, file_id_t file_id,
                              uint8_t *data);
/// @}
#endif
