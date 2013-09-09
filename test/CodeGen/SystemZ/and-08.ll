; Test memory-to-memory ANDs.
;
; RUN: llc < %s -mtriple=s390x-linux-gnu | FileCheck %s

@g1 = global i8 1
@g2 = global i16 2

; Test the simple i8 case.
define void @f1(i8 *%ptr1) {
; CHECK-LABEL: f1:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %old = load i8 *%ptr2
  %and = and i8 %val, %old
  store i8 %and, i8 *%ptr2
  ret void
}

; ...and again in reverse.
define void @f2(i8 *%ptr1) {
; CHECK-LABEL: f2:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %old = load i8 *%ptr2
  %and = and i8 %old, %val
  store i8 %and, i8 *%ptr2
  ret void
}

; Test i8 cases where one value is zero-extended to 32 bits and the other
; sign-extended.
define void @f3(i8 *%ptr1) {
; CHECK-LABEL: f3:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %extval = zext i8 %val to i32
  %old = load i8 *%ptr2
  %extold = sext i8 %old to i32
  %and = and i32 %extval, %extold
  %trunc = trunc i32 %and to i8
  store i8 %trunc, i8 *%ptr2
  ret void
}

; ...and again with the extension types reversed.
define void @f4(i8 *%ptr1) {
; CHECK-LABEL: f4:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %extval = sext i8 %val to i32
  %old = load i8 *%ptr2
  %extold = zext i8 %old to i32
  %and = and i32 %extval, %extold
  %trunc = trunc i32 %and to i8
  store i8 %trunc, i8 *%ptr2
  ret void
}

; ...and again with two sign extensions.
define void @f5(i8 *%ptr1) {
; CHECK-LABEL: f5:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %extval = sext i8 %val to i32
  %old = load i8 *%ptr2
  %extold = sext i8 %old to i32
  %and = and i32 %extval, %extold
  %trunc = trunc i32 %and to i8
  store i8 %trunc, i8 *%ptr2
  ret void
}

; ...and again with two zero extensions.
define void @f6(i8 *%ptr1) {
; CHECK-LABEL: f6:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %extval = zext i8 %val to i32
  %old = load i8 *%ptr2
  %extold = zext i8 %old to i32
  %and = and i32 %extval, %extold
  %trunc = trunc i32 %and to i8
  store i8 %trunc, i8 *%ptr2
  ret void
}

; Test i8 cases where the value is extended to 64 bits (just one case
; this time).
define void @f7(i8 *%ptr1) {
; CHECK-LABEL: f7:
; CHECK: nc 1(1,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i8 *%ptr1, i64 1
  %val = load i8 *%ptr1
  %extval = sext i8 %val to i64
  %old = load i8 *%ptr2
  %extold = zext i8 %old to i64
  %and = and i64 %extval, %extold
  %trunc = trunc i64 %and to i8
  store i8 %trunc, i8 *%ptr2
  ret void
}

; Test the simple i16 case.
define void @f8(i16 *%ptr1) {
; CHECK-LABEL: f8:
; CHECK: nc 2(2,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i16 *%ptr1, i64 1
  %val = load i16 *%ptr1
  %old = load i16 *%ptr2
  %and = and i16 %val, %old
  store i16 %and, i16 *%ptr2
  ret void
}

; Test i16 cases where the value is extended to 32 bits.
define void @f9(i16 *%ptr1) {
; CHECK-LABEL: f9:
; CHECK: nc 2(2,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i16 *%ptr1, i64 1
  %val = load i16 *%ptr1
  %extval = zext i16 %val to i32
  %old = load i16 *%ptr2
  %extold = sext i16 %old to i32
  %and = and i32 %extval, %extold
  %trunc = trunc i32 %and to i16
  store i16 %trunc, i16 *%ptr2
  ret void
}

; Test i16 cases where the value is extended to 64 bits.
define void @f10(i16 *%ptr1) {
; CHECK-LABEL: f10:
; CHECK: nc 2(2,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i16 *%ptr1, i64 1
  %val = load i16 *%ptr1
  %extval = sext i16 %val to i64
  %old = load i16 *%ptr2
  %extold = zext i16 %old to i64
  %and = and i64 %extval, %extold
  %trunc = trunc i64 %and to i16
  store i16 %trunc, i16 *%ptr2
  ret void
}

; Test the simple i32 case.
define void @f11(i32 *%ptr1) {
; CHECK-LABEL: f11:
; CHECK: nc 4(4,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i32 *%ptr1, i64 1
  %val = load i32 *%ptr1
  %old = load i32 *%ptr2
  %and = and i32 %old, %val
  store i32 %and, i32 *%ptr2
  ret void
}

; Test i32 cases where the value is extended to 64 bits.
define void @f12(i32 *%ptr1) {
; CHECK-LABEL: f12:
; CHECK: nc 4(4,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i32 *%ptr1, i64 1
  %val = load i32 *%ptr1
  %extval = sext i32 %val to i64
  %old = load i32 *%ptr2
  %extold = zext i32 %old to i64
  %and = and i64 %extval, %extold
  %trunc = trunc i64 %and to i32
  store i32 %trunc, i32 *%ptr2
  ret void
}

; Test the i64 case.
define void @f13(i64 *%ptr1) {
; CHECK-LABEL: f13:
; CHECK: nc 8(8,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i64 *%ptr1, i64 1
  %val = load i64 *%ptr1
  %old = load i64 *%ptr2
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2
  ret void
}

; Make sure that we don't use NC if the first load is volatile.
define void @f14(i64 *%ptr1) {
; CHECK-LABEL: f14:
; CHECK-NOT: nc
; CHECK: br %r14
  %ptr2 = getelementptr i64 *%ptr1, i64 1
  %val = load volatile i64 *%ptr1
  %old = load i64 *%ptr2
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2
  ret void
}

; ...likewise the second.
define void @f15(i64 *%ptr1) {
; CHECK-LABEL: f15:
; CHECK-NOT: nc
; CHECK: br %r14
  %ptr2 = getelementptr i64 *%ptr1, i64 1
  %val = load i64 *%ptr1
  %old = load volatile i64 *%ptr2
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2
  ret void
}

; ...likewise the store.
define void @f16(i64 *%ptr1) {
; CHECK-LABEL: f16:
; CHECK-NOT: nc
; CHECK: br %r14
  %ptr2 = getelementptr i64 *%ptr1, i64 1
  %val = load i64 *%ptr1
  %old = load i64 *%ptr2
  %and = and i64 %old, %val
  store volatile i64 %and, i64 *%ptr2
  ret void
}

; Test that NC is used for aligned loads and stores, even if there is
; no way of telling whether they alias.
define void @f17(i64 *%ptr1, i64 *%ptr2) {
; CHECK-LABEL: f17:
; CHECK: nc 0(8,%r3), 0(%r2)
; CHECK: br %r14
  %val = load i64 *%ptr1
  %old = load i64 *%ptr2
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2
  ret void
}

; ...but if one of the loads isn't aligned, we can't be sure.
define void @f18(i64 *%ptr1, i64 *%ptr2) {
; CHECK-LABEL: f18:
; CHECK-NOT: nc
; CHECK: br %r14
  %val = load i64 *%ptr1, align 2
  %old = load i64 *%ptr2
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2
  ret void
}

; Repeat the previous test with the operands in the opposite order.
define void @f19(i64 *%ptr1, i64 *%ptr2) {
; CHECK-LABEL: f19:
; CHECK-NOT: nc
; CHECK: br %r14
  %val = load i64 *%ptr1, align 2
  %old = load i64 *%ptr2
  %and = and i64 %val, %old
  store i64 %and, i64 *%ptr2
  ret void
}

; ...and again with the other operand being unaligned.
define void @f20(i64 *%ptr1, i64 *%ptr2) {
; CHECK-LABEL: f20:
; CHECK-NOT: nc
; CHECK: br %r14
  %val = load i64 *%ptr1
  %old = load i64 *%ptr2, align 2
  %and = and i64 %val, %old
  store i64 %and, i64 *%ptr2, align 2
  ret void
}

; Test a case where there is definite overlap.
define void @f21(i64 %base) {
; CHECK-LABEL: f21:
; CHECK-NOT: nc
; CHECK: br %r14
  %add = add i64 %base, 1
  %ptr1 = inttoptr i64 %base to i64 *
  %ptr2 = inttoptr i64 %add to i64 *
  %val = load i64 *%ptr1
  %old = load i64 *%ptr2, align 1
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2, align 1
  ret void
}

; Test that we can use NC for global addresses for i8.
define void @f22(i8 *%ptr) {
; CHECK-LABEL: f22:
; CHECK: larl [[REG:%r[0-5]]], g1
; CHECK: nc 0(1,%r2), 0([[REG]])
; CHECK: br %r14
  %val = load i8 *@g1
  %old = load i8 *%ptr
  %and = and i8 %val, %old
  store i8 %and, i8 *%ptr
  ret void
}

; ...and again with the global on the store.
define void @f23(i8 *%ptr) {
; CHECK-LABEL: f23:
; CHECK: larl [[REG:%r[0-5]]], g1
; CHECK: nc 0(1,[[REG]]), 0(%r2)
; CHECK: br %r14
  %val = load i8 *%ptr
  %old = load i8 *@g1
  %and = and i8 %val, %old
  store i8 %and, i8 *@g1
  ret void
}

; Test that we use NC even where LHRL and STHRL are available.
define void @f24(i16 *%ptr) {
; CHECK-LABEL: f24:
; CHECK: larl [[REG:%r[0-5]]], g2
; CHECK: nc 0(2,%r2), 0([[REG]])
; CHECK: br %r14
  %val = load i16 *@g2
  %old = load i16 *%ptr
  %and = and i16 %val, %old
  store i16 %and, i16 *%ptr
  ret void
}

; ...likewise on the other side.
define void @f25(i16 *%ptr) {
; CHECK-LABEL: f25:
; CHECK: larl [[REG:%r[0-5]]], g2
; CHECK: nc 0(2,[[REG]]), 0(%r2)
; CHECK: br %r14
  %val = load i16 *%ptr
  %old = load i16 *@g2
  %and = and i16 %val, %old
  store i16 %and, i16 *@g2
  ret void
}

; Test a case where offset disambiguation is enough.
define void @f26(i64 *%ptr1) {
; CHECK-LABEL: f26:
; CHECK: nc 8(8,%r2), 0(%r2)
; CHECK: br %r14
  %ptr2 = getelementptr i64 *%ptr1, i64 1
  %val = load i64 *%ptr1, align 1
  %old = load i64 *%ptr2, align 1
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2, align 1
  ret void
}

; Test a case where TBAA tells us there is no alias.
define void @f27(i64 *%ptr1, i64 *%ptr2) {
; CHECK-LABEL: f27:
; CHECK: nc 0(8,%r3), 0(%r2)
; CHECK: br %r14
  %val = load i64 *%ptr1, align 2, !tbaa !1
  %old = load i64 *%ptr2, align 2, !tbaa !2
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2, align 2, !tbaa !2
  ret void
}

; Test a case where TBAA information is present but doesn't help.
define void @f28(i64 *%ptr1, i64 *%ptr2) {
; CHECK-LABEL: f28:
; CHECK-NOT: nc
; CHECK: br %r14
  %val = load i64 *%ptr1, align 2, !tbaa !1
  %old = load i64 *%ptr2, align 2, !tbaa !1
  %and = and i64 %old, %val
  store i64 %and, i64 *%ptr2, align 2, !tbaa !1
  ret void
}

!0 = metadata !{ metadata !"root" }
!1 = metadata !{ metadata !"set1", metadata !0 }
!2 = metadata !{ metadata !"set2", metadata !0 }
