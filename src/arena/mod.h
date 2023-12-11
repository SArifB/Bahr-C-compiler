#pragma once
#include <utility/mod.h>

typedef struct Region Region;
typedef struct Arena Arena;

struct Region {
  Region* next;
  usize count;
  usize capacity;
  usize data[];
};

struct Arena {
  Region* begin;
  Region* end;
};

extern Region* region_alloc(const usize);
extern any arena_alloc(Arena[static 1], const usize);
extern any arena_realloc(Arena[static 1], any, const usize, const usize);
extern void arena_reset(Arena[static 1]);
extern void arena_free(Arena[static 1]);

extern Arena* DEFAULT_ARENA;
#define aalloc(size) arena_alloc(DEFAULT_ARENA, size)
