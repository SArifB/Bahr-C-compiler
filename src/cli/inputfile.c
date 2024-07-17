#include <arena/mod.h>
#include <cli/inputfile.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

#ifdef __linux__
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

InputFile input_file(rcstr file_name) {
  i32 fd = open(file_name, O_RDONLY);
  if (fd == -1) {
    perror("open");
    exit(1);
  }

  struct stat sb;
  if (fstat(fd, &sb) == -1) {
    perror("stat");
    exit(1);
  }

  rcstr adrs = mmap(nullptr, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
  if (adrs == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }

  rcstr name = strdup(file_name);
  if (name == nullptr) {
    perror("strdup");
    exit(1);
  }

  return (InputFile){
    .name = name,
    .file = adrs,
    .length = sb.st_size,
    .descriptor = fd,
  };
}

void input_free(InputFile input) {
  if (munmap((void*)input.file, input.length) == -1) {
    perror("munmap");
    exit(1);
  };
  if (close(input.descriptor) == -1) {
    perror("close");
    exit(1);
  }
  free((void*)input.name);
}

#elif _WIN32  // TODO: Test on Windows
#include <windows.h>

InputFile input_file(rcstr file_name) {
  HANDLE handle = CreateFile(
    file_name, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, nullptr
  );
  if (handle == INVALID_HANDLE_VALUE) {
    perror("CreateFile");
    exit(1);
  }

  LARGE_INTEGER file_size;
  if (Getfile_sizeEx(handle, &file_size) == 0) {
    perror("Getfile_sizeEx");
    CloseHandle(handle);
    exit(1);
  }

  HANDLE mapping = Createmapping(handle, nullptr, PAGE_READONLY, 0, 0, nullptr);
  if (mapping == nullptr) {
    perror("Createmapping");
    CloseHandle(handle);
    exit(1);
  }

  LPCVOID file_view = MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0);
  if (file_view == nullptr) {
    perror("MapViewOfFile");
    CloseHandle(mapping);
    CloseHandle(handle);
    exit(1);
  }

  rcstr name = strdup(file_name);
  if (name == nullptr) {
    perror("strdup");
    exit(1);
  }

  return (InputFile){
    .name = name,
    .file = file_view,
    .size = file_size.QuadPart,
    .handle = handle,
    .mapping = mapping,
  };
}

void input_free(InputFile input) {
  if (!UnmapViewOfFile(input.file)) {
    perror("UnmapViewOfFile");
    exit(1);
  }
  if (!CloseHandle(input.mapping)) {
    perror("CloseHandle");
    exit(1);
  }
  if (!CloseHandle(input.handle)) {
    perror("CloseHandle");
    exit(1);
  }
  free((void*)input.name);
}
#endif
