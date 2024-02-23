#include <hashmap/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

HashMap* hashmap_make(usize size) {
  usize final_size = 2;
  while (final_size < size) {
    final_size *= 2;
  }
  HashMap* map = calloc(1, sizeof(HashMap) + sizeof(HashMapNode*) * final_size);
  if (map == nullptr) {
    perror("calloc");
    exit(1);
  }
  map->size = final_size;
  return map;
}

void hashmap_free(HashMap* map) {
  HashMapNode* cursor = nullptr;
  for (usize i = 0; i != map->size; ++i) {
    cursor = map->buckets[i];
    while (cursor != nullptr) {
      HashMapNode* tmp = cursor;
      cursor = cursor->next;
      free(tmp);
    }
  }
  free(map);
}

static inline usize write_hash(usize hash, const u8* bytes, usize length);

void** hashmap_get(HashMap* map, StrView key) {
  usize hash = write_hash(0, (u8*)key.ptr, key.size);
  usize index = hash & (map->size - 1);
  HashMapNode** cursor = &map->buckets[index];

  while (*cursor != nullptr) {
    HashMapNode* latest = *cursor;
    if (latest->key_hash == hash) {
      return &latest->value;
    }
    cursor = &latest->next;
  }

  HashMapNode* node = malloc(sizeof(HashMapNode));
  if (node == nullptr) {
    perror("malloc");
    exit(1);
  }
  *node = (HashMapNode){
    .key_hash = hash,
  };

  *cursor = node;
  return &node->value;
}

#if __SIZEOF_POINTER__ == 8
#define K 0x517cc1b727220a95UL
#else
#define K 0x9e3779b9UL
#endif

static inline usize rotate_left(usize value, int count) {
  return (value << count) | (value >> (sizeof(usize) * 8 - count));
}

static inline usize add_to_hash(usize hash, usize i) {
  hash *= K;
  hash ^= rotate_left(hash, 5) ^ i;
  return hash;
}

static inline usize write_hash(usize hash, const u8* bytes, usize length) {
  while (length >= sizeof(usize)) {
    usize val = {};
    memcpy(&val, bytes, sizeof(usize));
    hash = add_to_hash(hash, val);
    bytes += sizeof(usize);
    length -= sizeof(usize);
  }
  if (length >= 4) {
    uint32_t val = 0;
    memcpy(&val, bytes, 4);
    hash = add_to_hash(hash, val);
    bytes += 4;
    length -= 4;
  }
  if (length >= 2) {
    uint32_t val = 0;
    memcpy(&val, bytes, 2);
    hash = add_to_hash(hash, val);
    bytes += 2;
    length -= 2;
  }
  if (length >= 1) {
    hash = add_to_hash(hash, *bytes);
  }
  return hash;
}
