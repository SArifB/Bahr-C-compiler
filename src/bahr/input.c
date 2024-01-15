#include <bahr/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility/mod.h>

InputFile input_file(cstr filename) {
  i32 fd = open(filename, O_RDONLY);
  if (fd == -1) {
    exit(1);
  }
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    exit(1);
  }
  cstr addr = mmap(nullptr, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (addr == MAP_FAILED) {
    exit(1);
  }
  return (InputFile){
    .name = filename,
    .file = addr,
    .size = sb.st_size,
    .descriptor = fd,
  };
}

void input_free(InputFile input) {
  if (munmap((str)input.file, input.size) == -1) {
    exit(1);
  };
  if (close(input.descriptor) == -1) {
    exit(1);
  }
}
