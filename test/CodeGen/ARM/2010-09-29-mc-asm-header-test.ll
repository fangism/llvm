; RUN: llc < %s -mtriple=armv6-linux-gnueabi | FileCheck %s --check-prefix=V6
; RUN: llc < %s -mtriple=thumbv6m-linux-gnueabi | FileCheck %s --check-prefix=V6M
; RUN: llc < %s -mtriple=armv6-linux-gnueabi -mcpu=arm1156t2f-s | FileCheck %s --check-prefix=ARM1156T2F-S
; RUN: llc < %s -mtriple=thumbv7m-linux-gnueabi | FileCheck %s --check-prefix=V7M
; RUN: llc < %s -mtriple=armv7-linux-gnueabi | FileCheck %s --check-prefix=V7
; RUN: llc < %s -mtriple=armv8-linux-gnueabi | FileCheck %s --check-prefix=V8
; RUN: llc < %s -mtriple=thumbv8-linux-gnueabi | FileCheck %s --check-prefix=Vt8
; RUN: llc < %s -mtriple=armv8-linux-gnueabi -mattr=+fp-armv8 | FileCheck %s --check-prefix=V8-FPARMv8
; RUN: llc < %s -mtriple=armv8-linux-gnueabi -mattr=+neon | FileCheck %s --check-prefix=V8-NEON
; RUN: llc < %s -mtriple=armv8-linux-gnueabi -mattr=+fp-armv8 -mattr=+neon | FileCheck %s --check-prefix=V8-FPARMv8-NEON
; RUN: llc < %s -mtriple=armv8-linux-gnueabi -mattr=+fp-armv8,+neon,+crypto | FileCheck %s --check-prefix=V8-FPARMv8-NEON-CRYPTO
; RUN: llc < %s -mtriple=armv7-linux-gnueabi -mattr=-neon,-vfp2 | FileCheck %s --check-prefix=NOFP
; RUN: llc < %s -mtriple=armv7-linux-gnueabi -mcpu=cortex-a9 | FileCheck %s --check-prefix=CORTEX-A9
; RUN: llc < %s -mtriple=thumbv6m-linux-gnueabi -mcpu=cortex-m0 | FileCheck %s --check-prefix=CORTEX-M0
; RUN: llc < %s -mtriple=thumbv7m-linux-gnueabi -mcpu=cortex-m4 | FileCheck %s --check-prefix=CORTEX-M4
; RUN: llc < %s -mtriple=armv7r-linux-gnueabi -mcpu=cortex-r5 | FileCheck %s --check-prefix=CORTEX-R5
; RUN: llc < %s -mtriple=armv8-linux-gnueabi -mcpu=cortex-a53 | FileCheck %s --check-prefix=CORTEX-A53
; This tests that MC/asm header conversion is smooth and that build attributes are correct
;

; V6:   .eabi_attribute 6, 6
; V6:   .eabi_attribute 8, 1
; V6:   .eabi_attribute 24, 1
; V6:   .eabi_attribute 25, 1

; V6M:  .eabi_attribute 6, 12
; V6M:  .eabi_attribute 7, 77
; V6M:  .eabi_attribute 8, 0
; V6M:  .eabi_attribute 9, 1
; V6M:  .eabi_attribute 24, 1
; V6M:  .eabi_attribute 25, 1

; ARM1156T2F-S: .cpu arm1156t2f-s
; ARM1156T2F-S: .eabi_attribute 6, 8
; ARM1156T2F-S: .eabi_attribute 8, 1
; ARM1156T2F-S: .eabi_attribute 9, 2
; ARM1156T2F-S: .eabi_attribute 10, 2
; ARM1156T2F-S: .fpu vfpv2
; ARM1156T2F-S: .eabi_attribute 20, 1
; ARM1156T2F-S: .eabi_attribute 21, 1
; ARM1156T2F-S: .eabi_attribute 23, 3
; ARM1156T2F-S: .eabi_attribute 24, 1
; ARM1156T2F-S: .eabi_attribute 25, 1

; V7M:  .eabi_attribute 6, 10
; V7M:  .eabi_attribute 7, 77
; V7M:  .eabi_attribute 8, 0
; V7M:  .eabi_attribute 9, 2
; V7M:  .eabi_attribute 24, 1
; V7M:  .eabi_attribute 25, 1
; V7M:  .eabi_attribute 44, 0

; V7:      .syntax unified
; V7: .eabi_attribute 6, 10
; V7: .eabi_attribute 20, 1
; V7: .eabi_attribute 21, 1
; V7: .eabi_attribute 23, 3
; V7: .eabi_attribute 24, 1
; V7: .eabi_attribute 25, 1

; V8:      .syntax unified
; V8: .eabi_attribute 6, 14

; Vt8:     .syntax unified
; Vt8: .eabi_attribute 6, 14

; V8-FPARMv8:      .syntax unified
; V8-FPARMv8: .eabi_attribute 6, 14
; V8-FPARMv8: .eabi_attribute 10, 7
; V8-FPARMv8: .fpu fp-armv8

; V8-NEON:      .syntax unified
; V8-NEON: .eabi_attribute 6, 14
; V8-NEON: .fpu neon
; V8-NEON: .eabi_attribute 12, 3

; V8-FPARMv8-NEON:      .syntax unified
; V8-FPARMv8-NEON: .eabi_attribute 6, 14
; V8-FPARMv8-NEON: .fpu neon-fp-armv8
; V8-FPARMv8-NEON: .eabi_attribute 10, 7
; V8-FPARMv8-NEON: .eabi_attribute 12, 3

; V8-FPARMv8-NEON-CRYPTO:      .syntax unified
; V8-FPARMv8-NEON-CRYPTO: .eabi_attribute 6, 14
; V8-FPARMv8-NEON-CRYPTO: .fpu crypto-neon-fp-armv8
; V8-FPARMv8-NEON-CRYPTO: .eabi_attribute 10, 7
; V8-FPARMv8-NEON-CRYPTO: .eabi_attribute 12, 3

; NOFP-NOT:   .eabi_attribute 20
; NOFP-NOT:   .eabi_attribute 21
; NOFP-NOT:   .eabi_attribute 23

; CORTEX-A9:  .cpu cortex-a9
; CORTEX-A9:  .eabi_attribute 6, 10
; CORTEX-A9:  .eabi_attribute 7, 65
; CORTEX-A9:  .eabi_attribute 8, 1
; CORTEX-A9:  .eabi_attribute 9, 2
; CORTEX-A9:  .fpu neon
; CORTEX-A9:  .eabi_attribute 10, 3
; CORTEX-A9:  .eabi_attribute 12, 1
; CORTEX-A9:  .eabi_attribute 20, 1
; CORTEX-A9:  .eabi_attribute 21, 1
; CORTEX-A9:  .eabi_attribute 23, 3
; CORTEX-A9:  .eabi_attribute 24, 1
; CORTEX-A9:  .eabi_attribute 25, 1

; CORTEX-M0:  .cpu cortex-m0
; CORTEX-M0:  .eabi_attribute 6, 12
; CORTEX-M0:  .eabi_attribute 7, 77
; CORTEX-M0:  .eabi_attribute 8, 0
; CORTEX-M0:  .eabi_attribute 9, 1
; CORTEX-M0:  .eabi_attribute 24, 1
; CORTEX-M0:  .eabi_attribute 25, 1

; CORTEX-M4:  .cpu cortex-m4
; CORTEX-M4:  .eabi_attribute 6, 13
; CORTEX-M4:  .eabi_attribute 7, 77
; CORTEX-M4:  .eabi_attribute 8, 0
; CORTEX-M4:  .eabi_attribute 9, 2
; CORTEX-M4:  .eabi_attribute 10, 6
; CORTEX-M4:  .fpu vfpv4
; CORTEX-M4:  .eabi_attribute 20, 1
; CORTEX-M4:  .eabi_attribute 21, 1
; CORTEX-M4:  .eabi_attribute 23, 3
; CORTEX-M4:  .eabi_attribute 24, 1
; CORTEX-M4:  .eabi_attribute 25, 1
; CORTEX-M4:  .eabi_attribute 44, 0

; CORTEX-R5:  .cpu cortex-r5
; CORTEX-R5:  .eabi_attribute 6, 10
; CORTEX-R5:  .eabi_attribute 7, 82
; CORTEX-R5:   .eabi_attribute 8, 1
; CORTEX-R5:  .eabi_attribute 9, 2
; CORTEX-R5:  .eabi_attribute 10, 4
; CORTEX-R5:  .fpu vfpv3
; CORTEX-R5:  .eabi_attribute 20, 1
; CORTEX-R5:  .eabi_attribute 21, 1
; CORTEX-R5:  .eabi_attribute 23, 3
; CORTEX-R5:  .eabi_attribute 24, 1
; CORTEX-R5:  .eabi_attribute 25, 1
; CORTEX-R5:  .eabi_attribute 44, 2

; CORTEX-A53:  .cpu cortex-a53
; CORTEX-A53:  .eabi_attribute 6, 14
; CORTEX-A53:  .eabi_attribute 7, 65
; CORTEX-A53:  .eabi_attribute 8, 1
; CORTEX-A53:  .eabi_attribute 9, 2
; CORTEX-A53:  .eabi_attribute 24, 1
; CORTEX-A53:  .eabi_attribute 25, 1

define i32 @f(i64 %z) {
	ret i32 0
}
