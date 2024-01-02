; ModuleID = 'some_code'
source_filename = "some_code"

@.str = private unnamed_addr global [13 x i8] c"hello \0Aworld\00", align 1
@.str.1 = private unnamed_addr global [10 x i8] c"some text\00", align 1

declare i32 @puts(ptr)

declare i32 @float_fn(float)

define i32 @main(i32 %0, ptr %1) {
entry:
  %argc = alloca i32, align 4
  store i32 %0, ptr %argc, align 4
  %argv = alloca ptr, align 8
  store ptr %1, ptr %argv, align 8
  %2 = load ptr, ptr %argv, align 8
  %3 = load ptr, ptr %2, align 8
  %puts = call i32 @puts(ptr %3)
  %message = alloca ptr, align 8
  store ptr @.str, ptr %message, align 8
  %4 = load ptr, ptr %message, align 8
  %puts1 = call i32 @puts(ptr %4)
  %puts2 = call i32 @puts(ptr @.str.1)
  ret i32 0
}
