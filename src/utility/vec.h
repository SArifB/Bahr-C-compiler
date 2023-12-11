#pragma once
#include <utility/mod.h>

#define DECLARE_VECTOR(T)                                                      \
  typedef struct T##Vector T##Vector;                                          \
  struct T##Vector {                                                           \
    usize capacity;                                                            \
    usize length;                                                              \
    T buffer[];                                                                \
  };

#define DEFINE_VECTOR(T, alloc, dealloc)                                       \
  extern T##Vector* T##_vector_make(const usize size);                         \
  extern void T##_vector_push(T##Vector** vec, const T val);                   \
  extern void T##_vector_push_many(                                            \
    T##Vector** vec, const T* src, const usize src_size                        \
  );                                                                           \
  extern void T##_vector_pop(T##Vector* vec);                                  \
  extern void T##_vector_pop_many(T##Vector* vec, const usize src_size);       \
  extern void T##_vector_copy(                                                 \
    const T* const src, const usize src_size, T* dest                          \
  );                                                                           \
                                                                               \
  T##Vector* T##_vector_make(const usize size) {                               \
    T##Vector* tmp = alloc(sizeof(T##Vector) + sizeof(T) * size);              \
    if (tmp == nullptr) {                                                      \
      eprintf("%s\n", stringify(alloc));                                       \
      exit(-1);                                                                \
    }                                                                          \
    *tmp = (T##Vector){ .capacity = size };                                    \
    return tmp;                                                                \
  }                                                                            \
                                                                               \
  void T##_vector_push(T##Vector** vec_addr, const T val) {                    \
    T##Vector* vec = *vec_addr;                                                \
    if (vec->length >= vec->capacity) {                                        \
      usize new_size = 2;                                                      \
      while (new_size <= vec->capacity) {                                      \
        new_size *= 2;                                                         \
      }                                                                        \
      T##Vector* tmp = T##_vector_make(new_size);                              \
      T##_vector_copy(vec->buffer, vec->length, tmp->buffer);                  \
      tmp->length = vec->length;                                               \
      dealloc(vec);                                                            \
      *vec_addr = tmp;                                                         \
      vec = tmp;                                                               \
    }                                                                          \
    vec->buffer[vec->length] = val;                                            \
    vec->length += 1;                                                          \
  }                                                                            \
                                                                               \
  void T##_vector_push_many(                                                   \
    T##Vector** vec_addr, const T* src, const usize src_size                   \
  ) {                                                                          \
    T##Vector* vec = *vec_addr;                                                \
    if (src_size + vec->length >= vec->capacity) {                             \
      usize new_size = 2;                                                      \
      while (new_size <= vec->capacity) {                                      \
        new_size *= 2;                                                         \
      }                                                                        \
      T##Vector* tmp = T##_vector_make(new_size);                              \
      T##_vector_copy(vec->buffer, vec->length, tmp->buffer);                  \
      tmp->length = vec->length;                                               \
      dealloc(vec);                                                            \
      *vec_addr = tmp;                                                         \
      vec = tmp;                                                               \
    }                                                                          \
    T##_vector_copy(src, src_size - 1, vec->buffer + vec->length);             \
    vec->length += src_size;                                                   \
  }                                                                            \
                                                                               \
  void T##_vector_pop(T##Vector* vec) {                                        \
    if (vec->length == 0) {                                                    \
      return;                                                                  \
    }                                                                          \
    vec->length -= 1;                                                          \
  }                                                                            \
                                                                               \
  void T##_vector_pop_many(T##Vector* vec, const usize src_size) {             \
    for (usize i = 0; i < src_size; ++i) {                                     \
      if (vec->length == 0) {                                                  \
        return;                                                                \
      }                                                                        \
      vec->length -= 1;                                                        \
    }                                                                          \
  }                                                                            \
                                                                               \
  void T##_vector_copy(const T* const src, const usize src_size, T* dest) {    \
    for (usize i = 0; i < src_size; ++i) {                                     \
      dest[i] = src[i];                                                        \
    }                                                                          \
  }
