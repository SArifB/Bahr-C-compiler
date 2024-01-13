#pragma once
#include <utility/mod.h>

typedef struct HashMapNode HashMapNode;
struct HashMapNode {
  HashMapNode* next;
  void* value;
  usize key_hash;
};

typedef struct HashMap HashMap;
struct HashMap {
  usize size;
  HashMapNode* buckets[];
};

extern HashMap* hashmap_make(usize size);
extern void hashmap_free(HashMap* map);
extern void** hashmap_get(HashMap* map, StrView key);
