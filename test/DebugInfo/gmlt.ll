; REQUIRES: object-emission
; RUN: %llc_dwarf -O0 -filetype=obj < %s | llvm-dwarfdump - | FileCheck %s

; Generated from the following source compiled with clang++ -gmlt:
; void f1() {}
; void __attribute__((section("__TEXT,__bar"))) f2() {}
; void __attribute__((always_inline)) f3() { f1(); }
; void f4() { f3(); }

; Check that
;  * -gmlt includes no DW_TAG_subprograms for subprograms without inlined
;    subroutines.
;  * yet still produces DW_AT_ranges and a range list in debug_ranges that
;    describes those subprograms

; CHECK: DW_TAG_compile_unit
; CHECK:   DW_AT_ranges [DW_FORM_sec_offset] (0x00000000)
; CHECK-NOT: {{DW_TAG|NULL}}


; FIXME: Emitting separate abstract definitions is inefficient when we could
; just attach the DW_AT_name to the inlined_subroutine directly. Except that
; would produce many string relocations. Implement string indexing in the
; skeleton CU to address the relocation problem, then remove abstract
; definitions from -gmlt here.

; CHECK: [[F3_ABS_DEF:.*]]:  DW_TAG_subprogram
; CHECK-NEXT:     DW_AT_name {{.*}} "f3"

; FIXME: We don't really need DW_AT_inline, consumers can ignore this due to
; the absence of high_pc/low_pc/ranges and know that they just need it for
; retrieving the name of a concrete inlined instance

; CHECK-NEXT:     DW_AT_inline
; CHECK-NOT: {{DW_TAG|DW_AT|NULL}}

; Check that we only provide the minimal attributes on a subprogram to save space.
; CHECK:   DW_TAG_subprogram
; CHECK-NEXT:     DW_AT_low_pc
; CHECK-NEXT:     DW_AT_high_pc
; CHECK-NEXT:     DW_AT_name
; CHECK-NOT: {{DW_TAG|DW_AT}}
; CHECK:     DW_TAG_inlined_subroutine

; As mentioned above - replace DW_AT_abstract_origin with DW_AT_name to save
; space once we have support for string indexing in non-dwo sections

; CHECK-NEXT:       DW_AT_abstract_origin {{.*}} {[[F3_ABS_DEF]]}
; CHECK-NEXT:       DW_AT_low_pc
; CHECK-NEXT:       DW_AT_high_pc
; CHECK-NEXT:       DW_AT_call_file
; CHECK-NEXT:       DW_AT_call_line

; Make sure we don't have any other subprograms here (subprograms with no
; inlined subroutines are omitted by design to save space)

; CHECK-NOT: {{DW_TAG|DW_AT}}
; CHECK: NULL
; CHECK-NOT: {{DW_TAG|DW_AT}}
; CHECK: NULL


; CHECK: .debug_ranges contents:

; ... some addresses (depends on platform (such as platforms with function
; reordering in the linker), and looks wonky on platforms with zero values
; written in relocation places (dumper needs to	be fixed to read the
; relocations rather than interpret that as the end of a range list))

; CHECK: 00000000 <End of list>


; Check that we don't emit any pubnames or pubtypes under -gmlt
; CHECK: .debug_pubnames contents:
; CHECK-NOT: Offset

; CHECK: .debug_pubtypes contents:
; CHECK-NOT: Offset

; Function Attrs: nounwind uwtable
define void @_Z2f1v() #0 {
entry:
  ret void, !dbg !13
}

; Function Attrs: nounwind uwtable
define void @_Z2f2v() #0 section "__TEXT,__bar" {
entry:
  ret void, !dbg !14
}

; Function Attrs: alwaysinline nounwind uwtable
define void @_Z2f3v() #1 {
entry:
  call void @_Z2f1v(), !dbg !15
  ret void, !dbg !16
}

; Function Attrs: nounwind uwtable
define void @_Z2f4v() #0 {
entry:
  call void @_Z2f1v() #2, !dbg !17
  ret void, !dbg !19
}

attributes #0 = { nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { alwaysinline nounwind uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!10, !11}
!llvm.ident = !{!12}

!0 = metadata !{i32 786449, metadata !1, i32 4, metadata !"clang version 3.6.0 ", i1 false, metadata !"", i32 0, metadata !2, metadata !2, metadata !3, metadata !2, metadata !2, metadata !"", i32 2} ; [ DW_TAG_compile_unit ] [/tmp/dbginfo/gmlt.cpp] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"gmlt.cpp", metadata !"/tmp/dbginfo"}
!2 = metadata !{}
!3 = metadata !{metadata !4, metadata !7, metadata !8, metadata !9}
!4 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"f1", metadata !"f1", metadata !"", i32 1, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void ()* @_Z2f1v, null, null, metadata !2, i32 1} ; [ DW_TAG_subprogram ] [line 1] [def] [f1]
!5 = metadata !{i32 786473, metadata !1}          ; [ DW_TAG_file_type ] [/tmp/dbginfo/gmlt.cpp]
!6 = metadata !{i32 786453, i32 0, null, metadata !"", i32 0, i64 0, i64 0, i64 0, i32 0, null, metadata !2, i32 0, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!7 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"f2", metadata !"f2", metadata !"", i32 2, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void ()* @_Z2f2v, null, null, metadata !2, i32 2} ; [ DW_TAG_subprogram ] [line 2] [def] [f2]
!8 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"f3", metadata !"f3", metadata !"", i32 3, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void ()* @_Z2f3v, null, null, metadata !2, i32 3} ; [ DW_TAG_subprogram ] [line 3] [def] [f3]
!9 = metadata !{i32 786478, metadata !1, metadata !5, metadata !"f4", metadata !"f4", metadata !"", i32 4, metadata !6, i1 false, i1 true, i32 0, i32 0, null, i32 256, i1 false, void ()* @_Z2f4v, null, null, metadata !2, i32 4} ; [ DW_TAG_subprogram ] [line 4] [def] [f4]
!10 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!11 = metadata !{i32 2, metadata !"Debug Info Version", i32 1}
!12 = metadata !{metadata !"clang version 3.6.0 "}
!13 = metadata !{i32 1, i32 12, metadata !4, null}
!14 = metadata !{i32 2, i32 53, metadata !7, null}
!15 = metadata !{i32 3, i32 44, metadata !8, null}
!16 = metadata !{i32 3, i32 50, metadata !8, null}
!17 = metadata !{i32 3, i32 44, metadata !8, metadata !18}
!18 = metadata !{i32 4, i32 13, metadata !9, null}
!19 = metadata !{i32 4, i32 19, metadata !9, null}
