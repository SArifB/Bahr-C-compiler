#include <bahrc/inputfile.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utility/mod.h>

Inputfile inputfile_make(StrView filename) {
  if (filename.pointer[filename.length] != '\0') {
    eputn("Invalid file name, required to be nullbyte terminated: ");
    eputw(filename);
    exit(1);
  }

  i32 fd = open(filename.pointer, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    perror("fstat");
    exit(1);
  }

  rcstr address = mmap(nullptr, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (address == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  return (Inputfile){
    .content = {
      .pointer = address,
      .length = st.st_size,
    },
    .descriptor = fd,
  };
}

void inputfile_free(Inputfile file) {
  if (munmap((void*)file.content.pointer, file.content.length) == -1) {
    perror("munmap");
    exit(1);
  };
  if (close(file.descriptor) == -1) {
    perror("close");
    exit(1);
  }
}
