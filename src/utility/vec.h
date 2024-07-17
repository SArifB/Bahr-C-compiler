#pragma once
#include <utility/mod.h>

#define DEFINE_VECTOR(T)              \
  typedef struct T##Vector T##Vector; \
  struct T##Vector {                  \
    usize capacity;                   \
    usize length;                     \
    T buffer[];                       \
  };

#define DEFINE_VEC_FNS(T, alloc, dealloc)                                     \
  unused undiscardable static T##Vector* T##_vector_make(usize size) {        \
    usize new_size = 4;                                                       \
    while (new_size < size) {                                                 \
      new_size *= 2;                                                          \
    }                                                                         \
    T##Vector* tmp = alloc(sizeof(T##Vector) + sizeof(T) * new_size);         \
    if (tmp == nullptr) {                                                     \
      eprintf("%s\n", stringify(alloc));                                      \
      exit(1);                                                                \
    }                                                                         \
    *tmp = (T##Vector){ .capacity = new_size };                               \
    return tmp;                                                               \
  }                                                                           \
                                                                              \
  unused undiscardable static T##Vector* T##_vector_realloc(                  \
    T##Vector** vec_adrs, usize size                                          \
  ) {                                                                         \
    T##Vector* vec = *vec_adrs;                                               \
    T##Vector* tmp = T##_vector_make(size);                                   \
    memcpy(                                                                   \
      (void* restrict)tmp->buffer, (const void* restrict)vec->buffer,         \
      sizeof(T) * vec->length                                                 \
    );                                                                        \
    tmp->length = vec->length;                                                \
    dealloc(vec);                                                             \
    *vec_adrs = tmp;                                                          \
    return tmp;                                                               \
  }                                                                           \
                                                                              \
  unused static void T##_vector_push(T##Vector** vec_adrs, T val) {           \
    T##Vector* vec = *vec_adrs;                                               \
    if (vec->length == vec->capacity) {                                       \
      vec = T##_vector_realloc(vec_adrs, vec->length + 1);                    \
    }                                                                         \
    vec->buffer[vec->length] = val;                                           \
    vec->length += 1;                                                         \
  }                                                                           \
                                                                              \
  unused static void T##_vector_push_many(                                    \
    T##Vector** vec_adrs, const T* src, usize src_size                        \
  ) {                                                                         \
    T##Vector* vec = *vec_adrs;                                               \
    if (vec->length + src_size >= vec->capacity) {                            \
      vec = T##_vector_realloc(vec_adrs, vec->length + src_size);             \
    }                                                                         \
    memcpy(                                                                   \
      (void* restrict)(vec->buffer + vec->length), (const void* restrict)src, \
      sizeof(T) * src_size                                                    \
    );                                                                        \
    vec->length += src_size;                                                  \
  }                                                                           \
                                                                              \
  unused static void T##_vector_concat(                                       \
    T##Vector** vec_adrs, const T##Vector* src                                \
  ) {                                                                         \
    T##Vector* vec = *vec_adrs;                                               \
    if (vec->length + src->length >= vec->capacity) {                         \
      vec = T##_vector_realloc(vec_adrs, vec->length + src->length);          \
    }                                                                         \
    memcpy(                                                                   \
      (void* restrict)(vec->buffer + vec->length),                            \
      (const void* restrict)src->buffer, sizeof(T) * src->length              \
    );                                                                        \
    vec->length += src->length;                                               \
  }                                                                           \
                                                                              \
  unused static void T##_vector_pop(T##Vector* vec) {                         \
    if (vec->length != 0) {                                                   \
      vec->length -= 1;                                                       \
    }                                                                         \
  }                                                                           \
                                                                              \
  unused static void T##_vector_pop_many(T##Vector* vec, usize size) {        \
    if (size > vec->length) {                                                 \
      vec->length = 0;                                                        \
    } else {                                                                  \
      vec->length -= size;                                                    \
    }                                                                         \
  }
