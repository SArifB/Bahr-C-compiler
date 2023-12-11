; ModuleID = 'some_code'
source_filename = "some_code"

define i32 @add_test(i32 %0) {
entry:
  %ret_val = add i32 20, %0
  ret i32 %ret_val
}

