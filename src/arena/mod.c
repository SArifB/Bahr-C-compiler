#include <arena/mod.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <utility/mod.h>

#define DEF_REG_CAP (usize)(8 * 1024)
#define ELEM_SIZE(size) (((size) + sizeof(isize) - 1) / sizeof(isize))

static Region* region_alloc(const usize capacity) {
  const usize size = sizeof(Region) + sizeof(usize) * capacity;
  Region* tmp = mmap(
    nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
  );
  if (tmp == MAP_FAILED) {
    exit(1);
  }
  *tmp = (Region){.capacity = DEF_REG_CAP};
  return tmp;
}

void* arena_alloc(Arena arena[static 1], const usize size_in) {
  const usize size = ELEM_SIZE(size_in);

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

  void* result = &arena->end->data[arena->end->count];
  arena->end->count += size;
  return result;
}

static void memory_copy(
  usize* const restrict dest, const usize* const restrict src, const usize size
) {
  for (usize i = 0; i < size; ++i) {
    dest[i] = src[i];
  }
}

void* arena_realloc(
  Arena arena[static 1], void* old_ptr, const usize old_size,
  const usize new_size
) {
  if (new_size <= old_size) {
    return old_ptr;
  }
  if (old_ptr == &arena->end->data[arena->end->count - ELEM_SIZE(old_size)]) {
    arena->end->count += ELEM_SIZE(new_size) - ELEM_SIZE(old_size);
    return old_ptr;
  }
  void* tmp = arena_alloc(arena, new_size);
  memory_copy(tmp, old_ptr, old_size);
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
    if (munmap(tmp, tmp->capacity) == -1) {
      exit(1);
    };
  }
}
