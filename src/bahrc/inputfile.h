#pragma once
#include <utility/mod.h>

typedef struct Inputfile Inputfile;
struct Inputfile {
  StrView content;
  const i32 descriptor;
};

extern Inputfile inputfile_make(StrView filename);
extern void inputfile_free(Inputfile file);
