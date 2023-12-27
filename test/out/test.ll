; ModuleID = 'some_code'
source_filename = "some_code"

define i32 @test() {
entry:
  ret i32 20
}

define i32 @square(i32 %0) {
entry:
  %arg = alloca i32, align 4
  store i32 %0, ptr %arg, align 4
  %1 = load i32, ptr %arg, align 4
  %2 = load i32, ptr %arg, align 4
  %mul = mul i32 %1, %2
  ret i32 %mul
}
