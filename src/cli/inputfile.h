#pragma once
#include <utility/mod.h>

#ifdef _WIN32  // TODO: Test on Windows
#include <windows.h>
#endif

typedef struct InputFile InputFile;
struct InputFile {
  const cstr name;
  const cstr file;
  const usize length;
#ifdef __linux__
  const i32 descriptor;
#elif _WIN32  // TODO: Test on Windows
  const HANDLE handle;
  const HANDLE mapping;
#endif
};

extern InputFile input_file(cstr file_name);
extern void input_free(InputFile input);
