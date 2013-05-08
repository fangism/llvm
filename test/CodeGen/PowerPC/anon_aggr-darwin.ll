; RUN: llc -O0 -mcpu=g4 -mtriple=powerpc-apple-darwin8 < %s | FileCheck -check-prefix=DARWIN32 %s
; RUN: llc -O0 -mcpu=ppc970 -mtriple=powerpc64-apple-darwin8 < %s | FileCheck -check-prefix=DARWIN64 %s

; Test case cloned from "anon-aggr.ll", used for PR 14779.
; PR 15821: anonymous aggregates are not handled correctly.
; The bug is triggered by passing a byval structure after an anonymous
; aggregate.

%tarray = type { i64, i8* }

define i8* @func1({ i64, i8* } %array, i8* %ptr) {
entry:
  %array_ptr = extractvalue {i64, i8* } %array, 1
  %cond = icmp eq i8* %array_ptr, %ptr
  br i1 %cond, label %equal, label %unequal
equal:
  ret i8* %array_ptr
unequal:
  ret i8* %ptr
}

; DARWIN32: _func1:
; DARWIN32: mr
; DARWIN32: mr r[[REG1:[0-9]+]], r[[REGA:[0-9]+]]
; DARWIN32: mr r[[REG2:[0-9]+]], r[[REGB:[0-9]+]]
; DARWIN32: cmplw cr{{[0-9]+}}, r[[REGA]], r[[REGB]]
; DARWIN32: stw r[[REG1]], -[[OFFSET1:[0-9]+]]
; DARWIN32: stw r[[REG2]], -[[OFFSET2:[0-9]+]]
; DARWIN32: lwz r3, -[[OFFSET1]]
; DARWIN32: lwz r3, -[[OFFSET2]]

; DARWIN64: _func1:
; DARWIN64: mr
; DARWIN64: mr r[[REG1:[0-9]+]], r[[REGA:[0-9]+]]
; DARWIN64: mr r[[REG2:[0-9]+]], r[[REGB:[0-9]+]]
; DARWIN64: cmpld cr{{[0-9]+}}, r[[REGA]], r[[REGB]]
; DARWIN64: std r[[REG1]], -[[OFFSET1:[0-9]+]]
; DARWIN64: std r[[REG2]], -[[OFFSET2:[0-9]+]]
; DARWIN64: ld r3, -[[OFFSET1]]
; DARWIN64: ld r3, -[[OFFSET2]]


define i8* @func2({ i64, i8* } %array1, %tarray* byval %array2) {
entry:
  %array1_ptr = extractvalue {i64, i8* } %array1, 1
  %tmp = getelementptr inbounds %tarray* %array2, i32 0, i32 1
  %array2_ptr = load i8** %tmp
  %cond = icmp eq i8* %array1_ptr, %array2_ptr
  br i1 %cond, label %equal, label %unequal
equal:
  ret i8* %array1_ptr
unequal:
  ret i8* %array2_ptr
}

; DARWIN32: _func2:
; DARWIN32: addi r[[REG1:[0-9]+]], r[[REGSP:[0-9]+]], 36
; DARWIN32: lwz r[[REG2:[0-9]+]], 44(r[[REGSP]])
; DARWIN32: mr
; DARWIN32: mr r[[REG3:[0-9]+]], r[[REGA:[0-9]+]]
; DARWIN32: cmplw cr{{[0-9]+}}, r[[REGA]], r[[REG2]]
; DARWIN32: stw r[[REG3]], -[[OFFSET1:[0-9]+]]
; DARWIN32: stw r[[REG2]], -[[OFFSET2:[0-9]+]]
; DARWIN32: lwz r3, -[[OFFSET1]]
; DARWIN32: lwz r3, -[[OFFSET2]]

; DARWIN64: _func2:
; DARWIN64: addi r[[REG1:[0-9]+]], r1, 64
; DARWIN64: ld r[[REG2:[0-9]+]], 8(r[[REG1]])
; DARWIN64: mr
; DARWIN64: mr r[[REG3:[0-9]+]], r[[REGA:[0-9]+]]
; DARWIN64: cmpld cr{{[0-9]+}}, r[[REGA]], r[[REG2]]
; DARWIN64: std r[[REG3]], -[[OFFSET1:[0-9]+]]
; DARWIN64: std r[[REG2]], -[[OFFSET2:[0-9]+]]
; DARWIN64: ld r3, -[[OFFSET1]]
; DARWIN64: ld r3, -[[OFFSET2]]


define i8* @func3({ i64, i8* }* byval %array1, %tarray* byval %array2) {
entry:
  %tmp1 = getelementptr inbounds { i64, i8* }* %array1, i32 0, i32 1
  %array1_ptr = load i8** %tmp1
  %tmp2 = getelementptr inbounds %tarray* %array2, i32 0, i32 1
  %array2_ptr = load i8** %tmp2
  %cond = icmp eq i8* %array1_ptr, %array2_ptr
  br i1 %cond, label %equal, label %unequal
equal:
  ret i8* %array1_ptr
unequal:
  ret i8* %array2_ptr
}

; DARWIN32: _func3:
; DARWIN32: addi r[[REG1:[0-9]+]], r[[REGSP:[0-9]+]], 40
; DARWIN32: addi r[[REG2:[0-9]+]], r[[REGSP]], 24
; DARWIN32: lwz r[[REG3:[0-9]+]], 48(r[[REGSP]])
; DARWIN32: lwz r[[REG4:[0-9]+]], 32(r[[REGSP]])
; DARWIN32: cmplw cr{{[0-9]+}}, r[[REG4]], r[[REG3]]
; DARWIN32: stw r[[REG3]], -[[OFFSET1:[0-9]+]]
; DARWIN32: stw r[[REG4]], -[[OFFSET2:[0-9]+]]
; DARWIN32: lwz r3, -[[OFFSET2]]
; DARWIN32: lwz r3, -[[OFFSET1]]

; DARWIN64: _func3:
; DARWIN64: addi r[[REG1:[0-9]+]], r1, 64
; DARWIN64: addi r[[REG2:[0-9]+]], r1, 48
; DARWIN64: ld r[[REG3:[0-9]+]], 8(r[[REG1]])
; DARWIN64: ld r[[REG4:[0-9]+]], 8(r[[REG2]])
; DARWIN64: cmpld cr{{[0-9]+}}, r[[REG4]], r[[REG3]]
; DARWIN64: std r[[REG3]], -[[OFFSET1:[0-9]+]]
; DARWIN64: std r[[REG4]], -[[OFFSET2:[0-9]+]]
; DARWIN64: ld r3, -[[OFFSET2]]
; DARWIN64: ld r3, -[[OFFSET1]]


define i8* @func4(i64 %p1, i64 %p2, i64 %p3, i64 %p4,
                  i64 %p5, i64 %p6, i64 %p7, i64 %p8,
                  { i64, i8* } %array1, %tarray* byval %array2) {
entry:
  %array1_ptr = extractvalue {i64, i8* } %array1, 1
  %tmp = getelementptr inbounds %tarray* %array2, i32 0, i32 1
  %array2_ptr = load i8** %tmp
  %cond = icmp eq i8* %array1_ptr, %array2_ptr
  br i1 %cond, label %equal, label %unequal
equal:
  ret i8* %array1_ptr
unequal:
  ret i8* %array2_ptr
}

; DARWIN32: _func4:
; DARWIN32: lwz r[[REG4:[0-9]+]], 96(r1)
; DARWIN32: addi r[[REG1:[0-9]+]], r1, 100
; DARWIN32: lwz r[[REG3:[0-9]+]], 108(r1)
; DARWIN32: mr r[[REG2:[0-9]+]], r[[REG4]]
; DARWIN32: cmplw cr{{[0-9]+}}, r[[REG4]], r[[REG3]]
; DARWIN32: stw r[[REG4]], -[[OFFSET1:[0-9]+]]
; DARWIN32: stw r[[REG3]], -[[OFFSET2:[0-9]+]]
; DARWIN32: lwz r[[REG1]], -[[OFFSET1]]
; DARWIN32: lwz r[[REG1]], -[[OFFSET2]]

; DARWIN64: _func4:
; DARWIN64: addi r[[REG1:[0-9]+]], r1, 128
; DARWIN64: ld r[[REG2:[0-9]+]], 120(r1)
; DARWIN64: ld r[[REG3:[0-9]+]], 8(r[[REG1]])
; DARWIN64: mr r[[REG4:[0-9]+]], r[[REG2]]
; DARWIN64: cmpld cr{{[0-9]+}}, r[[REG2]], r[[REG3]]
; DARWIN64: std r[[REG4]], -[[OFFSET1:[0-9]+]]
; DARWIN64: std r[[REG3]], -[[OFFSET2:[0-9]+]]
; DARWIN64: ld r3, -[[OFFSET1]]
; DARWIN64: ld r3, -[[OFFSET2]]

