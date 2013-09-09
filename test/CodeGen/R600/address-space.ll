; RUN: llc -march=r600 -mcpu=SI < %s | FileCheck %s

; Test that codegenprepare understands address space sizes

%struct.foo = type { [3 x float], [3 x float] }

; CHECK-LABEL: @do_as_ptr_calcs:
; CHECK: V_ADD_I32_e64 {{VGPR[0-9]+}},
; CHECK: V_ADD_I32_e64 [[REG1:VGPR[0-9]+]],
; CHECK: DS_READ_B32 [[REG1]],
define void @do_as_ptr_calcs(%struct.foo addrspace(3)* nocapture %ptr) nounwind {
entry:
  %x = getelementptr inbounds %struct.foo addrspace(3)* %ptr, i32 0, i32 1, i32 0
  %y = getelementptr inbounds %struct.foo addrspace(3)* %ptr, i32 0, i32 1, i32 2
  br label %bb32

bb32:
  %a = load float addrspace(3)* %x, align 4
  %b = load float addrspace(3)* %y, align 4
  %cmp = fcmp one float %a, %b
  br i1 %cmp, label %bb34, label %bb33

bb33:
  unreachable

bb34:
  unreachable
}


