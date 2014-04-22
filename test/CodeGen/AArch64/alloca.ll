; RUN: llc -mtriple=aarch64-none-linux-gnu -verify-machineinstrs < %s | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-AARCH64
; RUN: llc -mtriple=arm64-linux-gnu -verify-machineinstrs -o - %s | FileCheck %s --check-prefix=CHECK --check-prefix=CHECK-ARM64
; RUN: llc -mtriple=aarch64-none-linux-gnu -mattr=-fp-armv8 -verify-machineinstrs < %s | FileCheck --check-prefix=CHECK-NOFP %s

declare void @use_addr(i8*)

define void @test_simple_alloca(i64 %n) {
; CHECK-LABEL: test_simple_alloca:

  %buf = alloca i8, i64 %n
  ; Make sure we align the stack change to 16 bytes:
; CHECK: {{mov|add}} x29
; CHECK: mov [[TMP:x[0-9]+]], sp
; CHECK: add [[SPDELTA_TMP:x[0-9]+]], x0, #15
; CHECK: and [[SPDELTA:x[0-9]+]], [[SPDELTA_TMP]], #0xfffffffffffffff0

  ; Make sure we change SP. It would be surprising if anything but x0 were used
  ; for the final sp, but it could be if it was then moved into x0.
; CHECK: sub [[NEWSP:x[0-9]+]], [[TMP]], [[SPDELTA]]
; CHECK: mov sp, [[NEWSP]]

  call void @use_addr(i8* %buf)
; CHECK: bl use_addr

  ret void
  ; Make sure epilogue restores sp from fp
; CHECK: {{sub|mov}} sp, x29
; CHECK: ret
}

declare void @use_addr_loc(i8*, i64*)

define i64 @test_alloca_with_local(i64 %n) {
; CHECK-LABEL: test_alloca_with_local:
; CHECK-DAG: sub sp, sp, [[LOCAL_STACK:#[0-9]+]]
; CHECK-DAG: {{mov|add}} x29, sp

  %loc = alloca i64
  %buf = alloca i8, i64 %n
  ; Make sure we align the stack change to 16 bytes:
; CHECK: mov [[TMP:x[0-9]+]], sp
; CHECK: add [[SPDELTA_TMP:x[0-9]+]], x0, #15
; CHECK: and [[SPDELTA:x[0-9]+]], [[SPDELTA_TMP]], #0xfffffffffffffff0

  ; Make sure we change SP. It would be surprising if anything but x0 were used
  ; for the final sp, but it could be if it was then moved into x0.
; CHECK: sub [[NEWSP:x[0-9]+]], [[TMP]], [[SPDELTA]]
; CHECK: mov sp, [[NEWSP]]

; CHECK: sub {{x[0-9]+}}, x29, #[[LOC_FROM_FP:[0-9]+]]

  call void @use_addr_loc(i8* %buf, i64* %loc)
; CHECK: bl use_addr

  %val = load i64* %loc
; CHECK-AARCH64: sub x[[TMP:[0-9]+]], x29, #[[LOC_FROM_FP]]
; CHECK-AARCH64: ldr x0, [x[[TMP]]]

; CHECK-ARM64: ldur x0, [x29, #-[[LOC_FROM_FP]]]

  ret i64 %val
  ; Make sure epilogue restores sp from fp
; CHECK: {{sub|mov}} sp, x29
; CHECK: ret
}

define void @test_variadic_alloca(i64 %n, ...) {
; CHECK-LABEL: test_variadic_alloca:

; CHECK-AARCH64: sub     sp, sp, #{{[0-9]+}}
; CHECK-AARCH64: add     x29, sp, #192
; CHECK-AARCH64: sub     [[TMP:x[0-9]+]], x29, #192
; CHECK-AARCH64: add     x8, [[TMP]], #0
; CHECK-AARCH64-FP: str     q7, [x8, #112]
; [...]
; CHECK-AARCH64-FP: str     q1, [x8, #16]


; CHECK-NOFP: sub     sp, sp, #80
; CHECK-NOFP: stp     x29, x30, [sp, #64]
; CHECK-NOFP: add     x29, sp, #64
; CHECK-NOFP: sub     [[TMP:x[0-9]+]], x29, #64
; CHECK-NOFP: add     x8, [[TMP]], #0


; CHECK-ARM64: stp     x29, x30, [sp, #-16]!
; CHECK-ARM64: mov     x29, sp
; CHECK-ARM64: sub     sp, sp, #192
; CHECK-ARM64: stp     q6, q7, [x29, #-96]
; [...]
; CHECK-ARM64: stp     q0, q1, [x29, #-192]

; CHECK-ARM64: stp     x6, x7, [x29, #-16]
; [...]
; CHECK-ARM64: stp     x2, x3, [x29, #-48]


  %addr = alloca i8, i64 %n

  call void @use_addr(i8* %addr)
; CHECK: bl use_addr

  ret void
; CHECK-AARCH64: sub sp, x29, #192
; CHECK-AARCH64: ldp x29, x30, [sp, #192]
; CHECK-AARCH64: add sp, sp, #208

; CHECK-NOFP: sub sp, x29, #64
; CHECK-NOFP: ldp x29, x30, [sp, #64]
; CHECK-NOFP: add sp, sp, #80
}

define void @test_alloca_large_frame(i64 %n) {
; CHECK-LABEL: test_alloca_large_frame:

; CHECK-AARCH64: sub sp, sp, #496
; CHECK-AARCH64: stp x29, x30, [sp, #480]
; CHECK-AARCH64: add x29, sp, #480
; CHECK-AARCH64: sub sp, sp, #48
; CHECK-AARCH64: sub sp, sp, #1953, lsl #12

; CHECK-ARM64: stp     x20, x19, [sp, #-32]!
; CHECK-ARM64: stp     x29, x30, [sp, #16]
; CHECK-ARM64: add     x29, sp, #16
; CHECK-ARM64: sub     sp, sp, #7999488
; CHECK-ARM64: sub     sp, sp, #512

  %addr1 = alloca i8, i64 %n
  %addr2 = alloca i64, i64 1000000

  call void @use_addr_loc(i8* %addr1, i64* %addr2)

  ret void
; CHECK-AARCH64: sub sp, x29, #480
; CHECK-AARCH64: ldp x29, x30, [sp, #480]
; CHECK-AARCH64: add sp, sp, #496

; CHECK-ARM64: sub     sp, x29, #16
; CHECK-ARM64: ldp     x29, x30, [sp, #16]
; CHECK-ARM64: ldp     x20, x19, [sp], #32
}

declare i8* @llvm.stacksave()
declare void @llvm.stackrestore(i8*)

define void @test_scoped_alloca(i64 %n) {
; CHECK-LABEL: test_scoped_alloca:

  %sp = call i8* @llvm.stacksave()
; CHECK: mov [[SAVED_SP:x[0-9]+]], sp
; CHECK: mov [[OLDSP:x[0-9]+]], sp

  %addr = alloca i8, i64 %n
; CHECK: and [[SPDELTA:x[0-9]+]], {{x[0-9]+}}, #0xfffffffffffffff0
; CHECK-DAG: sub [[NEWSP:x[0-9]+]], [[OLDSP]], [[SPDELTA]]
; CHECK: mov sp, [[NEWSP]]

  call void @use_addr(i8* %addr)
; CHECK: bl use_addr

  call void @llvm.stackrestore(i8* %sp)
; CHECK: mov sp, [[SAVED_SP]]

  ret void
}
