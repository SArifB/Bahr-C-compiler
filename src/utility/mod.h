#pragma once
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uintptr_t usize;
typedef intptr_t isize;

typedef float f32;
typedef double f64;
typedef long double f128;

typedef const char* cstr;
typedef char* str;

typedef struct StrView StrView;
struct StrView {
  cstr ptr;
  usize size;
};

typedef struct StrArr StrArr;
struct StrArr {
  usize size;
  char array[];
};

#define unused [[maybe_unused]]
#define undiscardable [[nodiscard]]
#define unreturning [[noreturn]]
#define noaddress [[no_unique_address]]

#define stringify_inner(STR) #STR
#define stringify(STR) stringify_inner(STR)

#define fn(...) typeof(__VA_ARGS__)*

#define max(a, b) (((a) > (b)) ? (a) : (b))
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define sizeof_arr(arr) (sizeof(arr) / sizeof(*(arr)))

#define eprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define evprintf(fmt, ...) vfprintf(stderr, fmt, __VA_ARGS__)
#define eputs(string) fputs(string "\n", stderr)
#define eputw(view) eprintf("%.*s\n", (i32)(view).size, (view).ptr)
#define eputa(arr) eprintf("%.*s\n", (i32)(arr)->size, (arr)->array)
#define eputc(ch) fputc(ch, stderr)
