; This tests mach-O/PPC relocation entries.
; This test is paired with test/CodeGen/PowerPC/hello-reloc.s, 
; which tests llvm-mc.

; RUN: llc -filetype=asm -relocation-model=pic -mcpu=g4 -mtriple=powerpc-apple-darwin8 %s -o - | tee %t1 | FileCheck -check-prefix=DARWIN-G4-ASM %s
; RUN: llc -filetype=obj -relocation-model=pic -mcpu=g4 -mtriple=powerpc-apple-darwin8 %s -o - | tee %t2 | macho-dump | tee %t3 | FileCheck -check-prefix=DARWIN-G4-DUMP %s

; FIXME: validating .s->.o requires darwin asm syntax support in PPCAsmParser
; RUN-XFAIL: llvm-mc -relocation-model=pic -mcpu=g4 -triple=powerpc-apple-darwin8 %t1 -o - | tee %t4 | macho-dump | tee %t5 | FileCheck -check-prefix=DARWIN-G4-DUMP %s
; RUN-XFAIL: diff -u %t2 %t4 || diff -u %t3 %t5

; ModuleID = 'hello-puts.c'
; compiled with clang (-fno-common -DPIC -femit-all-decls) from:
; extern int puts(const char*);
; int main(int argc, char* argv[]) { puts("Hello, world!"); return 0; }

target datalayout = "E-p:32:32:32-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:32:64-f32:32:32-f64:64:64-v128:128:128-n32"
target triple = "powerpc-apple-macosx10.4.0"

@.str = private unnamed_addr constant [14 x i8] c"Hello, world!\00", align 1

; Function Attrs: nounwind
define i32 @main(i32 %argc, i8** %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca i8**, align 4
  store i32 0, i32* %retval
  store i32 %argc, i32* %argc.addr, align 4
  store i8** %argv, i8*** %argv.addr, align 4
  %call = call i32 @puts(i8* getelementptr inbounds ([14 x i8]* @.str, i32 0, i32 0))
  ret i32 0
}

declare i32 @puts(i8*) #1

attributes #0 = { nounwind "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "ssp-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "less-precise-fpmad"="false" "no-frame-pointer-elim"="true" "no-frame-pointer-elim-non-leaf"="true" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "ssp-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }

; DARWIN-G4-ASM:	.machine ppc7400
; DARWIN-G4-ASM-NEXT:	.section	__TEXT,__textcoal_nt,coalesced,pure_instructions
; DARWIN-G4-ASM-NEXT:	.section	__TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
; DARWIN-G4-ASM-NEXT:	.section	__TEXT,__text,regular,pure_instructions
; DARWIN-G4-ASM-NEXT:	.globl	_main
; DARWIN-G4-ASM-NEXT:	.align	4
; DARWIN-G4-ASM-NEXT:_main:                                  ; @main
; DARWIN-G4-ASM-NEXT:; BB#0:                                 ; %entry
; DARWIN-G4-ASM-NEXT:	mflr r0
; DARWIN-G4-ASM-NEXT:	stw r31, -4(r1)
; DARWIN-G4-ASM-NEXT:	stw r0, 8(r1)
; DARWIN-G4-ASM-NEXT:	stwu r1, -80(r1)
; DARWIN-G4-ASM-NEXT:	bl L0$pb
; DARWIN-G4-ASM-NEXT:L0$pb:
; DARWIN-G4-ASM-NEXT:	mr r31, r1
; DARWIN-G4-ASM-NEXT:	li [[REGA:r[0-9]+]], 0
; DARWIN-G4-ASM-NEXT:	mflr [[REGC:r[0-9]+]]
; DARWIN-G4-ASM-NEXT:	stw [[REGB:r[0-9]+]], 68(r31)
; DARWIN-G4-ASM-NEXT:	stw [[REGA]], 72(r31)
; DARWIN-G4-ASM-NEXT:	stw r4, 64(r31)
; DARWIN-G4-ASM-NEXT:	addis [[REGC]], [[REGC]], ha16(L_.str-L0$pb)
; DARWIN-G4-ASM-NEXT:	la [[REGB]], lo16(L_.str-L0$pb)([[REGC]])
; DARWIN-G4-ASM-NEXT:	bl L_puts$stub
; DARWIN-G4-ASM-NEXT:	li [[REGB]], 0
; DARWIN-G4-ASM-NEXT:	addi r1, r1, 80
; DARWIN-G4-ASM-NEXT:	lwz r0, 8(r1)
; DARWIN-G4-ASM-NEXT:	lwz r31, -4(r1)
; DARWIN-G4-ASM-NEXT:	mtlr r0
; DARWIN-G4-ASM-NEXT:	blr
; DARWIN-G4-ASM:	.section	__TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
; DARWIN-G4-ASM-NEXT:	.align	4
; DARWIN-G4-ASM-NEXT:L_puts$stub:
; DARWIN-G4-ASM-NEXT:	.indirect_symbol	_puts
; DARWIN-G4-ASM-NEXT:	mflr r0
; DARWIN-G4-ASM-NEXT:	bcl 20, 31, L_puts$stub$tmp
; DARWIN-G4-ASM-NEXT:L_puts$stub$tmp:
; DARWIN-G4-ASM-NEXT:	mflr [[REGD:r[0-9]+]]
; DARWIN-G4-ASM-NEXT:	addis [[REGD]], [[REGD]], ha16(L_puts$lazy_ptr-L_puts$stub$tmp)
; DARWIN-G4-ASM-NEXT:	mtlr r0
; DARWIN-G4-ASM-NEXT:	lwzu [[REGE:r[0-9]+]], lo16(L_puts$lazy_ptr-L_puts$stub$tmp)([[REGD]])
; DARWIN-G4-ASM-NEXT:	mtctr [[REGE]]
; DARWIN-G4-ASM-NEXT:	bctr
; DARWIN-G4-ASM-NEXT:	.section	__DATA,__la_symbol_ptr,lazy_symbol_pointers
; DARWIN-G4-ASM-NEXT:L_puts$lazy_ptr:
; DARWIN-G4-ASM-NEXT:	.indirect_symbol	_puts
; DARWIN-G4-ASM-NEXT:	.long	dyld_stub_binding_helper
; DARWIN-G4-ASM:.subsections_via_symbols
; DARWIN-G4-ASM-NEXT:	.section	__TEXT,__cstring,cstring_literals
; DARWIN-G4-ASM-NEXT:L_.str:                                 ; @.str
; DARWIN-G4-ASM-NEXT:	.asciz	 "Hello, world!"

; DARWIN-G4-DUMP: ('cputype', 18)
; DARWIN-G4-DUMP-NEXT: ('cpusubtype', 0)
; DARWIN-G4-DUMP-NEXT: ('filetype', 1)
; DARWIN-G4-DUMP-NEXT: ('num_load_commands', 3)
; DARWIN-G4-DUMP-NEXT: ('load_commands_size', 500)
; DARWIN-G4-DUMP-NEXT: ('flag', 8192)
; DARWIN-G4-DUMP-NEXT: ('load_commands', [
; DARWIN-G4-DUMP-NEXT: # Load Command 0
; DARWIN-G4-DUMP-NEXT: (('command', 1)
; DARWIN-G4-DUMP-NEXT: ('size', 396)
; DARWIN-G4-DUMP-NEXT: ('segment_name', '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('vm_addr', 0)
; DARWIN-G4-DUMP-NEXT: ('vm_size', 130)
; DARWIN-G4-DUMP-NEXT: ('file_offset', 528)
; DARWIN-G4-DUMP-NEXT: ('file_size', 130)
; DARWIN-G4-DUMP-NEXT: ('maxprot', 7)
; DARWIN-G4-DUMP-NEXT: ('initprot', 7)
; DARWIN-G4-DUMP-NEXT: ('num_sections', 5)
; DARWIN-G4-DUMP-NEXT: ('flags', 0)
; DARWIN-G4-DUMP-NEXT: ('sections', [
; DARWIN-G4-DUMP-NEXT: # Section 0
; DARWIN-G4-DUMP-NEXT: (('section_name', '__text\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('address', 0)
; DARWIN-G4-DUMP-NEXT: ('size', 80)
; DARWIN-G4-DUMP-NEXT: ('offset', 528)
; DARWIN-G4-DUMP-NEXT: ('alignment', 4)
; DARWIN-G4-DUMP-NEXT: ('reloc_offset', 660)
; DARWIN-G4-DUMP-NEXT: ('num_reloc', 5)
; DARWIN-G4-DUMP-NEXT: ('flags', 0x80000400)
; DARWIN-G4-DUMP-NEXT: ('reserved1', 0)
; DARWIN-G4-DUMP-NEXT: ('reserved2', 0)
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ('_relocations', [
; DARWIN-G4-DUMP-NEXT: # Relocation 0
; DARWIN-G4-DUMP-NEXT: (('word-0', 0x34),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x3c3)),
; DARWIN-G4-DUMP-NEXT: # Relocation 1
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xab000030),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x74)),
; DARWIN-G4-DUMP-NEXT: # Relocation 2
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xa1000000),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x14)),
; DARWIN-G4-DUMP-NEXT: # Relocation 3
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xac00002c),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x74)),
; DARWIN-G4-DUMP-NEXT: # Relocation 4
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xa1000060),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x14)),
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: # Section 1
; DARWIN-G4-DUMP-NEXT: (('section_name', '__textcoal_nt\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('address', 80)
; DARWIN-G4-DUMP-NEXT: ('size', 0)
; DARWIN-G4-DUMP-NEXT: ('offset', 608)
; DARWIN-G4-DUMP-NEXT: ('alignment', 0)
; DARWIN-G4-DUMP-NEXT: ('reloc_offset', 0)
; DARWIN-G4-DUMP-NEXT: ('num_reloc', 0)
; DARWIN-G4-DUMP-NEXT: ('flags', 0x8000000b)
; DARWIN-G4-DUMP-NEXT: ('reserved1', 0)
; DARWIN-G4-DUMP-NEXT: ('reserved2', 0)
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ('_relocations', [
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: # Section 2
; DARWIN-G4-DUMP-NEXT: (('section_name', '__picsymbolstub1')
; DARWIN-G4-DUMP-NEXT: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('address', 80)
; DARWIN-G4-DUMP-NEXT: ('size', 32)
; DARWIN-G4-DUMP-NEXT: ('offset', 608)
; DARWIN-G4-DUMP-NEXT: ('alignment', 4)
; DARWIN-G4-DUMP-NEXT: ('reloc_offset', 700)
; DARWIN-G4-DUMP-NEXT: ('num_reloc', 4)
; DARWIN-G4-DUMP-NEXT: ('flags', 0x80000408)
; DARWIN-G4-DUMP-NEXT: ('reserved1', 0)
; DARWIN-G4-DUMP-NEXT: ('reserved2', 32)
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ('_relocations', [
; DARWIN-G4-DUMP-NEXT: # Relocation 0
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xab000014),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x70)),
; DARWIN-G4-DUMP-NEXT: # Relocation 1
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xa1000000),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x58)),
; DARWIN-G4-DUMP-NEXT: # Relocation 2
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xac00000c),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x70)),
; DARWIN-G4-DUMP-NEXT: # Relocation 3
; DARWIN-G4-DUMP-NEXT: (('word-0', 0xa1000018),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x58)),
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: # Section 3
; DARWIN-G4-DUMP-NEXT: (('section_name', '__la_symbol_ptr\x00')
; DARWIN-G4-DUMP-NEXT: ('segment_name', '__DATA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('address', 112)
; DARWIN-G4-DUMP-NEXT: ('size', 4)
; DARWIN-G4-DUMP-NEXT: ('offset', 640)
; DARWIN-G4-DUMP-NEXT: ('alignment', 0)
; DARWIN-G4-DUMP-NEXT: ('reloc_offset', 732)
; DARWIN-G4-DUMP-NEXT: ('num_reloc', 1)
; DARWIN-G4-DUMP-NEXT: ('flags', 0x7)
; DARWIN-G4-DUMP-NEXT: ('reserved1', 1)
; DARWIN-G4-DUMP-NEXT: ('reserved2', 0)
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ('_relocations', [
; DARWIN-G4-DUMP-NEXT: # Relocation 0
; DARWIN-G4-DUMP-NEXT: (('word-0', 0x0),
; DARWIN-G4-DUMP-NEXT: ('word-1', 0x250)),
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: # Section 4
; DARWIN-G4-DUMP-NEXT: (('section_name', '__cstring\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('address', 116)
; DARWIN-G4-DUMP-NEXT: ('size', 14)
; DARWIN-G4-DUMP-NEXT: ('offset', 644)
; DARWIN-G4-DUMP-NEXT: ('alignment', 0)
; DARWIN-G4-DUMP-NEXT: ('reloc_offset', 0)
; DARWIN-G4-DUMP-NEXT: ('num_reloc', 0)
; DARWIN-G4-DUMP-NEXT: ('flags', 0x2)
; DARWIN-G4-DUMP-NEXT: ('reserved1', 0)
; DARWIN-G4-DUMP-NEXT: ('reserved2', 0)
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ('_relocations', [
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: # Load Command 1
; DARWIN-G4-DUMP-NEXT: (('command', 2)
; DARWIN-G4-DUMP-NEXT: ('size', 24)
; DARWIN-G4-DUMP-NEXT: ('symoff', 748)
; DARWIN-G4-DUMP-NEXT: ('nsyms', 3)
; DARWIN-G4-DUMP-NEXT: ('stroff', 784)
; DARWIN-G4-DUMP-NEXT: ('strsize', 40)
; DARWIN-G4-DUMP-NEXT: ('_string_data', '\x00_puts\x00dyld_stub_binding_helper\x00_main\x00\x00\x00')
; DARWIN-G4-DUMP-NEXT: ('_symbols', [
; DARWIN-G4-DUMP-NEXT: # Symbol 0
; DARWIN-G4-DUMP-NEXT: (('n_strx', 32)
; DARWIN-G4-DUMP-NEXT: ('n_type', 0xf)
; DARWIN-G4-DUMP-NEXT: ('n_sect', 1)
; DARWIN-G4-DUMP-NEXT: ('n_desc', 0)
; DARWIN-G4-DUMP-NEXT: ('n_value', 0)
; DARWIN-G4-DUMP-NEXT: ('_string', '_main')
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: # Symbol 1
; DARWIN-G4-DUMP-NEXT: (('n_strx', 1)
; DARWIN-G4-DUMP-NEXT: ('n_type', 0x1)
; DARWIN-G4-DUMP-NEXT: ('n_sect', 0)
; DARWIN-G4-DUMP-NEXT: ('n_desc', 1)
; DARWIN-G4-DUMP-NEXT: ('n_value', 0)
; DARWIN-G4-DUMP-NEXT: ('_string', '_puts')
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: # Symbol 2
; DARWIN-G4-DUMP-NEXT: (('n_strx', 7)
; DARWIN-G4-DUMP-NEXT: ('n_type', 0x1)
; DARWIN-G4-DUMP-NEXT: ('n_sect', 0)
; DARWIN-G4-DUMP-NEXT: ('n_desc', 0)
; DARWIN-G4-DUMP-NEXT: ('n_value', 0)
; DARWIN-G4-DUMP-NEXT: ('_string', 'dyld_stub_binding_helper')
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: # Load Command 2
; DARWIN-G4-DUMP-NEXT: (('command', 11)
; DARWIN-G4-DUMP-NEXT: ('size', 80)
; DARWIN-G4-DUMP-NEXT: ('ilocalsym', 0)
; DARWIN-G4-DUMP-NEXT: ('nlocalsym', 0)
; DARWIN-G4-DUMP-NEXT: ('iextdefsym', 0)
; DARWIN-G4-DUMP-NEXT: ('nextdefsym', 1)
; DARWIN-G4-DUMP-NEXT: ('iundefsym', 1)
; DARWIN-G4-DUMP-NEXT: ('nundefsym', 2)
; DARWIN-G4-DUMP-NEXT: ('tocoff', 0)
; DARWIN-G4-DUMP-NEXT: ('ntoc', 0)
; DARWIN-G4-DUMP-NEXT: ('modtaboff', 0)
; DARWIN-G4-DUMP-NEXT: ('nmodtab', 0)
; DARWIN-G4-DUMP-NEXT: ('extrefsymoff', 0)
; DARWIN-G4-DUMP-NEXT: ('nextrefsyms', 0)
; DARWIN-G4-DUMP-NEXT: ('indirectsymoff', 740)
; DARWIN-G4-DUMP-NEXT: ('nindirectsyms', 2)
; DARWIN-G4-DUMP-NEXT: ('extreloff', 0)
; DARWIN-G4-DUMP-NEXT: ('nextrel', 0)
; DARWIN-G4-DUMP-NEXT: ('locreloff', 0)
; DARWIN-G4-DUMP-NEXT: ('nlocrel', 0)
; DARWIN-G4-DUMP-NEXT: ('_indirect_symbols', [
; DARWIN-G4-DUMP-NEXT: # Indirect Symbol 0
; DARWIN-G4-DUMP-NEXT: (('symbol_index', 0x1),),
; DARWIN-G4-DUMP-NEXT: # Indirect Symbol 1
; DARWIN-G4-DUMP-NEXT: (('symbol_index', 0x1),),
; DARWIN-G4-DUMP-NEXT: ])
; DARWIN-G4-DUMP-NEXT: ),
; DARWIN-G4-DUMP-NEXT: ])
