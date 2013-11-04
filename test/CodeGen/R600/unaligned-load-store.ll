; RUN: llc -march=r600 -mcpu=SI < %s | FileCheck -check-prefix=SI %s

; SI-LABEL: @unaligned_load_store_i32:
; SI: V_ADD_I32_e64 [[REG:VGPR[0-9]+]]
; DS_READ_U8 {{VGPR[0-9]+}}, 0, [[REG]]
define void @unaligned_load_store_i32(i32 addrspace(3)* %p, i32 addrspace(3)* %r) nounwind {
  %v = load i32 addrspace(3)* %p, align 1
  store i32 %v, i32 addrspace(3)* %r, align 1
  ret void
}

; SI-LABEL: @unaligned_load_store_v4i32:
; SI: V_ADD_I32_e64 [[REG:VGPR[0-9]+]]
; DS_READ_U8 {{VGPR[0-9]+}}, 0, [[REG]]
define void @unaligned_load_store_v4i32(<4 x i32> addrspace(3)* %p, <4 x i32> addrspace(3)* %r) nounwind {
  %v = load <4 x i32> addrspace(3)* %p, align 1
  store <4 x i32> %v, <4 x i32> addrspace(3)* %r, align 1
  ret void
}
