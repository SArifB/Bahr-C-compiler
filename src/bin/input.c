#include <bin/input.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility/mod.h>

StrView input_file(const char* filename) {
  i32 fd = open(filename, O_RDONLY);
  if (fd == -1) {
    exit(1);
  }
  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    exit(1);
  }
  char* addr = mmap(nullptr, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (addr == MAP_FAILED) {
    exit(1);
  }
  close(fd);
  return (StrView){addr, addr + sb.st_size};
}

void input_free(StrView view) {
  munmap((char*)view.itr, view_len(view));
}