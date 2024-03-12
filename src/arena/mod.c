#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

#ifdef __linux__
#include <arena/mod.h>
#include <sys/mman.h>
#elif _WIN32  // TODO: Test on Windows
#include <windows.h>
#endif

#define DEF_REG_CAP (usize)(8 * 1024)
#define ELEM_SIZE(size) (((size) + sizeof(isize) - 1) / sizeof(isize))

static Region* region_alloc(const usize capacity) {
  const usize size = sizeof(Region) + sizeof(usize) * capacity;
#ifdef __linux__
  Region* tmp = mmap(
    nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0
  );
  if (tmp == MAP_FAILED) {
    perror("mmap");
    exit(1);
  }
#elif _WIN32  // TODO: Test on Windows
  Region* tmp =
    VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
  if (tmp == nullptr) {
    perror("VirtualAlloc");
    exit(1);
  }
#else
  Region* tmp = malloc(size);
  if (tmp == nullptr) {
    perror("malloc");
    exit(1);
  }
#endif
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
#ifdef __linux__
    if (munmap(tmp, tmp->capacity) == -1) {
      perror("munmap");
      exit(1);
    };
#elif _WIN32  // TODO: Test on Windows
    if (VirtualFree(myRegion, 0, MEM_RELEASE) == 0) {
      perror("VirtualFree");
      exit(1);
    }
#else
    free(tmp);
#endif
  }
}
