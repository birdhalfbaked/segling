#include "storage.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <unistd.h>

storage_result_t storage_init(storage_t *storage) {
  storage_result_t result = STORAGE_RESULT_OK;
  // create stable memory map that will need to be accessible by
  // external processes
  // this memory map will be used to store the data of the system
  int fd = open("/var/run/segling/storage.bin", O_RDWR | O_SYNC);
  void *map = NULL;

  if (fd == -1) {
    perror("open storage.bin");
    result = STORAGE_RESULT_ERROR_LOAD_FAILED;
  }
  if (result == STORAGE_RESULT_OK) {
    map = mmap(NULL, STORAGE_MAX_FILES * STORAGE_FILE_SIZE,
               PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED) {
      perror("mmap storage.bin");
      result = STORAGE_RESULT_ERROR_LOAD_FAILED;
    }
  }
  if (result == STORAGE_RESULT_OK) {
    // read the data from the memory map
    storage->files = map;
  }
  close(fd);
  return result;
}

storage_result_t storage_write(storage_t *storage, file_id_t file_id,
                               uint8_t *data) {
  for (int i = 0; i < STORAGE_FILE_SIZE; i++) {
    storage->files[file_id * STORAGE_FILE_SIZE + i] = data[i];
  }
  return STORAGE_RESULT_OK;
}

storage_result_t storage_read(storage_t *storage, file_id_t file_id,
                              uint8_t *data) {
  for (int i = 0; i < STORAGE_FILE_SIZE; i++) {
    data[i] = storage->files[file_id * STORAGE_FILE_SIZE + i];
  }
  return STORAGE_RESULT_OK;
}