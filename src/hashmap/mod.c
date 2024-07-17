#include <hashmap/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

static inline usize get_hash(StrView key);
static HashMap* alloc_table(usize size);
static HashNode* get_entry(HashMap* map, usize hash);

HashMap* hashmap_make(usize size) {
  usize final_size = 4;
  while (final_size < size) {
    final_size *= 2;
  }
  return alloc_table(final_size);
}

static inline usize get_index(usize index, usize size) {
  return index & (size - 1);
}

#if MAP_TYPE == LINKED_LIST

static HashNode* alloc_node(usize hash);
static HashNode* insert_node(HashMap* map, usize hash);

void hashmap_free(HashMap* map) {
  for (usize i = 0; i != map->size; ++i) {
    HashNode* cursor = map->entries[i];
    while (cursor != nullptr) {
      HashNode* tmp = cursor;
      cursor = cursor->next;
      free(tmp);
    }
  }
  free(map);
}

void** hashmap_get(HashMap** map_adrs, StrView key) {
  HashMap* map = *map_adrs;
  usize hash = get_hash(key);

  HashNode* entry = get_entry(map, hash);
  if (entry != nullptr) {
    return &entry->value;
  }
  entry = insert_node(map, hash);
  return &entry->value;
}

void** hashmap_find(HashMap** map_adrs, StrView key) {
  HashMap* map = *map_adrs;
  usize hash = get_hash(key);
  HashNode* entry = get_entry(map, hash);
  if (entry != nullptr) {
    return &entry->value;
  }
  return nullptr;
}

static HashMap* alloc_table(usize size) {
  HashMap* map = calloc(1, sizeof(HashMap*) + sizeof(HashNode*) * size);
  if (map == nullptr) {
    perror("calloc");
    exit(1);
  }
  map->size = size;
  return map;
}

static HashNode* alloc_node(usize hash) {
  HashNode* node = malloc(sizeof(HashNode));
  if (node == nullptr) {
    perror("malloc");
    exit(1);
  }
  *node = (HashNode){ .key = hash };
  return node;
}

static HashNode* get_entry(HashMap* map, usize hash) {
  usize index = get_index(hash, map->size);
  HashNode* entry = map->entries[index];
  for (; entry != nullptr; entry = entry->next) {
    if (entry->key == hash) {
      break;
    }
  }
  return entry;
}

static HashNode* insert_node(HashMap* map, usize hash) {
  usize index = get_index(hash, map->size);
  HashNode* new_entry = alloc_node(hash);
  new_entry->next = map->entries[index];
  map->entries[index] = new_entry;
  return new_entry;
}

#else

static HashMap* resize(HashMap* old_table);

void hashmap_free(HashMap* map_adrs) {
  free(map_adrs);
}

void** hashmap_get(HashMap** map_adrs, StrView key) {
  HashMap* map = *map_adrs;
  if (map->capacity >= MAP_MAX_LOAD(map->length)) {
    *map_adrs = resize(map);
    map = *map_adrs;
  }
  usize hash = get_hash(key);
  HashNode* entry = get_entry(map, hash);
  if (entry->key != 0) {
    return &entry->value;
  }
  map->capacity += 1;
  entry->key = hash;
  return &entry->value;
}

void** hashmap_find(HashMap** map_adrs, StrView key) {
  usize hash = get_hash(key);
  HashNode* entry = get_entry(*map_adrs, hash);
  if (entry->key != 0) {
    return &entry->value;
  }
  return nullptr;
}

static HashMap* alloc_table(usize size) {
  HashMap* map_adrs = calloc(1, sizeof(HashMap) + sizeof(HashNode) * size);
  if (map_adrs == nullptr) {
    perror("calloc");
    exit(1);
  }
  map_adrs->length = size;
  return map_adrs;
}

static HashNode* get_entry(HashMap* map, usize hash) {
  usize index = get_index(hash, map->length);
  HashNode* cursor = map->entries + index;

  while (cursor->key != 0) {
    if (cursor->key == hash) {
      break;
    }
    index = get_index(index + 1, map->length);
    cursor = map->entries + index;
  }
  return cursor;
}

static HashMap* resize(HashMap* old_table) {
  HashMap* new_table = alloc_table(old_table->length * 2);
  HashNode* iter = old_table->entries;
  HashNode* sentinel = old_table->entries + old_table->length;

  for (; iter != sentinel; ++iter) {
    *get_entry(new_table, iter->key) = *iter;
  }
  new_table->capacity = old_table->capacity;
  free(old_table);
  return new_table;
}

#endif

#if __SIZEOF_POINTER__ == 8
#define K 0x517cc1b727220a95UL
#else
#define K 0x9e3779b9UL
#endif

static inline usize rotate_left(usize value, u32 count) {
  return (value << count) | (value >> (sizeof(usize) * 8 - count));
}

static inline usize add_to_hash(usize hash, usize i) {
  hash *= K;
  hash ^= rotate_left(hash, 5) ^ i;
  return hash;
}

static inline usize get_hash(StrView key) {
  usize hash = 0;
  usize length = key.length;
  const u8* restrict bytes = (const u8* restrict)key.pointer;

  while (length >= sizeof(usize)) {
    usize val = 0;
    memcpy(&val, bytes, sizeof(usize));
    hash = add_to_hash(hash, val);
    bytes += sizeof(usize);
    length -= sizeof(usize);
  }
  if (length >= 4) {
    u32 val = 0;
    memcpy(&val, bytes, 4);
    hash = add_to_hash(hash, val);
    bytes += 4;
    length -= 4;
  }
  if (length >= 2) {
    u32 val = 0;
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
