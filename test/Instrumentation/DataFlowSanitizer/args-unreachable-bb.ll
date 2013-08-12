; RUN: opt < %s -dfsan -verify -dfsan-args-abi -S | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"

define i8 @unreachable_bb() {
  ; CHECK: @unreachable_bb
  ; CHECK: ret { i8, i16 } { i8 1, i16 0 }
  ; CHECK-NOT: bb2:
  ; CHECK-NOT: bb3:
  ; CHECK-NOT: bb4:
  ret i8 1

bb2:
  ret i8 2

bb3:
  br label %bb4

bb4:
  br label %bb3
}
