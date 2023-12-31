#include <arena/mod.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

Arena* DEFAULT_ARENA = &(Arena){};

#define DEF_REG_CAP (usize)(8 * 1024)

Region* region_alloc(const usize capacity) {
  Region* tmp = malloc(sizeof(Region) + sizeof(usize) * capacity);
  if (tmp != nullptr) {
    *tmp = (Region){.capacity = capacity};
  }
  return tmp;
}

any arena_alloc(Arena arena[static 1], const usize size_in) {
  const usize size = (size_in + sizeof(isize) - 1) / sizeof(isize);

  if (arena->end == nullptr) {
    arena->end = region_alloc(max(DEF_REG_CAP, size));
    arena->begin = arena->end;
  }

  while (arena->end->count + size > arena->end->capacity &&
         arena->end->next != nullptr) {
    arena->end = arena->end->next;
  }

  if (arena->end->count + size > arena->end->capacity) {
    arena->end->next = region_alloc(max(DEF_REG_CAP, size));
    arena->end = arena->end->next;
  }

  any result = &arena->end->data[arena->end->count];
  arena->end->count += size;
  return result;
}

any arena_realloc(
  Arena arena[static 1], any old_ptr, const usize old_size, const usize new_size
) {
  if (new_size <= old_size) {
    return old_ptr;
  }
  any tmp = arena_alloc(arena, new_size);
  memcpy(tmp, old_ptr, old_size);
  return tmp;
}

void arena_reset(Arena arena[static 1]) {
  for (Region* region = arena->begin; region != nullptr;
       region = region->next) {
    region->count = 0;
  }
  arena->end = arena->begin;
}

void arena_free(Arena arena[static 1]) {
  Region* region = arena->begin;

  while (region != nullptr) {
    Region* tmp = region;
    region = region->next;
    free(tmp);
  }
}
