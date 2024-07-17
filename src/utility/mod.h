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

typedef const char* restrict rcstr;
typedef char* restrict rstr;

typedef struct StrView StrView;
struct StrView {
  const usize length;
  const rcstr pointer;
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

#define eputc(ch) fputc(ch, stderr)
#define eputs(string) fputs(string "\n", stderr)
#define evprintf(fmt, ...) vfprintf(stderr, fmt, __VA_ARGS__)
#define eprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__)
#define eprintln(fmt, ...) eprintf(fmt "\n", __VA_ARGS__)
#define eputw(view) eprintln("%.*s", (i32)(view).length, (view).pointer)
#define eputa(arr) eprintln("%.*s", (i32)(arr)->length, (arr)->array)
