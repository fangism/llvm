; RUN: llc -verify-machineinstrs -mtriple=aarch64-none-linux-gnu < %s | FileCheck %s

; This test aims to check basic correctness of frame layout &
; frame access code. There are 8 functions in this test file,
; each function implements one element in the cartesian product
; of:
; . a function having a VLA/noVLA
; . a function with dynamic stack realignment/no dynamic stack realignment.
; . a function needing a frame pionter/no frame pointer,
; since the presence/absence of these has influence on the frame
; layout and which pointer to use to access various part of the
; frame (bp,sp,fp).
;
; Furthermore: in every test function:
; . there is always one integer and 1 floating point argument to be able
;   to check those are accessed correctly.
; . there is always one local variable to check that is accessed
;   correctly
;
; The LLVM-IR below was produced by clang on the following C++ code:
;extern "C" int g();
;extern "C" int novla_nodynamicrealign_call(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                             double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  volatile int l1;
;  return i10 + (int)d10 + l1 + g();
;}
;extern "C" int novla_nodynamicrealign_nocall(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                             double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  volatile int l1;
;  return i10 + (int)d10 + l1;
;}
;extern "C" int novla_dynamicrealign_call(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                         double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  alignas(128) volatile int l1;
;  return i10 + (int)d10 + l1 + g();
;}
;extern "C" int novla_dynamicrealign_nocall(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                           double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  alignas(128) volatile int l1;
;  return i10 + (int)d10 + l1;
;}
;
;extern "C" int vla_nodynamicrealign_call(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                         double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  volatile int l1;
;  volatile int vla[i1];
;  return i10 + (int)d10 + l1 + g() + vla[0];
;}
;extern "C" int vla_nodynamicrealign_nocall(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                           double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  volatile int l1;
;  volatile int vla[i1];
;  return i10 + (int)d10 + l1 + vla[0];
;}
;extern "C" int vla_dynamicrealign_call(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                       double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  alignas(128) volatile int l1;
;  volatile int vla[i1];
;  return i10 + (int)d10 + l1 + g() + vla[0];
;}
;extern "C" int vla_dynamicrealign_nocall(int i1, int i2, int i3, int i4, int i5, int i6, int i7, int i8, int i9, int i10,
;                                         double d1, double d2, double d3, double d4, double d5, double d6, double d7, double d8, double d9, double d10)
;{
;  // use an argument passed on the stack.
;  alignas(128) volatile int l1;
;  volatile int vla[i1];
;  return i10 + (int)d10 + l1 + vla[0];
;}



define i32 @novla_nodynamicrealign_call(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #0 {
entry:
  %l1 = alloca i32, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 4
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %call = tail call i32 @g()
  %add2 = add nsw i32 %add1, %call
  ret i32 %add2
}
; CHECK-LABEL: novla_nodynamicrealign_call
; CHECK: .cfi_startproc
;   Check that used callee-saved registers are saved
; CHECK: stp	x20, x19, [sp, #-32]!
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #16]
; CHECK: add	x29, sp, #16
;   Check correctness of cfi pseudo-instructions
; CHECK: .cfi_def_cfa w29, 16
; CHECK: .cfi_offset w30, -8
; CHECK: .cfi_offset w29, -16
; CHECK: .cfi_offset w19, -24
; CHECK: .cfi_offset w20, -32
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
;   Check correct access to local variable on the stack, through stack pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [sp, #12]
;   Check epilogue:
; CHECK: ldp	x29, x30, [sp, #16]
; CHECK: ldp	x20, x19, [sp], #32
; CHECK: ret
; CHECK: .cfi_endproc


declare i32 @g() #0

; Function Attrs: nounwind
define i32 @novla_nodynamicrealign_nocall(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #1 {
entry:
  %l1 = alloca i32, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 4
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  ret i32 %add1
}
; CHECK-LABEL: novla_nodynamicrealign_nocall
;   Check that space is reserved for one local variable on the stack.
; CHECK:	sub	sp, sp, #16             // =16
;   Check correct access to arguments passed on the stack, through stack pointer
; CHECK: ldr	d[[DARG:[0-9]+]], [sp, #40]
; CHECK: ldr	w[[IARG:[0-9]+]], [sp, #24]
;   Check correct access to local variable on the stack, through stack pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [sp, #12]
;   Check epilogue:
; CHECK: add	sp, sp, #16             // =16
; CHECK: ret


define i32 @novla_dynamicrealign_call(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #0 {
entry:
  %l1 = alloca i32, align 128
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 128
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %call = tail call i32 @g()
  %add2 = add nsw i32 %add1, %call
  ret i32 %add2
}

; CHECK-LABEL: novla_dynamicrealign_call
; CHECK: .cfi_startproc
;   Check that used callee-saved registers are saved
; CHECK: stp	x20, x19, [sp, #-32]!
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #16]
; CHECK: add	x29, sp, #16
;   Check the dynamic realignment of the stack pointer to a 128-byte boundary
; CHECK: sub	x9, sp, #96
; CHECK: and	sp, x9, #0xffffffffffffff80
;   Check correctness of cfi pseudo-instructions
; CHECK: .cfi_def_cfa w29, 16
; CHECK: .cfi_offset w30, -8
; CHECK: .cfi_offset w29, -16
; CHECK: .cfi_offset w19, -24
; CHECK: .cfi_offset w20, -32
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
;   Check correct access to local variable on the stack, through re-aligned stack pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [sp]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: sub	sp, x29, #16            // =16
; CHECK: ldp	x29, x30, [sp, #16]
; CHECK: ldp	x20, x19, [sp], #32
; CHECK: ret
; CHECK: .cfi_endproc


; Function Attrs: nounwind
define i32 @novla_dynamicrealign_nocall(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #1 {
entry:
  %l1 = alloca i32, align 128
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 128
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  ret i32 %add1
}

; CHECK-LABEL: novla_dynamicrealign_nocall
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #-16]!
; CHECK: mov	x29, sp
;   Check the dynamic realignment of the stack pointer to a 128-byte boundary
; CHECK: sub	x9, sp, #112
; CHECK: and	sp, x9, #0xffffffffffffff80
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
;   Check correct access to local variable on the stack, through re-aligned stack pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [sp]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: mov	sp, x29
; CHECK: ldp	x29, x30, [sp], #16
; CHECK: ret


define i32 @vla_nodynamicrealign_call(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #0 {
entry:
  %l1 = alloca i32, align 4
  %0 = zext i32 %i1 to i64
  %vla = alloca i32, i64 %0, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 4
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %call = tail call i32 @g()
  %add2 = add nsw i32 %add1, %call
  %1 = load volatile i32, i32* %vla, align 4, !tbaa !1
  %add3 = add nsw i32 %add2, %1
  ret i32 %add3
}

; CHECK-LABEL: vla_nodynamicrealign_call
; CHECK: .cfi_startproc
;   Check that used callee-saved registers are saved
; CHECK: stp	x20, x19, [sp, #-32]!
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #16]
; CHECK: add	x29, sp, #16
;   Check that space is reserved on the stack for the local variable,
;   rounded up to a multiple of 16 to keep the stack pointer 16-byte aligned.
; CHECK: sub	sp, sp, #16
;   Check correctness of cfi pseudo-instructions
; CHECK: .cfi_def_cfa w29, 16
; CHECK: .cfi_offset w30, -8
; CHECK: .cfi_offset w29, -16
; CHECK: .cfi_offset w19, -24
; CHECK: .cfi_offset w20, -32
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
;   Check correct reservation of 16-byte aligned VLA (size in w0) on stack
; CHECK: ubfx	x9, x0, #0, #32
; CHECK: lsl	x9, x9, #2
; CHECK: add	x9, x9, #15
; CHECK: and	x9, x9, #0xfffffffffffffff0
; CHECK: mov	 x10, sp
; CHECK: sub	 x[[VLASPTMP:[0-9]+]], x10, x9
; CHECK: mov	 sp, x[[VLASPTMP]]
;   Check correct access to local variable, through frame pointer
; CHECK: ldur	w[[ILOC:[0-9]+]], [x29, #-20]
;   Check correct accessing of the VLA variable through the base pointer
; CHECK: ldr	w[[VLA:[0-9]+]], [x[[VLASPTMP]]]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: sub	sp, x29, #16            // =16
; CHECK: ldp	x29, x30, [sp, #16]
; CHECK: ldp	x20, x19, [sp], #32
; CHECK: ret
; CHECK: .cfi_endproc


; Function Attrs: nounwind
define i32 @vla_nodynamicrealign_nocall(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #1 {
entry:
  %l1 = alloca i32, align 4
  %0 = zext i32 %i1 to i64
  %vla = alloca i32, i64 %0, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 4
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %1 = load volatile i32, i32* %vla, align 4, !tbaa !1
  %add2 = add nsw i32 %add1, %1
  ret i32 %add2
}

; CHECK-LABEL: vla_nodynamicrealign_nocall
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #-16]!
; CHECK: mov	x29, sp
;   Check that space is reserved on the stack for the local variable,
;   rounded up to a multiple of 16 to keep the stack pointer 16-byte aligned.
; CHECK: sub	sp, sp, #16
;   Check correctness of cfi pseudo-instructions
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
;   Check correct reservation of 16-byte aligned VLA (size in w0) on stack
; CHECK: ubfx	x9, x0, #0, #32
; CHECK: lsl	x9, x9, #2
; CHECK: add	x9, x9, #15
; CHECK: and	x9, x9, #0xfffffffffffffff0
; CHECK: mov	 x10, sp
; CHECK: sub	 x[[VLASPTMP:[0-9]+]], x10, x9
; CHECK: mov	 sp, x[[VLASPTMP]]
;   Check correct access to local variable, through frame pointer
; CHECK: ldur	w[[ILOC:[0-9]+]], [x29, #-4]
;   Check correct accessing of the VLA variable through the base pointer
; CHECK: ldr	w[[VLA:[0-9]+]], [x[[VLASPTMP]]]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: mov    sp, x29
; CHECK: ldp	x29, x30, [sp], #16
; CHECK: ret


define i32 @vla_dynamicrealign_call(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #0 {
entry:
  %l1 = alloca i32, align 128
  %0 = zext i32 %i1 to i64
  %vla = alloca i32, i64 %0, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 128
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %call = tail call i32 @g()
  %add2 = add nsw i32 %add1, %call
  %1 = load volatile i32, i32* %vla, align 4, !tbaa !1
  %add3 = add nsw i32 %add2, %1
  ret i32 %add3
}

; CHECK-LABEL: vla_dynamicrealign_call
; CHECK: .cfi_startproc
;   Check that used callee-saved registers are saved
; CHECK: stp	x22, x21, [sp, #-48]!
; CHECK: stp	x20, x19, [sp, #16]
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #32]
; CHECK: add	x29, sp, #32
;   Check that the stack pointer gets re-aligned to 128
;   bytes & the base pointer (x19) gets initialized to
;   this 128-byte aligned area for local variables &
;   spill slots
; CHECK: sub	x9, sp, #80            // =80
; CHECK: and	sp, x9, #0xffffffffffffff80
; CHECK: mov    x19, sp
;   Check correctness of cfi pseudo-instructions
; CHECK: .cfi_def_cfa w29, 16
; CHECK: .cfi_offset w30, -8
; CHECK: .cfi_offset w29, -16
; CHECK: .cfi_offset w19, -24
; CHECK: .cfi_offset w20, -32
; CHECK: .cfi_offset w21, -40
; CHECK: .cfi_offset w22, -48
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
;   Check correct reservation of 16-byte aligned VLA (size in w0) on stack
;   and set-up of base pointer (x19).
; CHECK: ubfx	x9, x0, #0, #32
; CHECK: lsl	x9, x9, #2
; CHECK: add	x9, x9, #15
; CHECK: and	x9, x9, #0xfffffffffffffff0
; CHECK: mov	 x10, sp
; CHECK: sub	 x[[VLASPTMP:[0-9]+]], x10, x9
; CHECK: mov	 sp, x[[VLASPTMP]]
;   Check correct access to local variable, through base pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [x19]
; CHECK: ldr	 w[[VLA:[0-9]+]], [x[[VLASPTMP]]]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: sub	sp, x29, #32
; CHECK: ldp	x29, x30, [sp, #32]
; CHECK: ldp	x20, x19, [sp, #16]
; CHECK: ldp	x22, x21, [sp], #48
; CHECK: ret
; CHECK: .cfi_endproc


; Function Attrs: nounwind
define i32 @vla_dynamicrealign_nocall(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #1 {
entry:
  %l1 = alloca i32, align 128
  %0 = zext i32 %i1 to i64
  %vla = alloca i32, i64 %0, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 128
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %1 = load volatile i32, i32* %vla, align 4, !tbaa !1
  %add2 = add nsw i32 %add1, %1
  ret i32 %add2
}

; CHECK-LABEL: vla_dynamicrealign_nocall
;   Check that used callee-saved registers are saved
; CHECK: stp	x20, x19, [sp, #-32]!
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #16]
; CHECK: add	x29, sp, #16
;   Check that the stack pointer gets re-aligned to 128
;   bytes & the base pointer (x19) gets initialized to
;   this 128-byte aligned area for local variables &
;   spill slots
; CHECK: sub	x9, sp, #96
; CHECK: and	sp, x9, #0xffffffffffffff80
; CHECK: mov    x19, sp
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
;   Check correct reservation of 16-byte aligned VLA (size in w0) on stack
;   and set-up of base pointer (x19).
; CHECK: ubfx	x9, x0, #0, #32
; CHECK: lsl	x9, x9, #2
; CHECK: add	x9, x9, #15
; CHECK: and	x9, x9, #0xfffffffffffffff0
; CHECK: mov	 x10, sp
; CHECK: sub	 x[[VLASPTMP:[0-9]+]], x10, x9
; CHECK: mov	 sp, x[[VLASPTMP]]
;   Check correct access to local variable, through base pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [x19]
; CHECK: ldr	 w[[VLA:[0-9]+]], [x[[VLASPTMP]]]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: sub	sp, x29, #16
; CHECK: ldp	x29, x30, [sp, #16]
; CHECK: ldp	x20, x19, [sp], #32
; CHECK: ret


; Function Attrs: nounwind
define i32 @vla_dynamicrealign_nocall_large_align(i32 %i1, i32 %i2, i32 %i3, i32 %i4, i32 %i5, i32 %i6, i32 %i7, i32 %i8, i32 %i9, i32 %i10, double %d1, double %d2, double %d3, double %d4, double %d5, double %d6, double %d7, double %d8, double %d9, double %d10) #1 {
entry:
  %l1 = alloca i32, align 32768
  %0 = zext i32 %i1 to i64
  %vla = alloca i32, i64 %0, align 4
  %conv = fptosi double %d10 to i32
  %add = add nsw i32 %conv, %i10
  %l1.0.l1.0. = load volatile i32, i32* %l1, align 32768
  %add1 = add nsw i32 %add, %l1.0.l1.0.
  %1 = load volatile i32, i32* %vla, align 4, !tbaa !1
  %add2 = add nsw i32 %add1, %1
  ret i32 %add2
}

; CHECK-LABEL: vla_dynamicrealign_nocall_large_align
;   Check that used callee-saved registers are saved
; CHECK: stp	x20, x19, [sp, #-32]!
;   Check that the frame pointer is created:
; CHECK: stp	x29, x30, [sp, #16]
; CHECK: add	x29, sp, #16
;   Check that the stack pointer gets re-aligned to 128
;   bytes & the base pointer (x19) gets initialized to
;   this 128-byte aligned area for local variables &
;   spill slots
; CHECK: sub	x9, sp, #7, lsl #12
; CHECK: and	sp, x9, #0xffffffffffff8000
; CHECK: mov    x19, sp
;   Check correct access to arguments passed on the stack, through frame pointer
; CHECK: ldr	w[[IARG:[0-9]+]], [x29, #24]
; CHECK: ldr	d[[DARG:[0-9]+]], [x29, #40]
;   Check correct reservation of 16-byte aligned VLA (size in w0) on stack
;   and set-up of base pointer (x19).
; CHECK: ubfx	x9, x0, #0, #32
; CHECK: lsl	x9, x9, #2
; CHECK: add	x9, x9, #15
; CHECK: and	x9, x9, #0xfffffffffffffff0
; CHECK: mov	 x10, sp
; CHECK: sub	 x[[VLASPTMP:[0-9]+]], x10, x9
; CHECK: mov	 sp, x[[VLASPTMP]]
;   Check correct access to local variable, through base pointer
; CHECK: ldr	w[[ILOC:[0-9]+]], [x19]
; CHECK: ldr	 w[[VLA:[0-9]+]], [x[[VLASPTMP]]]
;   Check epilogue:
;     Check that stack pointer get restored from frame pointer.
; CHECK: sub	sp, x29, #16
; CHECK: ldp	x29, x30, [sp, #16]
; CHECK: ldp	x20, x19, [sp], #32
; CHECK: ret

attributes #0 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

!1 = !{!2, !2, i64 0}
!2 = !{!"int", !3, i64 0}
!3 = !{!"omnipotent char", !4, i64 0}
!4 = !{!"Simple C/C++ TBAA"}
