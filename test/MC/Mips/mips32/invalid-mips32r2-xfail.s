# Instructions that are supposed to be invalid but currently aren't
# This test will XPASS if any insn stops assembling.
#
# RUN: not llvm-mc %s -triple=mips-unknown-linux -show-encoding -mcpu=mips32 \
# RUN:     2> %t1
# RUN: not FileCheck %s < %t1
# XFAIL: *

# CHECK-NOT: error
        .set noat
        cvt.l.d $f24,$f15
        cvt.l.s $f11,$f29
        di      $s8
        ei      $t6
        luxc1   $f19,$s6($s5)
        mfhc1   $s8,$f24
        mthc1   $zero,$f16
        rdhwr   $sp,$11
        suxc1   $f12,$k1($t5)
