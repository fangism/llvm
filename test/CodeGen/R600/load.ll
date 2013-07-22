; RUN: llc < %s -march=r600 -mcpu=redwood | FileCheck --check-prefix=R600-CHECK %s
; RUN: llc < %s -march=r600 -mcpu=cayman | FileCheck --check-prefix=R600-CHECK %s
; RUN: llc < %s -march=r600 -mcpu=SI | FileCheck --check-prefix=SI-CHECK  %s

; Load an i8 value from the global address space.
; R600-CHECK: @load_i8
; R600-CHECK: VTX_READ_8 T{{[0-9]+\.X, T[0-9]+\.X}}

; SI-CHECK: @load_i8
; SI-CHECK: BUFFER_LOAD_UBYTE VGPR{{[0-9]+}},
define void @load_i8(i32 addrspace(1)* %out, i8 addrspace(1)* %in) {
  %1 = load i8 addrspace(1)* %in
  %2 = zext i8 %1 to i32
  store i32 %2, i32 addrspace(1)* %out
  ret void
}

; load an i32 value from the global address space.
; R600-CHECK: @load_i32
; R600-CHECK: VTX_READ_32 T{{[0-9]+}}.X, T{{[0-9]+}}.X, 0

; SI-CHECK: @load_i32
; SI-CHECK: BUFFER_LOAD_DWORD VGPR{{[0-9]+}}
define void @load_i32(i32 addrspace(1)* %out, i32 addrspace(1)* %in) {
entry:
  %0 = load i32 addrspace(1)* %in
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; load a f32 value from the global address space.
; R600-CHECK: @load_f32
; R600-CHECK: VTX_READ_32 T{{[0-9]+}}.X, T{{[0-9]+}}.X, 0

; SI-CHECK: @load_f32
; SI-CHECK: BUFFER_LOAD_DWORD VGPR{{[0-9]+}}
define void @load_f32(float addrspace(1)* %out, float addrspace(1)* %in) {
entry:
  %0 = load float addrspace(1)* %in
  store float %0, float addrspace(1)* %out
  ret void
}

; load a v2f32 value from the global address space
; R600-CHECK: @load_v2f32
; R600-CHECK: VTX_READ_32
; R600-CHECK: VTX_READ_32

; SI-CHECK: @load_v2f32
; SI-CHECK: BUFFER_LOAD_DWORDX2
define void @load_v2f32(<2 x float> addrspace(1)* %out, <2 x float> addrspace(1)* %in) {
entry:
  %0 = load <2 x float> addrspace(1)* %in
  store <2 x float> %0, <2 x float> addrspace(1)* %out
  ret void
}

; Load an i32 value from the constant address space.
; R600-CHECK: @load_const_addrspace_i32
; R600-CHECK: VTX_READ_32 T{{[0-9]+}}.X, T{{[0-9]+}}.X, 0

; SI-CHECK: @load_const_addrspace_i32
; SI-CHECK: S_LOAD_DWORD SGPR{{[0-9]+}}
define void @load_const_addrspace_i32(i32 addrspace(1)* %out, i32 addrspace(2)* %in) {
entry:
  %0 = load i32 addrspace(2)* %in
  store i32 %0, i32 addrspace(1)* %out
  ret void
}

; Load a f32 value from the constant address space.
; R600-CHECK: @load_const_addrspace_f32
; R600-CHECK: VTX_READ_32 T{{[0-9]+}}.X, T{{[0-9]+}}.X, 0

; SI-CHECK: @load_const_addrspace_f32
; SI-CHECK: S_LOAD_DWORD SGPR{{[0-9]+}}
define void @load_const_addrspace_f32(float addrspace(1)* %out, float addrspace(2)* %in) {
  %1 = load float addrspace(2)* %in
  store float %1, float addrspace(1)* %out
  ret void
}

; R600-CHECK: @load_i64
; R600-CHECK: RAT
; R600-CHECK: RAT

; SI-CHECK: @load_i64
; SI-CHECK: BUFFER_LOAD_DWORDX2
define void @load_i64(i64 addrspace(1)* %out, i64 addrspace(1)* %in) {
entry:
  %0 = load i64 addrspace(1)* %in
  store i64 %0, i64 addrspace(1)* %out
  ret void
}

; R600-CHECK: @load_i64_sext
; R600-CHECK: RAT
; R600-CHECK: RAT
; R600-CHECK: ASHR {{[* ]*}}T{{[0-9]\.[XYZW]}}, T{{[0-9]\.[XYZW]}},  literal.x
; R600-CHECK: 31
; SI-CHECK: @load_i64_sext
; SI-CHECK: BUFFER_LOAD_DWORDX2 [[VAL:VGPR[0-9]_VGPR[0-9]]]
; SI-CHECK: V_LSHL_B64 [[LSHL:VGPR[0-9]_VGPR[0-9]]], [[VAL]], 32
; SI-CHECK: V_ASHR_I64 VGPR{{[0-9]}}_VGPR{{[0-9]}}, [[LSHL]], 32

define void @load_i64_sext(i64 addrspace(1)* %out, i32 addrspace(1)* %in) {
entry:
  %0 = load i32 addrspace(1)* %in
  %1 = sext i32 %0 to i64
  store i64 %1, i64 addrspace(1)* %out
  ret void
}

; R600-CHECK: @load_i64_zext
; R600-CHECK: RAT
; R600-CHECK: RAT
define void @load_i64_zext(i64 addrspace(1)* %out, i32 addrspace(1)* %in) {
entry:
  %0 = load i32 addrspace(1)* %in
  %1 = zext i32 %0 to i64
  store i64 %1, i64 addrspace(1)* %out
  ret void
}
