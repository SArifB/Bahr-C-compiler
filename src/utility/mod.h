#pragma once
#include <stddef.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef size_t usize;
typedef ptrdiff_t isize;

typedef float f32;
typedef double f64;
typedef long double f128;

typedef const char* cstr;
typedef char* str;
typedef void* any;

typedef struct StrView StrView;
struct StrView {
  cstr itr;
  cstr sen;
};

#define unused [[maybe_unused]]
#define undiscardable [[nodiscard]]
#define noaddress [[no_unique_address]]

#define stringify_inner(STR) #STR
#define stringify(STR) stringify_inner(STR)

#define fn(...) typeof(__VA_ARGS__)*

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define sizeof_arr(arr) (sizeof(arr) / sizeof(*(arr)))

#define eprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#define eputs(string) fputs(string "\n", stderr);
#define eputc(ch) fputc(ch, stderr);
