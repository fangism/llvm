; RUN: llc < %s -march=arm -mattr=+neon,+fp16 | FileCheck %s

define <2 x i32> @vcvt_f32tos32(<2 x float>* %A) nounwind {
;CHECK: vcvt_f32tos32:
;CHECK: vcvt.s32.f32
	%tmp1 = load <2 x float>* %A
	%tmp2 = fptosi <2 x float> %tmp1 to <2 x i32>
	ret <2 x i32> %tmp2
}

define <2 x i32> @vcvt_f32tou32(<2 x float>* %A) nounwind {
;CHECK: vcvt_f32tou32:
;CHECK: vcvt.u32.f32
	%tmp1 = load <2 x float>* %A
	%tmp2 = fptoui <2 x float> %tmp1 to <2 x i32>
	ret <2 x i32> %tmp2
}

define <2 x float> @vcvt_s32tof32(<2 x i32>* %A) nounwind {
;CHECK: vcvt_s32tof32:
;CHECK: vcvt.f32.s32
	%tmp1 = load <2 x i32>* %A
	%tmp2 = sitofp <2 x i32> %tmp1 to <2 x float>
	ret <2 x float> %tmp2
}

define <2 x float> @vcvt_u32tof32(<2 x i32>* %A) nounwind {
;CHECK: vcvt_u32tof32:
;CHECK: vcvt.f32.u32
	%tmp1 = load <2 x i32>* %A
	%tmp2 = uitofp <2 x i32> %tmp1 to <2 x float>
	ret <2 x float> %tmp2
}

define <4 x i32> @vcvtQ_f32tos32(<4 x float>* %A) nounwind {
;CHECK: vcvtQ_f32tos32:
;CHECK: vcvt.s32.f32
	%tmp1 = load <4 x float>* %A
	%tmp2 = fptosi <4 x float> %tmp1 to <4 x i32>
	ret <4 x i32> %tmp2
}

define <4 x i32> @vcvtQ_f32tou32(<4 x float>* %A) nounwind {
;CHECK: vcvtQ_f32tou32:
;CHECK: vcvt.u32.f32
	%tmp1 = load <4 x float>* %A
	%tmp2 = fptoui <4 x float> %tmp1 to <4 x i32>
	ret <4 x i32> %tmp2
}

define <4 x float> @vcvtQ_s32tof32(<4 x i32>* %A) nounwind {
;CHECK: vcvtQ_s32tof32:
;CHECK: vcvt.f32.s32
	%tmp1 = load <4 x i32>* %A
	%tmp2 = sitofp <4 x i32> %tmp1 to <4 x float>
	ret <4 x float> %tmp2
}

define <4 x float> @vcvtQ_u32tof32(<4 x i32>* %A) nounwind {
;CHECK: vcvtQ_u32tof32:
;CHECK: vcvt.f32.u32
	%tmp1 = load <4 x i32>* %A
	%tmp2 = uitofp <4 x i32> %tmp1 to <4 x float>
	ret <4 x float> %tmp2
}

define <2 x i32> @vcvt_n_f32tos32(<2 x float>* %A) nounwind {
;CHECK: vcvt_n_f32tos32:
;CHECK: vcvt.s32.f32
	%tmp1 = load <2 x float>* %A
	%tmp2 = call <2 x i32> @llvm.arm.neon.vcvtfp2fxs.v2i32.v2f32(<2 x float> %tmp1, i32 1)
	ret <2 x i32> %tmp2
}

define <2 x i32> @vcvt_n_f32tou32(<2 x float>* %A) nounwind {
;CHECK: vcvt_n_f32tou32:
;CHECK: vcvt.u32.f32
	%tmp1 = load <2 x float>* %A
	%tmp2 = call <2 x i32> @llvm.arm.neon.vcvtfp2fxu.v2i32.v2f32(<2 x float> %tmp1, i32 1)
	ret <2 x i32> %tmp2
}

define <2 x float> @vcvt_n_s32tof32(<2 x i32>* %A) nounwind {
;CHECK: vcvt_n_s32tof32:
;CHECK: vcvt.f32.s32
	%tmp1 = load <2 x i32>* %A
	%tmp2 = call <2 x float> @llvm.arm.neon.vcvtfxs2fp.v2f32.v2i32(<2 x i32> %tmp1, i32 1)
	ret <2 x float> %tmp2
}

define <2 x float> @vcvt_n_u32tof32(<2 x i32>* %A) nounwind {
;CHECK: vcvt_n_u32tof32:
;CHECK: vcvt.f32.u32
	%tmp1 = load <2 x i32>* %A
	%tmp2 = call <2 x float> @llvm.arm.neon.vcvtfxu2fp.v2f32.v2i32(<2 x i32> %tmp1, i32 1)
	ret <2 x float> %tmp2
}

declare <2 x i32> @llvm.arm.neon.vcvtfp2fxs.v2i32.v2f32(<2 x float>, i32) nounwind readnone
declare <2 x i32> @llvm.arm.neon.vcvtfp2fxu.v2i32.v2f32(<2 x float>, i32) nounwind readnone
declare <2 x float> @llvm.arm.neon.vcvtfxs2fp.v2f32.v2i32(<2 x i32>, i32) nounwind readnone
declare <2 x float> @llvm.arm.neon.vcvtfxu2fp.v2f32.v2i32(<2 x i32>, i32) nounwind readnone

define <4 x i32> @vcvtQ_n_f32tos32(<4 x float>* %A) nounwind {
;CHECK: vcvtQ_n_f32tos32:
;CHECK: vcvt.s32.f32
	%tmp1 = load <4 x float>* %A
	%tmp2 = call <4 x i32> @llvm.arm.neon.vcvtfp2fxs.v4i32.v4f32(<4 x float> %tmp1, i32 1)
	ret <4 x i32> %tmp2
}

define <4 x i32> @vcvtQ_n_f32tou32(<4 x float>* %A) nounwind {
;CHECK: vcvtQ_n_f32tou32:
;CHECK: vcvt.u32.f32
	%tmp1 = load <4 x float>* %A
	%tmp2 = call <4 x i32> @llvm.arm.neon.vcvtfp2fxu.v4i32.v4f32(<4 x float> %tmp1, i32 1)
	ret <4 x i32> %tmp2
}

define <4 x float> @vcvtQ_n_s32tof32(<4 x i32>* %A) nounwind {
;CHECK: vcvtQ_n_s32tof32:
;CHECK: vcvt.f32.s32
	%tmp1 = load <4 x i32>* %A
	%tmp2 = call <4 x float> @llvm.arm.neon.vcvtfxs2fp.v4f32.v4i32(<4 x i32> %tmp1, i32 1)
	ret <4 x float> %tmp2
}

define <4 x float> @vcvtQ_n_u32tof32(<4 x i32>* %A) nounwind {
;CHECK: vcvtQ_n_u32tof32:
;CHECK: vcvt.f32.u32
	%tmp1 = load <4 x i32>* %A
	%tmp2 = call <4 x float> @llvm.arm.neon.vcvtfxu2fp.v4f32.v4i32(<4 x i32> %tmp1, i32 1)
	ret <4 x float> %tmp2
}

declare <4 x i32> @llvm.arm.neon.vcvtfp2fxs.v4i32.v4f32(<4 x float>, i32) nounwind readnone
declare <4 x i32> @llvm.arm.neon.vcvtfp2fxu.v4i32.v4f32(<4 x float>, i32) nounwind readnone
declare <4 x float> @llvm.arm.neon.vcvtfxs2fp.v4f32.v4i32(<4 x i32>, i32) nounwind readnone
declare <4 x float> @llvm.arm.neon.vcvtfxu2fp.v4f32.v4i32(<4 x i32>, i32) nounwind readnone

define <4 x float> @vcvt_f16tof32(<4 x i16>* %A) nounwind {
;CHECK: vcvt_f16tof32:
;CHECK: vcvt.f32.f16
	%tmp1 = load <4 x i16>* %A
	%tmp2 = call <4 x float> @llvm.arm.neon.vcvthf2fp(<4 x i16> %tmp1)
	ret <4 x float> %tmp2
}

define <4 x i16> @vcvt_f32tof16(<4 x float>* %A) nounwind {
;CHECK: vcvt_f32tof16:
;CHECK: vcvt.f16.f32
	%tmp1 = load <4 x float>* %A
	%tmp2 = call <4 x i16> @llvm.arm.neon.vcvtfp2hf(<4 x float> %tmp1)
	ret <4 x i16> %tmp2
}

declare <4 x float> @llvm.arm.neon.vcvthf2fp(<4 x i16>) nounwind readnone
declare <4 x i16> @llvm.arm.neon.vcvtfp2hf(<4 x float>) nounwind readnone

; We currently estimate the cost of sext/zext/trunc v8(v16)i32 <-> v8(v16)i8
; instructions as expensive. If lowering is improved the cost model needs to
; change.
; RUN: opt < %s  -cost-model -analyze -mtriple=thumbv7-apple-ios6.0.0 -march=arm -mcpu=cortex-a8 | FileCheck %s --check-prefix=COST
%T0_5 = type <8 x i8>
%T1_5 = type <8 x i32>
; CHECK: func_cvt5:
define void @func_cvt5(%T0_5* %loadaddr, %T1_5* %storeaddr) {
; CHECK: vmovl.s8
; CHECK: vmovl.s16
; CHECK: vmovl.s16
  %v0 = load %T0_5* %loadaddr
; COST: func_cvt5
; COST: cost of 3 {{.*}} sext
  %r = sext %T0_5 %v0 to %T1_5
  store %T1_5 %r, %T1_5* %storeaddr
  ret void
}
;; We currently estimate the cost of this instruction as expensive. If lowering
;; is improved the cost needs to change.
%TA0_5 = type <8 x i8>
%TA1_5 = type <8 x i32>
; CHECK: func_cvt1:
define void @func_cvt1(%TA0_5* %loadaddr, %TA1_5* %storeaddr) {
; CHECK: vmovl.u8
; CHECK: vmovl.u16
; CHECK: vmovl.u16
  %v0 = load %TA0_5* %loadaddr
; COST: func_cvt1
; COST: cost of 3 {{.*}} zext
  %r = zext %TA0_5 %v0 to %TA1_5
  store %TA1_5 %r, %TA1_5* %storeaddr
  ret void
}
;; We currently estimate the cost of this instruction as expensive. If lowering
;; is improved the cost needs to change.
%T0_51 = type <8 x i32>
%T1_51 = type <8 x i8>
; CHECK: func_cvt51:
define void @func_cvt51(%T0_51* %loadaddr, %T1_51* %storeaddr) {
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
  %v0 = load %T0_51* %loadaddr
; COST: func_cvt51
; COST: cost of 19 {{.*}} trunc
  %r = trunc %T0_51 %v0 to %T1_51
  store %T1_51 %r, %T1_51* %storeaddr
  ret void
}
;; We currently estimate the cost of this instruction as expensive. If lowering
;; is improved the cost needs to change.
%TT0_5 = type <16 x i8>
%TT1_5 = type <16 x i32>
; CHECK: func_cvt52:
define void @func_cvt52(%TT0_5* %loadaddr, %TT1_5* %storeaddr) {
; CHECK: vmovl.s16
; CHECK: vmovl.s16
; CHECK: vmovl.s16
; CHECK: vmovl.s16
  %v0 = load %TT0_5* %loadaddr
; COST: func_cvt52
; COST: cost of 6 {{.*}} sext
  %r = sext %TT0_5 %v0 to %TT1_5
  store %TT1_5 %r, %TT1_5* %storeaddr
  ret void
}
;; We currently estimate the cost of this instruction as expensive. If lowering
;; is improved the cost needs to change.
%TTA0_5 = type <16 x i8>
%TTA1_5 = type <16 x i32>
; CHECK: func_cvt12:
define void @func_cvt12(%TTA0_5* %loadaddr, %TTA1_5* %storeaddr) {
; CHECK: vmovl.u16
; CHECK: vmovl.u16
; CHECK: vmovl.u16
; CHECK: vmovl.u16
  %v0 = load %TTA0_5* %loadaddr
; COST: func_cvt12
; COST: cost of 6 {{.*}} zext
  %r = zext %TTA0_5 %v0 to %TTA1_5
  store %TTA1_5 %r, %TTA1_5* %storeaddr
  ret void
}
;; We currently estimate the cost of this instruction as expensive. If lowering
;; is improved the cost needs to change.
%TT0_51 = type <16 x i32>
%TT1_51 = type <16 x i8>
; CHECK: func_cvt512:
define void @func_cvt512(%TT0_51* %loadaddr, %TT1_51* %storeaddr) {
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
; CHECK: strb
  %v0 = load %TT0_51* %loadaddr
; COST: func_cvt512
; COST: cost of 38 {{.*}} trunc
  %r = trunc %TT0_51 %v0 to %TT1_51
  store %TT1_51 %r, %TT1_51* %storeaddr
  ret void
}

; CHECK: sext_v4i16_v4i64:
define void @sext_v4i16_v4i64(<4 x i16>* %loadaddr, <4 x i64>* %storeaddr) {
; CHECK: vmovl.s32
; CHECK: vmovl.s32
  %v0 = load <4 x i16>* %loadaddr
; COST: sext_v4i16_v4i64
; COST: cost of 3 {{.*}} sext
  %r = sext <4 x i16> %v0 to <4 x i64>
  store <4 x i64> %r, <4 x i64>* %storeaddr
  ret void
}

; CHECK: zext_v4i16_v4i64:
define void @zext_v4i16_v4i64(<4 x i16>* %loadaddr, <4 x i64>* %storeaddr) {
; CHECK: vmovl.u32
; CHECK: vmovl.u32
  %v0 = load <4 x i16>* %loadaddr
; COST: zext_v4i16_v4i64
; COST: cost of 3 {{.*}} zext
  %r = zext <4 x i16> %v0 to <4 x i64>
  store <4 x i64> %r, <4 x i64>* %storeaddr
  ret void
}

; CHECK: sext_v8i16_v8i64:
define void @sext_v8i16_v8i64(<8 x i16>* %loadaddr, <8 x i64>* %storeaddr) {
; CHECK: vmovl.s32
; CHECK: vmovl.s32
; CHECK: vmovl.s32
; CHECK: vmovl.s32
  %v0 = load <8 x i16>* %loadaddr
; COST: sext_v8i16_v8i64
; COST: cost of 6 {{.*}} sext
  %r = sext <8 x i16> %v0 to <8 x i64>
  store <8 x i64> %r, <8 x i64>* %storeaddr
  ret void
}

; CHECK: zext_v8i16_v8i64:
define void @zext_v8i16_v8i64(<8 x i16>* %loadaddr, <8 x i64>* %storeaddr) {
; CHECK: vmovl.u32
; CHECK: vmovl.u32
; CHECK: vmovl.u32
; CHECK: vmovl.u32
  %v0 = load <8 x i16>* %loadaddr
; COST: zext_v8i16_v8i64
; COST: cost of 6 {{.*}} zext
  %r = zext <8 x i16> %v0 to <8 x i64>
  store <8 x i64> %r, <8 x i64>* %storeaddr
  ret void
}

