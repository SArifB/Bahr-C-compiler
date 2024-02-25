#pragma once
#include <utility/mod.h>

typedef struct HashMap HashMap;
struct HashMap {
  void* table;
};

extern HashMap hashmap_make(usize size);
extern void hashmap_free(HashMap map);
extern void** hashmap_get(HashMap* map, StrView key);
extern void** hashmap_find(HashMap* map, StrView key);
