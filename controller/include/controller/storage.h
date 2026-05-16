#ifndef STORAGE_H
#define STORAGE_H

#include <stdint.h>

#include "scheduling.h"
/// @defgroup STORAGE_types STORAGE types
/// @{

typedef enum {
  STORAGE_RESULT_OK = 0,
  STORAGE_RESULT_ERROR_UNKNOWN = -1,
  STORAGE_RESULT_ERROR_LOAD_FAILED = -2,
} storage_result_t;

/// Memory map: logical "files" of 128 bytes within state.bin
#define STORAGE_MAX_FILES 16
#define STORAGE_FILE_SIZE 128

/// Byte length of state mmap
#define STORAGE_STATE_MAP_BYTES (STORAGE_MAX_FILES * STORAGE_FILE_SIZE)

#define STORAGE_STATE_PATH "/var/run/segling/state.bin"

typedef struct {
  uint64_t epoch;
  uint8_t data[STORAGE_FILE_SIZE - sizeof(uint64_t)];
} storage_file_t;

/// Storage handle
typedef struct {
  uint8_t *state;
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
storage_result_t storage_write(storage_t *storage, slot_id_t slot_id,
                               uint8_t *data);

/// Reads a telemetry slot from state.bin, or FILE_ID_COMMAND page from
/// commands.bin. Seqlocked slots copy payload into \p data with logical layout
/// (commands opcode at data[0]); byte data[STORAGE_FILE_SIZE-1] is zero when
/// unused.
storage_result_t storage_read(storage_t *storage, slot_id_t slot_id,
                              uint8_t *data);
/// @}

/// Commit slot and switch commit pointer for the file with the given slot_id
storage_result_t storage_commit(storage_t *storage, slot_id_t slot_id);
/// @}
#endif
