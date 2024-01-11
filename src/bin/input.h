#pragma once
#include <utility/mod.h>

typedef struct InputFile InputFile;
struct InputFile {
  cstr name;
  cstr file;
  usize size;
};

extern InputFile input_file(cstr filename);
extern void input_free(InputFile input);
