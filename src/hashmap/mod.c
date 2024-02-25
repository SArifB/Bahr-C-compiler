#include <hashmap/mod.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utility/mod.h>

#define LINKED_LIST 0
#define OPEN_ADDRESSING 1

#define MAP_TYPE OPEN_ADDRESSING
#if MAP_TYPE == LINKED_LIST

typedef struct HashNode HashNode;
struct HashNode {
  HashNode* next;
  void* value;
  usize key;
};

typedef struct HashTable HashTable;
struct HashTable {
  usize size;
  HashNode* entries[];
};

#elif MAP_TYPE == OPEN_ADDRESSING

typedef struct HashNode HashNode;
struct HashNode {
  void* value;
  usize key;
};

typedef struct HashTable HashTable;
struct HashTable {
  usize size;
  usize count;
  HashNode entries[];
};

#endif

static inline usize get_hash(usize hash, const u8* bytes, usize length);
static HashTable* alloc_table(usize size);
static HashNode* get_entry(HashTable* table, usize hash);

HashMap hashmap_make(usize size) {
  usize final_size = 4;
  while (final_size < size) {
    final_size *= 2;
  }
  HashTable* table = alloc_table(final_size);
  return (HashMap){table};
}

static inline usize get_index(usize index, usize size) {
  return index & (size - 1);
}

#if MAP_TYPE == LINKED_LIST

static HashNode* alloc_node(usize hash);
static HashNode* insert_node(HashTable* table, usize hash);

void hashmap_free(HashMap map) {
  for (usize i = 0; i != map.table->size; ++i) {
    HashNode* cursor = map.table->entries[i];
    while (cursor != nullptr) {
      HashNode* tmp = cursor;
      cursor = cursor->next;
      free(tmp);
    }
  }
  free(map.table);
}

void** hashmap_get(HashMap* map, StrView key) {
  HashTable* table = map->table;
  usize hash = get_hash(0, (u8*)key.ptr, key.size);

  HashNode* entry = get_entry(table, hash);
  if (entry != nullptr) {
    return &entry->value;
  }
  entry = insert_node(table, hash);
  return &entry->value;
}

void** hashmap_find(HashMap* map, StrView key) {
  HashTable* table = map->table;
  usize hash = get_hash(0, (u8*)key.ptr, key.size);
  HashNode* entry = get_entry(table, hash);
  if (entry != nullptr) {
    return &entry->value;
  }
  return nullptr;
}

static HashTable* alloc_table(usize size) {
  HashTable* table = calloc(1, sizeof(HashMap) + sizeof(HashNode*) * size);
  if (table == nullptr) {
    perror("calloc");
    exit(1);
  }
  table->size = size;
  return table;
}

static HashNode* alloc_node(usize hash) {
  HashNode* node = malloc(sizeof(HashNode));
  if (node == nullptr) {
    perror("malloc");
    exit(1);
  }
  *node = (HashNode){.key = hash};
  return node;
}

static HashNode* get_entry(HashTable* table, usize hash) {
  usize index = get_index(hash, table->size);
  HashNode* entry = table->entries[index];
  for (; entry != nullptr; entry = entry->next) {
    if (entry->key == hash) {
      break;
    }
  }
  return entry;
}

static HashNode* insert_node(HashTable* table, usize hash) {
  usize index = get_index(hash, table->size);
  HashNode* new_entry = alloc_node(hash);
  new_entry->next = table->entries[index];
  table->entries[index] = new_entry;
  return new_entry;
}

#else

static HashTable* resize(HashTable* old_table);

void hashmap_free(HashMap map) {
  free(map.table);
}

void** hashmap_get(HashMap* map, StrView key) {
  HashTable* table = map->table;
  if (table->count == table->size) {
    map->table = resize(table);
    table = map->table;
  }
  usize hash = get_hash(0, (u8*)key.ptr, key.size);
  HashNode* entry = get_entry(table, hash);
  if (entry->key != 0) {
    return &entry->value;
  }
  table->count += 1;
  entry->key = hash;
  return &entry->value;
}

void** hashmap_find(HashMap* map, StrView key) {
  usize hash = get_hash(0, (u8*)key.ptr, key.size);
  HashNode* entry = get_entry(map->table, hash);
  if (entry->key != 0) {
    return &entry->value;
  }
  return nullptr;
}

static HashTable* alloc_table(usize size) {
  HashTable* map = calloc(1, sizeof(HashTable) + sizeof(HashNode) * size);
  if (map == nullptr) {
    perror("calloc");
    exit(1);
  }
  map->size = size;
  return map;
}

static HashNode* get_entry(HashTable* table, usize hash) {
  usize index = get_index(hash, table->size);
  HashNode* cursor = table->entries + index;

  while (cursor->key != 0) {
    if (cursor->key == hash) {
      break;
    }
    index = get_index(index + 1, table->size);
    cursor = table->entries + index;
  }
  return cursor;
}

static HashTable* resize(HashTable* old_table) {
  HashTable* new_table = alloc_table(old_table->size * 2);
  HashNode* iter = old_table->entries;
  HashNode* sentinel = old_table->entries + old_table->size;

  for (; iter != sentinel; ++iter) {
    *get_entry(new_table, iter->key) = *iter;
  }
  new_table->count = old_table->count;
  free(old_table);
  return new_table;
}

#endif

#if __SIZEOF_POINTER__ == 8
#define K 0x517cc1b727220a95UL
#else
#define K 0x9e3779b9UL
#endif

static inline usize rotate_left(usize value, i32 count) {
  return (value << count) | (value >> (sizeof(usize) * 8 - count));
}

static inline usize add_to_hash(usize hash, usize i) {
  hash *= K;
  hash ^= rotate_left(hash, 5) ^ i;
  return hash;
}

static inline usize get_hash(usize hash, const u8* bytes, usize length) {
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
