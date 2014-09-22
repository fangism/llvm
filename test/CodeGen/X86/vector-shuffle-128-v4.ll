; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -x86-experimental-vector-shuffle-lowering | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+sse3 -x86-experimental-vector-shuffle-lowering | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE3
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+ssse3 -x86-experimental-vector-shuffle-lowering | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSSE3
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+sse4.1 -x86-experimental-vector-shuffle-lowering | FileCheck %s --check-prefix=ALL --check-prefix=SSE --check-prefix=SSE41
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx -x86-experimental-vector-shuffle-lowering | FileCheck %s --check-prefix=ALL --check-prefix=AVX --check-prefix=AVX1

target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-unknown"

define <4 x i32> @shuffle_v4i32_0001(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0001
; ALL:         pshufd {{.*}} # xmm0 = xmm0[0,0,0,1]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0020(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0020
; ALL:         pshufd {{.*}} # xmm0 = xmm0[0,0,2,0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 0>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0112(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0112
; ALL:         pshufd {{.*}} # xmm0 = xmm0[0,1,1,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 1, i32 1, i32 2>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0300(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0300
; ALL:         pshufd {{.*}} # xmm0 = xmm0[0,3,0,0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 3, i32 0, i32 0>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_1000(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_1000
; ALL:         pshufd {{.*}} # xmm0 = xmm0[1,0,0,0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 1, i32 0, i32 0, i32 0>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_2200(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_2200
; ALL:         pshufd {{.*}} # xmm0 = xmm0[2,2,0,0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 2, i32 2, i32 0, i32 0>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_3330(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_3330
; ALL:         pshufd {{.*}} # xmm0 = xmm0[3,3,3,0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 3, i32 3, i32 3, i32 0>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_3210(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_3210
; ALL:         pshufd {{.*}} # xmm0 = xmm0[3,2,1,0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_2121(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_2121
; ALL:         pshufd {{.*}} # xmm0 = xmm0[2,1,2,1]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 2, i32 1, i32 2, i32 1>
  ret <4 x i32> %shuffle
}

define <4 x float> @shuffle_v4f32_0001(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_0001
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0,0,1]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_0001
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[0,0,0,1]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_0020(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_0020
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0,2,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_0020
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[0,0,2,0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 0>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_0300(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_0300
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,3,0,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_0300
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[0,3,0,0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 0, i32 3, i32 0, i32 0>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_1000(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_1000
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[1,0,0,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_1000
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[1,0,0,0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 1, i32 0, i32 0, i32 0>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_2200(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_2200
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,2,0,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_2200
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[2,2,0,0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 2, i32 2, i32 0, i32 0>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_3330(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_3330
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,3,3,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_3330
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[3,3,3,0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 3, i32 3, i32 3, i32 0>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_3210(<4 x float> %a, <4 x float> %b) {
; SSE-LABEL: @shuffle_v4f32_3210
; SSE:       # BB#0:
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,2,1,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_3210
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = xmm0[3,2,1,0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_0011(<4 x float> %a, <4 x float> %b) {
; ALL-LABEL: @shuffle_v4f32_0011
; ALL:         unpcklps {{.*}} # xmm0 = xmm0[0,0,1,1]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 0, i32 0, i32 1, i32 1>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_2233(<4 x float> %a, <4 x float> %b) {
; ALL-LABEL: @shuffle_v4f32_2233
; ALL:         unpckhps {{.*}} # xmm0 = xmm0[2,2,3,3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 2, i32 2, i32 3, i32 3>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_0022(<4 x float> %a, <4 x float> %b) {
; SSE2-LABEL: @shuffle_v4f32_0022
; SSE2:         shufps {{.*}} # xmm0 = xmm0[0,0,2,2]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_0022
; SSE3:         movsldup {{.*}} # xmm0 = xmm0[0,0,2,2]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_0022
; SSSE3:         movsldup {{.*}} # xmm0 = xmm0[0,0,2,2]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_0022
; SSE41:         movsldup {{.*}} # xmm0 = xmm0[0,0,2,2]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_0022
; AVX:         vmovsldup {{.*}} # xmm0 = xmm0[0,0,2,2]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
  ret <4 x float> %shuffle
}
define <4 x float> @shuffle_v4f32_1133(<4 x float> %a, <4 x float> %b) {
; SSE2-LABEL: @shuffle_v4f32_1133
; SSE2:         shufps {{.*}} # xmm0 = xmm0[1,1,3,3]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_1133
; SSE3:         movshdup {{.*}} # xmm0 = xmm0[1,1,3,3]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_1133
; SSSE3:         movshdup {{.*}} # xmm0 = xmm0[1,1,3,3]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_1133
; SSE41:         movshdup {{.*}} # xmm0 = xmm0[1,1,3,3]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_1133
; AVX:         vmovshdup {{.*}} # xmm0 = xmm0[1,1,3,3]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> %a, <4 x float> %b, <4 x i32> <i32 1, i32 1, i32 3, i32 3>
  ret <4 x float> %shuffle
}

define <4 x i32> @shuffle_v4i32_0124(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_0124
; SSE2:         shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[2,0]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,1],xmm1[2,0]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_0124
; SSE3:         shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[2,0]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,1],xmm1[2,0]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_0124
; SSSE3:         shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[2,0]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,1],xmm1[2,0]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_0124
; SSE41:         insertps {{.*}} # xmm0 = xmm0[0,1,2],xmm1[0]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_0124
; AVX:         vinsertps {{.*}} # xmm0 = xmm0[0,1,2],xmm1[0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 1, i32 2, i32 4>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0142(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0142
; ALL:         shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[2,0]
; ALL-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,1],xmm1[0,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 1, i32 4, i32 2>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0412(<4 x i32> %a, <4 x i32> %b) {
; SSE-LABEL: @shuffle_v4i32_0412
; SSE:         shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[0,0]
; SSE-NEXT:    shufps {{.*}} # xmm1 = xmm1[2,0],xmm0[1,2]
; SSE-NEXT:    movaps %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_0412
; AVX:         vshufps {{.*}} # xmm1 = xmm1[0,0],xmm0[0,0]
; AVX-NEXT:    vshufps {{.*}} # xmm0 = xmm1[2,0],xmm0[1,2]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 4, i32 1, i32 2>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_4012(<4 x i32> %a, <4 x i32> %b) {
; SSE-LABEL: @shuffle_v4i32_4012
; SSE:         shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[0,0]
; SSE-NEXT:    shufps {{.*}} # xmm1 = xmm1[0,2],xmm0[1,2]
; SSE-NEXT:    movaps %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_4012
; AVX:         vshufps {{.*}} # xmm1 = xmm1[0,0],xmm0[0,0]
; AVX-NEXT:    vshufps {{.*}} # xmm0 = xmm1[0,2],xmm0[1,2]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 4, i32 0, i32 1, i32 2>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0145(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0145
; ALL:         punpcklqdq {{.*}} # xmm0 = xmm0[0],xmm1[0]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 1, i32 4, i32 5>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_0451(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0451
; ALL:         shufps {{.*}} # xmm0 = xmm0[0,1],xmm1[0,1]
; ALL-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,2,3,1]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 4, i32 5, i32 1>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_4501(<4 x i32> %a, <4 x i32> %b) {
; SSE-LABEL: @shuffle_v4i32_4501
; SSE:         punpcklqdq {{.*}} # xmm1 = xmm1[0],xmm0[0]
; SSE-NEXT:    movdqa %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_4501
; AVX:         punpcklqdq {{.*}} # xmm0 = xmm1[0],xmm0[0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 4, i32 5, i32 0, i32 1>
  ret <4 x i32> %shuffle
}
define <4 x i32> @shuffle_v4i32_4015(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_4015
; ALL:         shufps {{.*}} # xmm0 = xmm0[0,1],xmm1[0,1]
; ALL-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0,1,3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 4, i32 0, i32 1, i32 5>
  ret <4 x i32> %shuffle
}

define <4 x float> @shuffle_v4f32_4zzz(<4 x float> %a) {
; SSE2-LABEL: @shuffle_v4f32_4zzz
; SSE2:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][1,0]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,2],[[X]][2,3]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_4zzz
; SSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][1,0]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,2],[[X]][2,3]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_4zzz
; SSSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][1,0]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,2],[[X]][2,3]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_4zzz
; SSE41:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE41-NEXT:    blendps {{.*}} # [[X]] = xmm0[0],[[X]][1,2,3]
; SSE41-NEXT:    movaps %[[X]], %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_4zzz
; AVX:         vxorps %[[X:xmm[0-9]+]], %[[X]]
; AVX-NEXT:    vblendps {{.*}} # xmm0 = xmm0[0],[[X]][1,2,3]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> zeroinitializer, <4 x float> %a, <4 x i32> <i32 4, i32 1, i32 2, i32 3>
  ret <4 x float> %shuffle
}

define <4 x float> @shuffle_v4f32_z4zz(<4 x float> %a) {
; SSE2-LABEL: @shuffle_v4f32_z4zz
; SSE2:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][2,0]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][3,0]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_z4zz
; SSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][2,0]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][3,0]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_z4zz
; SSSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][2,0]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][3,0]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_z4zz
; SSE41:         insertps {{.*}} # xmm0 = zero,xmm0[0],zero,zero
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_z4zz
; AVX:         vinsertps {{.*}} # xmm0 = zero,xmm0[0],zero,zero
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> zeroinitializer, <4 x float> %a, <4 x i32> <i32 2, i32 4, i32 3, i32 0>
  ret <4 x float> %shuffle
}

define <4 x float> @shuffle_v4f32_zz4z(<4 x float> %a) {
; SSE2-LABEL: @shuffle_v4f32_zz4z
; SSE2:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][0,0]
; SSE2-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,0],xmm0[0,2]
; SSE2-NEXT:    movaps %[[X]], %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_zz4z
; SSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][0,0]
; SSE3-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,0],xmm0[0,2]
; SSE3-NEXT:    movaps %[[X]], %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_zz4z
; SSSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],[[X]][0,0]
; SSSE3-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,0],xmm0[0,2]
; SSSE3-NEXT:    movaps %[[X]], %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_zz4z
; SSE41:         insertps {{.*}} # xmm0 = zero,zero,xmm0[0],zero
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_zz4z
; AVX:         vinsertps {{.*}} # xmm0 = zero,zero,xmm0[0],zero
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> zeroinitializer, <4 x float> %a, <4 x i32> <i32 0, i32 0, i32 4, i32 0>
  ret <4 x float> %shuffle
}

define <4 x float> @shuffle_v4f32_zuu4(<4 x float> %a) {
; SSE2-LABEL: @shuffle_v4f32_zuu4
; SSE2:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,1],xmm0[2,0]
; SSE2-NEXT:    movaps %[[X]], %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_zuu4
; SSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,1],xmm0[2,0]
; SSE3-NEXT:    movaps %[[X]], %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_zuu4
; SSSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,1],xmm0[2,0]
; SSSE3-NEXT:    movaps %[[X]], %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_zuu4
; SSE41:         insertps {{.*}} # xmm0 = zero,zero,zero,xmm0[0]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_zuu4
; AVX:         vinsertps {{.*}} # xmm0 = zero,zero,zero,xmm0[0]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> zeroinitializer, <4 x float> %a, <4 x i32> <i32 0, i32 undef, i32 undef, i32 4>
  ret <4 x float> %shuffle
}

define <4 x float> @shuffle_v4f32_zzz7(<4 x float> %a) {
; SSE2-LABEL: @shuffle_v4f32_zzz7
; SSE2:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,0],[[X]][2,0]
; SSE2-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,1],xmm0[2,0]
; SSE2-NEXT:    movaps %[[X]], %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_zzz7
; SSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,0],[[X]][2,0]
; SSE3-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,1],xmm0[2,0]
; SSE3-NEXT:    movaps %[[X]], %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_zzz7
; SSSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,0],[[X]][2,0]
; SSSE3-NEXT:    shufps {{.*}} # [[X]] = [[X]][0,1],xmm0[2,0]
; SSSE3-NEXT:    movaps %[[X]], %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_zzz7
; SSE41:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE41-NEXT:    blendps {{.*}} # [[X]] = [[X]][0,1,2],xmm0[3]
; SSE41-NEXT:    movaps %[[X]], %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_zzz7
; AVX:         vxorps %[[X:xmm[0-9]+]], %[[X]]
; AVX-NEXT:    vblendps {{.*}} # xmm0 = [[X]][0,1,2],xmm0[3]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> zeroinitializer, <4 x float> %a, <4 x i32> <i32 0, i32 1, i32 2, i32 7>
  ret <4 x float> %shuffle
}

define <4 x float> @shuffle_v4f32_z6zz(<4 x float> %a) {
; SSE2-LABEL: @shuffle_v4f32_z6zz
; SSE2:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][0,0]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][2,3]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4f32_z6zz
; SSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][0,0]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][2,3]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4f32_z6zz
; SSSE3:         xorps %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][0,0]
; SSSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[2,0],[[X]][2,3]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4f32_z6zz
; SSE41:         insertps {{.*}} # xmm0 = zero,xmm0[2],zero,zero
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4f32_z6zz
; AVX:         vinsertps {{.*}} # xmm0 = zero,xmm0[2],zero,zero
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x float> zeroinitializer, <4 x float> %a, <4 x i32> <i32 0, i32 6, i32 2, i32 3>
  ret <4 x float> %shuffle
}

define <4 x i32> @shuffle_v4i32_4zzz(i32 %i) {
; ALL-LABEL: @shuffle_v4i32_4zzz
; ALL:         movd {{.*}}, %xmm0
; ALL-NEXT:    retq
  %a = insertelement <4 x i32> undef, i32 %i, i32 0
  %shuffle = shufflevector <4 x i32> zeroinitializer, <4 x i32> %a, <4 x i32> <i32 4, i32 1, i32 2, i32 3>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_z4zz(i32 %i) {
; ALL-LABEL: @shuffle_v4i32_z4zz
; ALL:         movd {{.*}}, %xmm0
; ALL-NEXT:    pshufd {{.*}} # xmm0 = xmm0[1,0,1,1]
; ALL-NEXT:    retq
  %a = insertelement <4 x i32> undef, i32 %i, i32 0
  %shuffle = shufflevector <4 x i32> zeroinitializer, <4 x i32> %a, <4 x i32> <i32 2, i32 4, i32 3, i32 0>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_zz4z(i32 %i) {
; ALL-LABEL: @shuffle_v4i32_zz4z
; ALL:         movd {{.*}}, %xmm0
; ALL-NEXT:    pshufd {{.*}} # xmm0 = xmm0[1,1,0,1]
; ALL-NEXT:    retq
  %a = insertelement <4 x i32> undef, i32 %i, i32 0
  %shuffle = shufflevector <4 x i32> zeroinitializer, <4 x i32> %a, <4 x i32> <i32 0, i32 0, i32 4, i32 0>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_zuu4(i32 %i) {
; ALL-LABEL: @shuffle_v4i32_zuu4
; ALL:         movd {{.*}}, %xmm0
; ALL-NEXT:    pshufd {{.*}} # xmm0 = xmm0[1,1,1,0]
; ALL-NEXT:    retq
  %a = insertelement <4 x i32> undef, i32 %i, i32 0
  %shuffle = shufflevector <4 x i32> zeroinitializer, <4 x i32> %a, <4 x i32> <i32 0, i32 undef, i32 undef, i32 4>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_z6zz(i32 %i) {
; ALL-LABEL: @shuffle_v4i32_z6zz
; ALL:         movd {{.*}}, %xmm0
; ALL-NEXT:    pshufd {{.*}} # xmm0 = xmm0[1,0,1,1]
; ALL-NEXT:    retq
  %a = insertelement <4 x i32> undef, i32 %i, i32 2
  %shuffle = shufflevector <4 x i32> zeroinitializer, <4 x i32> %a, <4 x i32> <i32 0, i32 6, i32 2, i32 3>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_7012(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_7012
; SSE2:       # BB#0:
; SSE2-NEXT:    shufps {{.*}} # xmm1 = xmm1[3,0],xmm0[0,0]
; SSE2-NEXT:    shufps {{.*}} # xmm1 = xmm1[0,2],xmm0[1,2]
; SSE2-NEXT:    movaps %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_7012
; SSE3:       # BB#0:
; SSE3-NEXT:    shufps {{.*}} # xmm1 = xmm1[3,0],xmm0[0,0]
; SSE3-NEXT:    shufps {{.*}} # xmm1 = xmm1[0,2],xmm0[1,2]
; SSE3-NEXT:    movaps %xmm1, %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_7012
; SSSE3:       # BB#0:
; SSSE3-NEXT:    palignr $12, {{.*}} # xmm0 = xmm1[12,13,14,15],xmm0[0,1,2,3,4,5,6,7,8,9,10,11]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_7012
; SSE41:       # BB#0:
; SSE41-NEXT:    palignr $12, {{.*}} # xmm0 = xmm1[12,13,14,15],xmm0[0,1,2,3,4,5,6,7,8,9,10,11]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_7012
; AVX:       # BB#0:
; AVX-NEXT:    vpalignr $12, {{.*}} # xmm0 = xmm1[12,13,14,15],xmm0[0,1,2,3,4,5,6,7,8,9,10,11]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 7, i32 0, i32 1, i32 2>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_6701(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_6701
; SSE2:       # BB#0:
; SSE2-NEXT:    shufpd {{.*}} # xmm1 = xmm1[1],xmm0[0]
; SSE2-NEXT:    movapd %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_6701
; SSE3:       # BB#0:
; SSE3-NEXT:    shufpd {{.*}} # xmm1 = xmm1[1],xmm0[0]
; SSE3-NEXT:    movapd %xmm1, %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_6701
; SSSE3:       # BB#0:
; SSSE3-NEXT:    palignr $8, {{.*}} # xmm0 = xmm1[8,9,10,11,12,13,14,15],xmm0[0,1,2,3,4,5,6,7]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_6701
; SSE41:       # BB#0:
; SSE41-NEXT:    palignr $8, {{.*}} # xmm0 = xmm1[8,9,10,11,12,13,14,15],xmm0[0,1,2,3,4,5,6,7]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_6701
; AVX:       # BB#0:
; AVX-NEXT:    vpalignr $8, {{.*}} # xmm0 = xmm1[8,9,10,11,12,13,14,15],xmm0[0,1,2,3,4,5,6,7]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 6, i32 7, i32 0, i32 1>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_5670(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_5670
; SSE2:       # BB#0:
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],xmm1[3,0]
; SSE2-NEXT:    shufps {{.*}} # xmm1 = xmm1[1,2],xmm0[2,0]
; SSE2-NEXT:    movaps %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_5670
; SSE3:       # BB#0:
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,0],xmm1[3,0]
; SSE3-NEXT:    shufps {{.*}} # xmm1 = xmm1[1,2],xmm0[2,0]
; SSE3-NEXT:    movaps %xmm1, %xmm0
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_5670
; SSSE3:       # BB#0:
; SSSE3-NEXT:    palignr $4, {{.*}} # xmm0 = xmm1[4,5,6,7,8,9,10,11,12,13,14,15],xmm0[0,1,2,3]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_5670
; SSE41:       # BB#0:
; SSE41-NEXT:    palignr $4, {{.*}} # xmm0 = xmm1[4,5,6,7,8,9,10,11,12,13,14,15],xmm0[0,1,2,3]
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_5670
; AVX:       # BB#0:
; AVX-NEXT:    vpalignr $4, {{.*}} # xmm0 = xmm1[4,5,6,7,8,9,10,11,12,13,14,15],xmm0[0,1,2,3]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 5, i32 6, i32 7, i32 0>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_1234(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_1234
; SSE2:       # BB#0:
; SSE2-NEXT:    shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[3,0]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[1,2],xmm1[2,0]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_1234
; SSE3:       # BB#0:
; SSE3-NEXT:    shufps {{.*}} # xmm1 = xmm1[0,0],xmm0[3,0]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[1,2],xmm1[2,0]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_1234
; SSSE3:       # BB#0:
; SSSE3-NEXT:    palignr $4, {{.*}} # xmm1 = xmm0[4,5,6,7,8,9,10,11,12,13,14,15],xmm1[0,1,2,3]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_1234
; SSE41:       # BB#0:
; SSE41-NEXT:    palignr $4, {{.*}} # xmm1 = xmm0[4,5,6,7,8,9,10,11,12,13,14,15],xmm1[0,1,2,3]
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_1234
; AVX:       # BB#0:
; AVX-NEXT:    vpalignr $4, {{.*}} # xmm0 = xmm0[4,5,6,7,8,9,10,11,12,13,14,15],xmm1[0,1,2,3]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 1, i32 2, i32 3, i32 4>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_2345(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_2345
; SSE2:       # BB#0:
; SSE2-NEXT:    shufpd {{.*}} # xmm0 = xmm0[1],xmm1[0]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_2345
; SSE3:       # BB#0:
; SSE3-NEXT:    shufpd {{.*}} # xmm0 = xmm0[1],xmm1[0]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_2345
; SSSE3:       # BB#0:
; SSSE3-NEXT:    palignr $8, {{.*}} # xmm1 = xmm0[8,9,10,11,12,13,14,15],xmm1[0,1,2,3,4,5,6,7]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_2345
; SSE41:       # BB#0:
; SSE41-NEXT:    palignr $8, {{.*}} # xmm1 = xmm0[8,9,10,11,12,13,14,15],xmm1[0,1,2,3,4,5,6,7]
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_2345
; AVX:       # BB#0:
; AVX-NEXT:    vpalignr $8, {{.*}} # xmm0 = xmm0[8,9,10,11,12,13,14,15],xmm1[0,1,2,3,4,5,6,7]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 2, i32 3, i32 4, i32 5>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_3456(<4 x i32> %a, <4 x i32> %b) {
; SSE2-LABEL: @shuffle_v4i32_3456
; SSE2:       # BB#0:
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,0],xmm1[0,0]
; SSE2-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,2],xmm1[1,2]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_3456
; SSE3:       # BB#0:
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,0],xmm1[0,0]
; SSE3-NEXT:    shufps {{.*}} # xmm0 = xmm0[0,2],xmm1[1,2]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_3456
; SSSE3:       # BB#0:
; SSSE3-NEXT:    palignr $12, {{.*}} # xmm1 = xmm0[12,13,14,15],xmm1[0,1,2,3,4,5,6,7,8,9,10,11]
; SSSE3-NEXT:    movdqa %xmm1, %xmm0
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_3456
; SSE41:       # BB#0:
; SSE41-NEXT:    palignr $12, {{.*}} # xmm1 = xmm0[12,13,14,15],xmm1[0,1,2,3,4,5,6,7,8,9,10,11]
; SSE41-NEXT:    movdqa %xmm1, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_3456
; AVX:       # BB#0:
; AVX-NEXT:    vpalignr $12, {{.*}} # xmm0 = xmm0[12,13,14,15],xmm1[0,1,2,3,4,5,6,7,8,9,10,11]
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 3, i32 4, i32 5, i32 6>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_0u1u(<4 x i32> %a, <4 x i32> %b) {
; ALL-LABEL: @shuffle_v4i32_0u1u
; ALL:       # BB#0:
; ALL-NEXT:    pshufd {{.*}} # xmm0 = xmm0[0,0,1,1]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 0, i32 undef, i32 1, i32 undef>
  ret <4 x i32> %shuffle
}

define <4 x i32> @shuffle_v4i32_0z1z(<4 x i32> %a) {
; SSE2-LABEL: @shuffle_v4i32_0z1z
; SSE2:       # BB#0:
; SSE2-NEXT:    pxor %[[X:xmm[0-9]+]], %[[X]]
; SSE2-NEXT:    punpckldq {{.*}} # xmm0 = xmm0[0],[[X]][0],xmm0[1],[[X]][1]
; SSE2-NEXT:    retq
;
; SSE3-LABEL: @shuffle_v4i32_0z1z
; SSE3:       # BB#0:
; SSE3-NEXT:    pxor %[[X:xmm[0-9]+]], %[[X]]
; SSE3-NEXT:    punpckldq {{.*}} # xmm0 = xmm0[0],[[X]][0],xmm0[1],[[X]][1]
; SSE3-NEXT:    retq
;
; SSSE3-LABEL: @shuffle_v4i32_0z1z
; SSSE3:       # BB#0:
; SSSE3-NEXT:    pxor %[[X:xmm[0-9]+]], %[[X]]
; SSSE3-NEXT:    punpckldq {{.*}} # xmm0 = xmm0[0],[[X]][0],xmm0[1],[[X]][1]
; SSSE3-NEXT:    retq
;
; SSE41-LABEL: @shuffle_v4i32_0z1z
; SSE41:       # BB#0:
; SSE41-NEXT:    pmovzxdq %xmm0, %xmm0
; SSE41-NEXT:    retq
;
; AVX-LABEL: @shuffle_v4i32_0z1z
; AVX:       # BB#0:
; AVX-NEXT:    vpmovzxdq %xmm0, %xmm0
; AVX-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 1, i32 7>
  ret <4 x i32> %shuffle
}

define <4 x i32> @insert_reg_and_zero_v4i32(i32 %a) {
; ALL-LABEL: @insert_reg_and_zero_v4i32
; ALL:       # BB#0:
; ALL-NEXT:    movd %edi, %xmm0
; ALL-NEXT:    retq
  %v = insertelement <4 x i32> undef, i32 %a, i32 0
  %shuffle = shufflevector <4 x i32> %v, <4 x i32> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x i32> %shuffle
}

define <4 x i32> @insert_mem_and_zero_v4i32(i32* %ptr) {
; ALL-LABEL: @insert_mem_and_zero_v4i32
; ALL:       # BB#0:
; ALL-NEXT:    movd (%rdi), %xmm0
; ALL-NEXT:    retq
  %a = load i32* %ptr
  %v = insertelement <4 x i32> undef, i32 %a, i32 0
  %shuffle = shufflevector <4 x i32> %v, <4 x i32> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x i32> %shuffle
}

define <4 x float> @insert_reg_and_zero_v4f32(float %a) {
; SSE-LABEL: @insert_reg_and_zero_v4f32
; SSE:       # BB#0:
; SSE-NEXT:    xorps %[[X:xmm[0-9]+]], %[[X]]
; SSE-NEXT:    movss %xmm0, %[[X]]
; SSE-NEXT:    movaps %[[X]], %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: @insert_reg_and_zero_v4f32
; AVX:       # BB#0:
; AVX-NEXT:    vxorps %[[X:xmm[0-9]+]], %[[X]], %[[X]]
; AVX-NEXT:    vmovss %xmm0, %[[X]], %xmm0
; AVX-NEXT:    retq
  %v = insertelement <4 x float> undef, float %a, i32 0
  %shuffle = shufflevector <4 x float> %v, <4 x float> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x float> %shuffle
}

define <4 x float> @insert_mem_and_zero_v4f32(float* %ptr) {
; ALL-LABEL: @insert_mem_and_zero_v4f32
; ALL:       # BB#0:
; ALL-NEXT:    movss (%rdi), %xmm0
; ALL-NEXT:    retq
  %a = load float* %ptr
  %v = insertelement <4 x float> undef, float %a, i32 0
  %shuffle = shufflevector <4 x float> %v, <4 x float> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x float> %shuffle
}

define <4 x float> @shuffle_mem_v4f32_3210(<4 x float>* %ptr) {
; SSE-LABEL: @shuffle_mem_v4f32_3210
; SSE:       # BB#0:
; SSE-NEXT:    movaps (%rdi), %xmm0
; SSE-NEXT:    shufps {{.*}} # xmm0 = xmm0[3,2,1,0]
; SSE-NEXT:    retq
;
; AVX-LABEL: @shuffle_mem_v4f32_3210
; AVX:       # BB#0:
; AVX-NEXT:    vpermilps {{.*}} # xmm0 = mem[3,2,1,0]
; AVX-NEXT:    retq
  %a = load <4 x float>* %ptr
  %shuffle = shufflevector <4 x float> %a, <4 x float> undef, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  ret <4 x float> %shuffle
}
