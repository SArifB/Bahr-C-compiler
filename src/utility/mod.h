#pragma once

typedef signed char i8;
typedef signed short i16;
typedef signed int i32;
typedef signed long i64;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long u64;

typedef typeof(sizeof(int)) usize;
typedef typeof((int*)0 - (int*)0) isize;

typedef float f32;
typedef double f64;

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
// #define value_name(STR) STR stringify(__COUNTER__)

#define defer defer__2(__COUNTER__)
#define defer__2(X) defer__3(X)
#define defer__3(X) defer__4(defer__id##X)
#define defer__4(ID)                                                           \
  auto void ID##func(...);                                                     \
  __attribute__((cleanup(ID##func))) char __##ID##defer__;                     \
  void ID##func(...)

#define lambda(return_type, function_body)                                     \
  ({ return_type __fn__ function_body __fn__; })

#define fn(...) typeof(__VA_ARGS__)*

#define max(a, b) ((a > b) ? (a) : (b))
#define min(a, b) ((a < b) ? (a) : (b))
#define cast(ptr, T) ((T*)ptr)
#define sizeof_arr(arr) sizeof(arr) / sizeof(*arr)

#define eprintf(fmt, ...) fprintf(stderr, fmt, __VA_ARGS__);
#define eputs(string) fputs(string "\n", stderr);
#define eputc(ch) fputc(ch, stderr);
