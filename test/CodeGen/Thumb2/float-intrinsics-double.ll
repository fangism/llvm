; RUN: llc < %s -mtriple=thumbv7-none-eabi   -mcpu=cortex-m3 | FileCheck %s -check-prefix=CHECK -check-prefix=SOFT -check-prefix=NONE
; RUN: llc < %s -mtriple=thumbv7-none-eabihf -mcpu=cortex-m4 | FileCheck %s -check-prefix=CHECK -check-prefix=SOFT -check-prefix=SP
; RUN: llc < %s -mtriple=thumbv7-none-eabihf -mcpu=cortex-a7 | FileCheck %s -check-prefix=CHECK -check-prefix=HARD -check-prefix=DP

declare double     @llvm.sqrt.f64(double %Val)
define double @sqrt_d(double %a) {
; CHECK-LABEL: sqrt_d:
; SOFT: {{(bl|b)}} sqrt
; HARD: vsqrt.f64 d0, d0
  %1 = call double @llvm.sqrt.f64(double %a)
  ret double %1
}

declare double     @llvm.powi.f64(double %Val, i32 %power)
define double @powi_d(double %a, i32 %b) {
; CHECK-LABEL: powi_d:
; SOFT: {{(bl|b)}} __powidf2
; HARD: b __powidf2
  %1 = call double @llvm.powi.f64(double %a, i32 %b)
  ret double %1
}

declare double     @llvm.sin.f64(double %Val)
define double @sin_d(double %a) {
; CHECK-LABEL: sin_d:
; SOFT: {{(bl|b)}} sin
; HARD: b sin
  %1 = call double @llvm.sin.f64(double %a)
  ret double %1
}

declare double     @llvm.cos.f64(double %Val)
define double @cos_d(double %a) {
; CHECK-LABEL: cos_d:
; SOFT: {{(bl|b)}} cos
; HARD: b cos
  %1 = call double @llvm.cos.f64(double %a)
  ret double %1
}

declare double     @llvm.pow.f64(double %Val, double %power)
define double @pow_d(double %a, double %b) {
; CHECK-LABEL: pow_d:
; SOFT: {{(bl|b)}} pow
; HARD: b pow
  %1 = call double @llvm.pow.f64(double %a, double %b)
  ret double %1
}

declare double     @llvm.exp.f64(double %Val)
define double @exp_d(double %a) {
; CHECK-LABEL: exp_d:
; SOFT: {{(bl|b)}} exp
; HARD: b exp
  %1 = call double @llvm.exp.f64(double %a)
  ret double %1
}

declare double     @llvm.exp2.f64(double %Val)
define double @exp2_d(double %a) {
; CHECK-LABEL: exp2_d:
; SOFT: {{(bl|b)}} exp2
; HARD: b exp2
  %1 = call double @llvm.exp2.f64(double %a)
  ret double %1
}

declare double     @llvm.log.f64(double %Val)
define double @log_d(double %a) {
; CHECK-LABEL: log_d:
; SOFT: {{(bl|b)}} log
; HARD: b log
  %1 = call double @llvm.log.f64(double %a)
  ret double %1
}

declare double     @llvm.log10.f64(double %Val)
define double @log10_d(double %a) {
; CHECK-LABEL: log10_d:
; SOFT: {{(bl|b)}} log10
; HARD: b log10
  %1 = call double @llvm.log10.f64(double %a)
  ret double %1
}

declare double     @llvm.log2.f64(double %Val)
define double @log2_d(double %a) {
; CHECK-LABEL: log2_d:
; SOFT: {{(bl|b)}} log2
; HARD: b log2
  %1 = call double @llvm.log2.f64(double %a)
  ret double %1
}

declare double     @llvm.fma.f64(double %a, double %b, double %c)
define double @fma_d(double %a, double %b, double %c) {
; CHECK-LABEL: fma_d:
; SOFT: {{(bl|b)}} fma
; HARD: vfma.f64
  %1 = call double @llvm.fma.f64(double %a, double %b, double %c)
  ret double %1
}

; FIXME: the FPv4-SP version is less efficient than the no-FPU version
declare double     @llvm.fabs.f64(double %Val)
define double @abs_d(double %a) {
; CHECK-LABEL: abs_d:
; NONE: bic r1, r1, #-2147483648
; SP: bl __aeabi_dcmpgt
; SP: bl __aeabi_dcmpun
; SP: bl __aeabi_dsub
; DP: vabs.f64 d0, d0
  %1 = call double @llvm.fabs.f64(double %a)
  ret double %1
}

declare double     @llvm.copysign.f64(double  %Mag, double  %Sgn)
define double @copysign_d(double %a, double %b) {
; CHECK-LABEL: copysign_d:
; SOFT: lsrs [[REG:r[0-9]+]], r3, #31
; SOFT: bfi r1, [[REG]], #31, #1
; HARD: vmov.i32 [[REG:d[0-9]+]], #0x80000000
; HARD: vshl.i64 [[REG]], [[REG]], #32
; HARD: vbsl [[REG]], d
  %1 = call double @llvm.copysign.f64(double %a, double %b)
  ret double %1
}

declare double     @llvm.floor.f64(double %Val)
define double @floor_d(double %a) {
; CHECK-LABEL: floor_d:
; SOFT: {{(bl|b)}} floor
; HARD: b floor
  %1 = call double @llvm.floor.f64(double %a)
  ret double %1
}

declare double     @llvm.ceil.f64(double %Val)
define double @ceil_d(double %a) {
; CHECK-LABEL: ceil_d:
; SOFT: {{(bl|b)}} ceil
; HARD: b ceil
  %1 = call double @llvm.ceil.f64(double %a)
  ret double %1
}

declare double     @llvm.trunc.f64(double %Val)
define double @trunc_d(double %a) {
; CHECK-LABEL: trunc_d:
; SOFT: {{(bl|b)}} trunc
; HARD: b trunc
  %1 = call double @llvm.trunc.f64(double %a)
  ret double %1
}

declare double     @llvm.rint.f64(double %Val)
define double @rint_d(double %a) {
; CHECK-LABEL: rint_d:
; SOFT: {{(bl|b)}} rint
; HARD: b rint
  %1 = call double @llvm.rint.f64(double %a)
  ret double %1
}

declare double     @llvm.nearbyint.f64(double %Val)
define double @nearbyint_d(double %a) {
; CHECK-LABEL: nearbyint_d:
; SOFT: {{(bl|b)}} nearbyint
; HARD: b nearbyint
  %1 = call double @llvm.nearbyint.f64(double %a)
  ret double %1
}

declare double     @llvm.round.f64(double %Val)
define double @round_d(double %a) {
; CHECK-LABEL: round_d:
; SOFT: {{(bl|b)}} round
; HARD: b round
  %1 = call double @llvm.round.f64(double %a)
  ret double %1
}

declare double     @llvm.fmuladd.f64(double %a, double %b, double %c)
define double @fmuladd_d(double %a, double %b, double %c) {
; CHECK-LABEL: fmuladd_d:
; SOFT: bl __aeabi_dmul
; SOFT: bl __aeabi_dadd
; HARD: vmul.f64
; HARD: vadd.f64
  %1 = call double @llvm.fmuladd.f64(double %a, double %b, double %c)
  ret double %1
}

declare i16 @llvm.convert.to.fp16.f64(double %a)
define i16 @d_to_h(double %a) {
; CHECK-LABEL: d_to_h:
; SOFT: bl __aeabi_d2h
; HARD: bl __aeabi_d2h
  %1 = call i16 @llvm.convert.to.fp16.f64(double %a)
  ret i16 %1
}

declare double @llvm.convert.from.fp16.f64(i16 %a)
define double @h_to_d(i16 %a) {
; CHECK-LABEL: h_to_d:
; NONE: bl __gnu_h2f_ieee
; NONE: bl __aeabi_f2d
; SP: vcvtb.f32.f16
; SP: bl __aeabi_f2d
; DP: vcvtb.f32.f16
; DP: vcvt.f64.f32
  %1 = call double @llvm.convert.from.fp16.f64(i16 %a)
  ret double %1
}
