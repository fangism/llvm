; RUN: not llc < %s -mtriple=i686-pc-linux-gnu %s 2>&1 | FileCheck %s

@a = external global i32
@b = alias i32* @a
; CHECK: b: Target doesn't support aliases to declarations
