; RUN: llc < %s -march=r600 -mcpu=redwood | FileCheck %s --check-prefix=R600-CHECK --check-prefix=FUNC
; RUN: llc < %s -march=r600 -mcpu=SI | FileCheck %s --check-prefix=SI-CHECK --check-prefix=FUNC

; This test checks that uses and defs of the AR register happen in the same
; instruction clause.

; FUNC-LABEL: @mova_same_clause

; R600-CHECK: MOVA_INT
; R600-CHECK-NOT: ALU clause
; R600-CHECK: 0 + AR.x
; R600-CHECK: MOVA_INT
; R600-CHECK-NOT: ALU clause
; R600-CHECK: 0 + AR.x

; SI-CHECK: V_READFIRSTLANE
; SI-CHECK: V_MOVRELD
; SI-CHECK: S_CBRANCH
; SI-CHECK: V_READFIRSTLANE
; SI-CHECK: V_MOVRELD
; SI-CHECK: S_CBRANCH
define void @mova_same_clause(i32 addrspace(1)* nocapture %out, i32 addrspace(1)* nocapture %in) {
entry:
  %stack = alloca [5 x i32], align 4
  %0 = load i32 addrspace(1)* %in, align 4
  %arrayidx1 = getelementptr inbounds [5 x i32]* %stack, i32 0, i32 %0
  store i32 4, i32* %arrayidx1, align 4
  %arrayidx2 = getelementptr inbounds i32 addrspace(1)* %in, i32 1
  %1 = load i32 addrspace(1)* %arrayidx2, align 4
  %arrayidx3 = getelementptr inbounds [5 x i32]* %stack, i32 0, i32 %1
  store i32 5, i32* %arrayidx3, align 4
  %arrayidx10 = getelementptr inbounds [5 x i32]* %stack, i32 0, i32 0
  %2 = load i32* %arrayidx10, align 4
  store i32 %2, i32 addrspace(1)* %out, align 4
  %arrayidx12 = getelementptr inbounds [5 x i32]* %stack, i32 0, i32 1
  %3 = load i32* %arrayidx12
  %arrayidx13 = getelementptr inbounds i32 addrspace(1)* %out, i32 1
  store i32 %3, i32 addrspace(1)* %arrayidx13
  ret void
}

; This test checks that the stack offset is calculated correctly for structs.
; All register loads/stores should be optimized away, so there shouldn't be
; any MOVA instructions.
;
; XXX: This generated code has unnecessary MOVs, we should be able to optimize
; this.

; FUNC-LABEL: @multiple_structs
; R600-CHECK-NOT: MOVA_INT
; SI-CHECK-NOT: V_MOVREL
%struct.point = type { i32, i32 }

define void @multiple_structs(i32 addrspace(1)* %out) {
entry:
  %a = alloca %struct.point
  %b = alloca %struct.point
  %a.x.ptr = getelementptr %struct.point* %a, i32 0, i32 0
  %a.y.ptr = getelementptr %struct.point* %a, i32 0, i32 1
  %b.x.ptr = getelementptr %struct.point* %b, i32 0, i32 0
  %b.y.ptr = getelementptr %struct.point* %b, i32 0, i32 1
  store i32 0, i32* %a.x.ptr
  store i32 1, i32* %a.y.ptr
  store i32 2, i32* %b.x.ptr
  store i32 3, i32* %b.y.ptr
  %a.indirect.ptr = getelementptr %struct.point* %a, i32 0, i32 0
  %b.indirect.ptr = getelementptr %struct.point* %b, i32 0, i32 0
  %a.indirect = load i32* %a.indirect.ptr
  %b.indirect = load i32* %b.indirect.ptr
  %0 = add i32 %a.indirect, %b.indirect
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; Test direct access of a private array inside a loop.  The private array
; loads and stores should be lowered to copies, so there shouldn't be any
; MOVA instructions.

; FUNC-LABEL: @direct_loop
; R600-CHECK-NOT: MOVA_INT
; SI-CHECK-NOT: V_MOVREL

define void @direct_loop(i32 addrspace(1)* %out, i32 addrspace(1)* %in) {
entry:
  %prv_array_const = alloca [2 x i32]
  %prv_array = alloca [2 x i32]
  %a = load i32 addrspace(1)* %in
  %b_src_ptr = getelementptr i32 addrspace(1)* %in, i32 1
  %b = load i32 addrspace(1)* %b_src_ptr
  %a_dst_ptr = getelementptr [2 x i32]* %prv_array_const, i32 0, i32 0
  store i32 %a, i32* %a_dst_ptr
  %b_dst_ptr = getelementptr [2 x i32]* %prv_array_const, i32 0, i32 1
  store i32 %b, i32* %b_dst_ptr
  br label %for.body

for.body:
  %inc = phi i32 [0, %entry], [%count, %for.body]
  %x_ptr = getelementptr [2 x i32]* %prv_array_const, i32 0, i32 0
  %x = load i32* %x_ptr
  %y_ptr = getelementptr [2 x i32]* %prv_array, i32 0, i32 0
  %y = load i32* %y_ptr
  %xy = add i32 %x, %y
  store i32 %xy, i32* %y_ptr
  %count = add i32 %inc, 1
  %done = icmp eq i32 %count, 4095
  br i1 %done, label %for.end, label %for.body

for.end:
  %value_ptr = getelementptr [2 x i32]* %prv_array, i32 0, i32 0
  %value = load i32* %value_ptr
  store i32 %value, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: @short_array

; R600-CHECK: MOV {{\** *}}T{{[0-9]\.[XYZW]}}, literal
; R600-CHECK: 65536
; R600-CHECK: *
; R600-CHECK: MOVA_INT

; SI-CHECK: V_MOV_B32_e32 v{{[0-9]}}, 65536
; SI-CHECK: V_MOVRELS_B32_e32
define void @short_array(i32 addrspace(1)* %out, i32 %index) {
entry:
  %0 = alloca [2 x i16]
  %1 = getelementptr [2 x i16]* %0, i32 0, i32 0
  %2 = getelementptr [2 x i16]* %0, i32 0, i32 1
  store i16 0, i16* %1
  store i16 1, i16* %2
  %3 = getelementptr [2 x i16]* %0, i32 0, i32 %index
  %4 = load i16* %3
  %5 = sext i16 %4 to i32
  store i32 %5, i32 addrspace(1)* %out
  ret void
}

; FUNC-LABEL: @char_array

; R600-CHECK: OR_INT {{\** *}}T{{[0-9]\.[XYZW]}}, {{[PVT0-9]+\.[XYZW]}}, literal
; R600-CHECK: 256
; R600-CHECK: *
; R600-CHECK-NEXT: MOVA_INT

; SI-CHECK: V_OR_B32_e32 v{{[0-9]}}, 256
; SI-CHECK: V_MOVRELS_B32_e32
define void @char_array(i32 addrspace(1)* %out, i32 %index) {
entry:
  %0 = alloca [2 x i8]
  %1 = getelementptr [2 x i8]* %0, i32 0, i32 0
  %2 = getelementptr [2 x i8]* %0, i32 0, i32 1
  store i8 0, i8* %1
  store i8 1, i8* %2
  %3 = getelementptr [2 x i8]* %0, i32 0, i32 %index
  %4 = load i8* %3
  %5 = sext i8 %4 to i32
  store i32 %5, i32 addrspace(1)* %out
  ret void

}

; Make sure we don't overwrite workitem information with private memory

; FUNC-LABEL: @work_item_info
; R600-CHECK-NOT: MOV T0.X
; Additional check in case the move ends up in the last slot
; R600-CHECK-NOT: MOV * TO.X

; SI-CHECK-NOT: V_MOV_B32_e{{(32|64)}} v0
define void @work_item_info(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0 = alloca [2 x i32]
  %1 = getelementptr [2 x i32]* %0, i32 0, i32 0
  %2 = getelementptr [2 x i32]* %0, i32 0, i32 1
  store i32 0, i32* %1
  store i32 1, i32* %2
  %3 = getelementptr [2 x i32]* %0, i32 0, i32 %in
  %4 = load i32* %3
  %5 = call i32 @llvm.r600.read.tidig.x()
  %6 = add i32 %4, %5
  store i32 %6, i32 addrspace(1)* %out
  ret void
}

; Test that two stack objects are not stored in the same register
; The second stack object should be in T3.X
; FUNC-LABEL: @no_overlap
; R600-CHECK: MOV {{\** *}}T3.X
; SI-CHECK: V_MOV_B32_e32 v3
define void @no_overlap(i32 addrspace(1)* %out, i32 %in) {
entry:
  %0 = alloca [3 x i8], align 1
  %1 = alloca [2 x i8], align 1
  %2 = getelementptr [3 x i8]* %0, i32 0, i32 0
  %3 = getelementptr [3 x i8]* %0, i32 0, i32 1
  %4 = getelementptr [3 x i8]* %0, i32 0, i32 2
  %5 = getelementptr [2 x i8]* %1, i32 0, i32 0
  %6 = getelementptr [2 x i8]* %1, i32 0, i32 1
  store i8 0, i8* %2
  store i8 1, i8* %3
  store i8 2, i8* %4
  store i8 1, i8* %5
  store i8 0, i8* %6
  %7 = getelementptr [3 x i8]* %0, i32 0, i32 %in
  %8 = getelementptr [2 x i8]* %1, i32 0, i32 %in
  %9 = load i8* %7
  %10 = load i8* %8
  %11 = add i8 %9, %10
  %12 = sext i8 %11 to i32
  store i32 %12, i32 addrspace(1)* %out
  ret void
}



declare i32 @llvm.r600.read.tidig.x() nounwind readnone
