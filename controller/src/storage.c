#include "controller/storage.h"
#include <fcntl.h>
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

static void voxor_commit(uint8_t *state, slot_id_t slot_id) {
  uint8_t *slot_base = state + (size_t)slot_id * STORAGE_FILE_SIZE;
  atomic_fetch_xor_explicit((_Atomic uint8_t *)slot_base, 1U,
                            memory_order_release);
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

  return STORAGE_RESULT_OK;
}

storage_result_t storage_write(storage_t *storage, slot_id_t slot_id,
                               uint8_t *data) {
  return STORAGE_RESULT_OK;
}

storage_result_t storage_read(storage_t *storage, slot_id_t slot_id,
                              uint8_t *data) {
  return STORAGE_RESULT_OK;
}

storage_result_t storage_commit(storage_t *storage, slot_id_t slot_id) {
  return STORAGE_RESULT_OK;
}