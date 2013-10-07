; RUN: llc < %s -march=sparc -mattr=hard-quad-float | FileCheck %s --check-prefix=HARD
; RUN: llc < %s -march=sparc -mattr=-hard-quad-float | FileCheck %s --check-prefix=SOFT


; HARD-LABEL: f128_ops
; HARD:       ldd
; HARD:       ldd
; HARD:       ldd
; HARD:       ldd
; HARD:       faddq [[R0:.+]],  [[R1:.+]],  [[R2:.+]]
; HARD:       fsubq [[R2]], [[R3:.+]], [[R4:.+]]
; HARD:       fmulq [[R4]], [[R5:.+]], [[R6:.+]]
; HARD:       fdivq [[R6]], [[R2]]
; HARD:       std
; HARD:       std

; SOFT-LABEL: f128_ops
; SOFT:       ldd
; SOFT:       ldd
; SOFT:       ldd
; SOFT:       ldd
; SOFT:       call _Q_add
; SOFT:       call _Q_sub
; SOFT:       call _Q_mul
; SOFT:       call _Q_div
; SOFT:       std
; SOFT:       std

define void @f128_ops(fp128* noalias sret %scalar.result, fp128* byval %a, fp128* byval %b, fp128* byval %c, fp128* byval %d) {
entry:
  %0 = load fp128* %a, align 8
  %1 = load fp128* %b, align 8
  %2 = load fp128* %c, align 8
  %3 = load fp128* %d, align 8
  %4 = fadd fp128 %0, %1
  %5 = fsub fp128 %4, %2
  %6 = fmul fp128 %5, %3
  %7 = fdiv fp128 %6, %4
  store fp128 %7, fp128* %scalar.result, align 8
  ret void
}

; HARD-LABEL: f128_spill
; HARD:       std %f{{.+}}, [%[[S0:.+]]]
; HARD:       std %f{{.+}}, [%[[S1:.+]]]
; HARD-DAG:   ldd [%[[S0]]], %f{{.+}}
; HARD-DAG:   ldd [%[[S1]]], %f{{.+}}
; HARD:       jmp

; SOFT-LABEL: f128_spill
; SOFT:       std %f{{.+}}, [%[[S0:.+]]]
; SOFT:       std %f{{.+}}, [%[[S1:.+]]]
; SOFT-DAG:   ldd [%[[S0]]], %f{{.+}}
; SOFT-DAG:   ldd [%[[S1]]], %f{{.+}}
; SOFT:       jmp

define void @f128_spill(fp128* noalias sret %scalar.result, fp128* byval %a) {
entry:
  %0 = load fp128* %a, align 8
  call void asm sideeffect "", "~{f0},~{f1},~{f2},~{f3},~{f4},~{f5},~{f6},~{f7},~{f8},~{f9},~{f10},~{f11},~{f12},~{f13},~{f14},~{f15},~{f16},~{f17},~{f18},~{f19},~{f20},~{f21},~{f22},~{f23},~{f24},~{f25},~{f26},~{f27},~{f28},~{f29},~{f30},~{f31}"()
  store fp128 %0, fp128* %scalar.result, align 8
  ret void
}

; HARD-LABEL: f128_compare
; HARD:       fcmpq
; HARD-NEXT:  nop

; SOFT-LABEL: f128_compare
; SOFT:       _Q_cmp

define i32 @f128_compare(fp128* byval %f0, fp128* byval %f1, i32 %a, i32 %b) {
entry:
   %0 = load fp128* %f0, align 8
   %1 = load fp128* %f1, align 8
   %cond = fcmp ult fp128 %0, %1
   %ret = select i1 %cond, i32 %a, i32 %b
   ret i32 %ret
}

; HARD-LABEL: f128_compare2
; HARD:       fcmpq
; HARD:       fb{{ule|g}}

; SOFT-LABEL: f128_compare2
; SOFT:       _Q_cmp
; SOFT:       cmp

define i32 @f128_compare2() {
entry:
  %0 = fcmp ogt fp128 undef, 0xL00000000000000000000000000000000
  br i1 %0, label %"5", label %"7"

"5":                                              ; preds = %entry
  ret i32 0

"7":                                              ; preds = %entry
  ret i32 1
}


; HARD-LABEL: f128_abs
; HARD:       fabss

; SOFT-LABEL: f128_abs
; SOFT:       fabss

define void @f128_abs(fp128* noalias sret %scalar.result, fp128* byval %a) {
entry:
  %0 = load fp128* %a, align 8
  %1 = tail call fp128 @llvm.fabs.f128(fp128 %0)
  store fp128 %1, fp128* %scalar.result, align 8
  ret void
}

declare fp128 @llvm.fabs.f128(fp128) nounwind readonly

; HARD-LABEL: int_to_f128
; HARD:       fitoq

; SOFT-LABEL: int_to_f128
; SOFT:       _Q_itoq

define void @int_to_f128(fp128* noalias sret %scalar.result, i32 %i) {
entry:
  %0 = sitofp i32 %i to fp128
  store fp128 %0, fp128* %scalar.result, align 8
  ret void
}

; HARD-LABEL: fp128_unaligned
; HARD:       ldub
; HARD:       faddq
; HARD:       stb
; HARD:       jmp

; SOFT-LABEL: fp128_unaligned
; SOFT:       ldub
; SOFT:       call _Q_add
; SOFT:       stb
; SOFT:       jmp

define void @fp128_unaligned(fp128* %a, fp128* %b, fp128* %c) {
entry:
  %0 = load fp128* %a, align 1
  %1 = load fp128* %b, align 1
  %2 = fadd fp128 %0, %1
  store fp128 %2, fp128* %c, align 1
  ret void
}
