#pragma once
#include <utility/mod.h>

typedef struct InputFile InputFile;
struct InputFile {
  const cstr name;
  const cstr file;
  const usize size;
  const i32 descriptor;
};

extern InputFile input_file(cstr filename);
extern void input_free(InputFile input);
