; RUN: llc -split-dwarf=Enable -O0 %s -mtriple=x86_64-unknown-linux-gnu -filetype=obj -o %t
; RUN: llvm-dwarfdump -debug-dump=all %t | FileCheck %s

@a = common global i32 0, align 4

!llvm.dbg.cu = !{!0}

!0 = metadata !{i32 786449, i32 0, i32 12, metadata !"baz.c", metadata !"/usr/local/google/home/echristo/tmp", metadata !"clang version 3.3 (trunk 169021) (llvm/trunk 169020)", i1 true, i1 false, metadata !"", i32 0, metadata !1, metadata !1, metadata !1, metadata !3, metadata !"baz.dwo"} ; [ DW_TAG_compile_unit ] [/usr/local/google/home/echristo/tmp/baz.c] [DW_LANG_C99]
!1 = metadata !{i32 0}
!3 = metadata !{metadata !5}
!5 = metadata !{i32 786484, i32 0, null, metadata !"a", metadata !"a", metadata !"", metadata !6, i32 1, metadata !7, i32 0, i32 1, i32* @a} ; [ DW_TAG_variable ] [a] [line 1] [def]
!6 = metadata !{i32 786473, metadata !"baz.c", metadata !"/usr/local/google/home/echristo/tmp", null} ; [ DW_TAG_file_type ]
!7 = metadata !{i32 786468, null, metadata !"int", null, i32 0, i64 32, i64 32, i64 0, i32 0, i32 5} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]

; Check that the skeleton compile unit contains the proper attributes:
; This DIE has the following attributes: DW_AT_comp_dir, DW_AT_stmt_list,
; DW_AT_low_pc, DW_AT_high_pc, DW_AT_ranges, DW_AT_dwo_name, DW_AT_dwo_id,
; DW_AT_ranges_base, DW_AT_addr_base.

; CHECK: .debug_abbrev contents:
; CHECK: Abbrev table for offset: 0x00000000
; CHECK: [1] DW_TAG_compile_unit DW_CHILDREN_no
; CHECK: DW_AT_GNU_dwo_name      DW_FORM_strp
; CHECK: DW_AT_GNU_dwo_id        DW_FORM_data8
; CHECK: DW_AT_GNU_addr_base     DW_FORM_sec_offset
; CHECK: DW_AT_low_pc    DW_FORM_addr
; CHECK: DW_AT_stmt_list DW_FORM_sec_offset
; CHECK: DW_AT_comp_dir  DW_FORM_strp

; CHECK: .debug_info contents:
; CHECK: DW_TAG_compile_unit
; CHECK: DW_AT_GNU_dwo_name [DW_FORM_strp] ( .debug_str[0x00000000] = "baz.dwo")
; CHECK: DW_AT_GNU_dwo_id [DW_FORM_data8]  (0x0000000000000000)
; CHECK: DW_AT_GNU_addr_base [DW_FORM_sec_offset]                   (0x00000000)
; CHECK: DW_AT_low_pc [DW_FORM_addr]       (0x0000000000000000)
; CHECK: DW_AT_stmt_list [DW_FORM_sec_offset]   (0x00000000)
; CHECK: DW_AT_comp_dir [DW_FORM_strp]     ( .debug_str[0x00000008] = "/usr/local/google/home/echristo/tmp")

; CHECK: .debug_str contents:
; CHECK: 0x00000000: "baz.dwo"
; CHECK: 0x00000008: "/usr/local/google/home/echristo/tmp"

; Check that we're using the right forms.
; CHECK: .debug_abbrev.dwo contents:
; CHECK: Abbrev table for offset: 0x00000000
; CHECK: [1] DW_TAG_compile_unit DW_CHILDREN_yes
; CHECK: DW_AT_producer  DW_FORM_GNU_str_index
; CHECK: DW_AT_language  DW_FORM_data2
; CHECK: DW_AT_name      DW_FORM_GNU_str_index
; CHECK: DW_AT_low_pc    DW_FORM_GNU_addr_index
; CHECK: DW_AT_stmt_list DW_FORM_data4
; CHECK: DW_AT_comp_dir  DW_FORM_GNU_str_index
; CHECK: DW_AT_GNU_dwo_id        DW_FORM_data8

; CHECK: [2] DW_TAG_base_type    DW_CHILDREN_no
; CHECK: DW_AT_name      DW_FORM_GNU_str_index
; CHECK: DW_AT_encoding  DW_FORM_data1
; CHECK: DW_AT_byte_size DW_FORM_data1

; CHECK: [3] DW_TAG_variable     DW_CHILDREN_no
; CHECK: DW_AT_name      DW_FORM_GNU_str_index
; CHECK: DW_AT_type      DW_FORM_ref4
; CHECK: DW_AT_external  DW_FORM_flag_present
; CHECK: DW_AT_decl_file DW_FORM_data1
; CHECK: DW_AT_decl_line DW_FORM_data1
; CHECK: DW_AT_location  DW_FORM_block1

; Check that the rest of the compile units have information.
; CHECK: .debug_info.dwo contents:
; CHECK: DW_TAG_compile_unit
; CHECK: DW_AT_producer [DW_FORM_GNU_str_index] ( indexed (00000000) string = "clang version 3.3 (trunk 169021) (llvm/trunk 169020)")
; CHECK: DW_AT_language [DW_FORM_data2]        (0x000c)
; CHECK: DW_AT_name [DW_FORM_GNU_str_index]    ( indexed (00000001) string = "baz.c")
; CHECK: DW_AT_low_pc [DW_FORM_GNU_addr_index]     ( indexed (00000000) address = 0x0000000000000000)
; CHECK: DW_AT_GNU_dwo_id [DW_FORM_data8]  (0x0000000000000000)
; CHECK: DW_TAG_base_type
; CHECK: DW_AT_name [DW_FORM_GNU_str_index]     ( indexed (00000004) string = "int")
; CHECK: DW_TAG_variable
; CHECK: DW_AT_name [DW_FORM_GNU_str_index]     ( indexed (00000003) string = "a")
; CHECK: DW_AT_type [DW_FORM_ref4]       (cu + 0x001e => {0x0000001e})
; CHECK: DW_AT_external [DW_FORM_flag_present]   (true)
; CHECK: DW_AT_decl_file [DW_FORM_data1] (0x01)
; CHECK: DW_AT_decl_line [DW_FORM_data1] (0x01)
; CHECK: DW_AT_location [DW_FORM_block1] (<0x02> fb 01 )


; CHECK: .debug_str.dwo contents:
; CHECK: 0x00000000: "clang version 3.3 (trunk 169021) (llvm/trunk 169020)"
; CHECK: 0x00000035: "baz.c"
; CHECK: 0x0000003b: "/usr/local/google/home/echristo/tmp"
; CHECK: 0x0000005f: "a"
; CHECK: 0x00000061: "int"

; CHECK: .debug_str_offsets.dwo contents:
; CHECK: 0x00000000: 00000000
; CHECK: 0x00000004: 00000035
; CHECK: 0x00000008: 0000003b
; CHECK: 0x0000000c: 0000005f
; CHECK: 0x00000010: 00000061
