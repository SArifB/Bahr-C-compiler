; ModuleID = 'some_code'
source_filename = "some_code"

@.str = private unnamed_addr global [12 x i8] c"hello world\00", align 1

declare i32 @puts(ptr)

define i32 @main() {
entry:
  %message = alloca ptr, align 8
  store ptr @.str, ptr %message, align 8
  %0 = load ptr, ptr %message, align 8
  %call = call i32 @puts(ptr %0)
  ret i32 0
}
