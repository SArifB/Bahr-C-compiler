#pragma once
#include <utility/mod.h>

#define LINKED_LIST 0
#define OPEN_ADDRESSING 1

#ifndef MAP_TYPE
#define MAP_TYPE OPEN_ADDRESSING
#endif

#ifndef MAP_MAX_LOAD
#define MAP_MAX_LOAD(SIZE) (((SIZE) / 10) * 7)
#endif

#if MAP_TYPE == LINKED_LIST

typedef struct HashNode HashNode;
struct HashNode {
  HashNode* next;
  void* value;
  usize key;
};

typedef struct HashMap HashMap;
struct HashMap {
  usize capacity;
  HashNode* entries[];
};

#elif MAP_TYPE == OPEN_ADDRESSING

typedef struct HashNode HashNode;
struct HashNode {
  void* value;
  usize key;
};

typedef struct HashMap HashMap;
struct HashMap {
  usize capacity;
  usize length;
  HashNode entries[];
};

#endif

extern HashMap* hashmap_make(usize size);
extern void hashmap_free(HashMap* map);
extern void** hashmap_get(HashMap** map_adrs, StrView key);
extern void** hashmap_find(HashMap** map_adrs, StrView key);
