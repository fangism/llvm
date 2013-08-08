; RUN: opt < %s -dfsan -S | FileCheck %s
target datalayout = "e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v64:64:64-v128:128:128-a0:0:64-s0:64:64-f80:128:128-n8:16:32:64-S128"

define i8 @add(i8 %a, i8 %b) {
  ; CHECK: @add
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: call{{.*}}__dfsan_union
  ; CHECK: add i8
  ; CHECK: store{{.*}}__dfsan_retval_tls
  ; CHECK: ret i8
  %c = add i8 %a, %b
  ret i8 %c
}

define i8 @sub(i8 %a, i8 %b) {
  ; CHECK: @sub
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: call{{.*}}__dfsan_union
  ; CHECK: sub i8
  ; CHECK: store{{.*}}__dfsan_retval_tls
  ; CHECK: ret i8
  %c = sub i8 %a, %b
  ret i8 %c
}

define i8 @mul(i8 %a, i8 %b) {
  ; CHECK: @mul
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: call{{.*}}__dfsan_union
  ; CHECK: mul i8
  ; CHECK: store{{.*}}__dfsan_retval_tls
  ; CHECK: ret i8
  %c = mul i8 %a, %b
  ret i8 %c
}

define i8 @sdiv(i8 %a, i8 %b) {
  ; CHECK: @sdiv
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: call{{.*}}__dfsan_union
  ; CHECK: sdiv i8
  ; CHECK: store{{.*}}__dfsan_retval_tls
  ; CHECK: ret i8
  %c = sdiv i8 %a, %b
  ret i8 %c
}

define i8 @udiv(i8 %a, i8 %b) {
  ; CHECK: @udiv
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: load{{.*}}__dfsan_arg_tls
  ; CHECK: call{{.*}}__dfsan_union
  ; CHECK: udiv i8
  ; CHECK: store{{.*}}__dfsan_retval_tls
  ; CHECK: ret i8
  %c = udiv i8 %a, %b
  ret i8 %c
}
