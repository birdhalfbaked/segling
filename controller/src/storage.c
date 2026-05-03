#include "storage.h"
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static _Atomic uint8_t *seq_byte(uint8_t *slot_base) {
  return (_Atomic uint8_t *)slot_base;
}

static void seqlock_write_payload(uint8_t *slot_base, const uint8_t *payload) {
  (void)atomic_fetch_xor_explicit(seq_byte(slot_base), 1U,
                                   memory_order_acq_rel);
  memcpy(slot_base + STORAGE_SEQLOCK_BYTES, payload,
         STORAGE_SLOT_PAYLOAD_BYTES);
  (void)atomic_fetch_xor_explicit(seq_byte(slot_base), 1U,
                                   memory_order_release);
}

static void seqlock_read_payload(const uint8_t *slot_base, uint8_t *payload_out) {
  for (;;) {
    uint8_t s1 =
        atomic_load_explicit(seq_byte((uint8_t *)slot_base), memory_order_acquire);
    if ((s1 & 1U) != 0U) {
      continue;
    }
    memcpy(payload_out, slot_base + STORAGE_SEQLOCK_BYTES,
           STORAGE_SLOT_PAYLOAD_BYTES);
    atomic_thread_fence(memory_order_acquire);
    uint8_t s2 =
        atomic_load_explicit(seq_byte((uint8_t *)slot_base), memory_order_acquire);
    if (s1 == s2 && (s2 & 1U) == 0U) {
      break;
    }
  }
}

static storage_result_t mmap_rw(const char *path, size_t len, uint8_t **out) {
  int fd = open(path, O_RDWR | O_SYNC);
  void *p = MAP_FAILED;
  if (fd == -1) {
    perror(path);
    return STORAGE_RESULT_ERROR_LOAD_FAILED;
  }
  p = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (p == MAP_FAILED) {
    perror("mmap");
    (void)close(fd);
    return STORAGE_RESULT_ERROR_LOAD_FAILED;
  }
  (void)close(fd);
  *out = (uint8_t *)p;
  return STORAGE_RESULT_OK;
}

storage_result_t storage_init(storage_t *storage) {
  storage_result_t result = STORAGE_RESULT_OK;
  if (storage == NULL) {
    return STORAGE_RESULT_ERROR_UNKNOWN;
  }
  storage->state = NULL;
  storage->commands = NULL;

  result = mmap_rw(STORAGE_STATE_PATH, STORAGE_STATE_MAP_BYTES, &storage->state);
  if (result != STORAGE_RESULT_OK) {
    return result;
  }

  result =
      mmap_rw(STORAGE_COMMANDS_PATH, STORAGE_COMMAND_MAP_BYTES, &storage->commands);
  if (result != STORAGE_RESULT_OK) {
    (void)munmap(storage->state, STORAGE_STATE_MAP_BYTES);
    storage->state = NULL;
    return result;
  }

  atomic_store_explicit(
      seq_byte(storage->state + (size_t)FILE_ID_AHRS * STORAGE_FILE_SIZE), 0U,
      memory_order_relaxed);
  atomic_store_explicit(seq_byte(storage->commands), 0U, memory_order_relaxed);

  return STORAGE_RESULT_OK;
}

storage_result_t storage_deinit(storage_t *storage) {
  if (storage == NULL) {
    return STORAGE_RESULT_ERROR_UNKNOWN;
  }
  if (storage->state != NULL) {
    if (munmap(storage->state, STORAGE_STATE_MAP_BYTES) != 0) {
      perror("munmap state");
      return STORAGE_RESULT_ERROR_UNKNOWN;
    }
    storage->state = NULL;
  }
  if (storage->commands != NULL) {
    if (munmap(storage->commands, STORAGE_COMMAND_MAP_BYTES) != 0) {
      perror("munmap commands");
      return STORAGE_RESULT_ERROR_UNKNOWN;
    }
    storage->commands = NULL;
  }
  return STORAGE_RESULT_OK;
}

storage_result_t storage_write(storage_t *storage, file_id_t file_id,
                               uint8_t *data) {
  if (storage == NULL || data == NULL || storage->state == NULL ||
      storage->commands == NULL) {
    return STORAGE_RESULT_ERROR_UNKNOWN;
  }
  if (file_id == FILE_ID_COMMAND) {
    seqlock_write_payload(storage->commands, data);
    return STORAGE_RESULT_OK;
  }
  if (file_id == FILE_ID_AHRS) {
    seqlock_write_payload(storage->state + (size_t)file_id * STORAGE_FILE_SIZE,
                          data);
    return STORAGE_RESULT_OK;
  }
  for (int i = 0; i < STORAGE_FILE_SIZE; i++) {
    storage->state[file_id * STORAGE_FILE_SIZE + i] = data[i];
  }
  return STORAGE_RESULT_OK;
}

storage_result_t storage_read(storage_t *storage, file_id_t file_id,
                              uint8_t *data) {
  if (storage == NULL || data == NULL || storage->state == NULL ||
      storage->commands == NULL) {
    return STORAGE_RESULT_ERROR_UNKNOWN;
  }
  if (file_id == FILE_ID_COMMAND) {
    seqlock_read_payload(storage->commands, data);
    data[STORAGE_FILE_SIZE - 1] = 0;
    return STORAGE_RESULT_OK;
  }
  if (file_id == FILE_ID_AHRS) {
    seqlock_read_payload(storage->state + (size_t)file_id * STORAGE_FILE_SIZE,
                         data);
    data[STORAGE_FILE_SIZE - 1] = 0;
    return STORAGE_RESULT_OK;
  }
  for (int i = 0; i < STORAGE_FILE_SIZE; i++) {
    data[i] = storage->state[file_id * STORAGE_FILE_SIZE + i];
  }
  return STORAGE_RESULT_OK;
}
