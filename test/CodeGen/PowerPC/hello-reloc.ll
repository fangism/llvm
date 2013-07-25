; This tests mach-O/PPC relocation entries.
; RUN: llc -filetype=asm -relocation-model=pic -mcpu=g4 -mtriple=powerpc-apple-darwin8 %s -o - | FileCheck -check-prefix=DARWIN-G4-ASM %s
; RUN: llc -filetype=obj -relocation-model=pic -mcpu=g4 -mtriple=powerpc-apple-darwin8 %s -o - | macho-dump | FileCheck -check-prefix=DARWIN-G4-DUMP %s

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
; DARWIN-G4-ASM:	.section	__TEXT,__textcoal_nt,coalesced,pure_instructions
; DARWIN-G4-ASM:	.section	__TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
; DARWIN-G4-ASM:	.section	__TEXT,__text,regular,pure_instructions
; DARWIN-G4-ASM:	.globl	_main
; DARWIN-G4-ASM:	.align	4
; DARWIN-G4-ASM:_main:                                  ; @main
; DARWIN-G4-ASM:; BB#0:                                 ; %entry
; DARWIN-G4-ASM:	mflr r0
; DARWIN-G4-ASM:	stw r31, -4(r1)
; DARWIN-G4-ASM:	stw r0, 8(r1)
; DARWIN-G4-ASM:	stwu r1, -80(r1)
; DARWIN-G4-ASM:	bl L0$pb
; DARWIN-G4-ASM:L0$pb:
; DARWIN-G4-ASM:	mr r31, r1
; DARWIN-G4-ASM:	li [[REGA:r[0-9]+]], 0
; DARWIN-G4-ASM:	mflr [[REGC:r[0-9]+]]
; DARWIN-G4-ASM:	stw [[REGB:r[0-9]+]], 68(r31)
; DARWIN-G4-ASM:	stw [[REGA]], 72(r31)
; DARWIN-G4-ASM:	stw r4, 64(r31)
; DARWIN-G4-ASM:	addis [[REGC]], [[REGC]], ha16(L_.str-L0$pb)
; DARWIN-G4-ASM:	la [[REGB]], lo16(L_.str-L0$pb)([[REGC]])
; DARWIN-G4-ASM:	bl L_puts$stub
; DARWIN-G4-ASM:	li [[REGB]], 0
; DARWIN-G4-ASM:	addi r1, r1, 80
; DARWIN-G4-ASM:	lwz r0, 8(r1)
; DARWIN-G4-ASM:	lwz r31, -4(r1)
; DARWIN-G4-ASM:	mtlr r0
; DARWIN-G4-ASM:	blr
; DARWIN-G4-ASM:	.section	__TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
; DARWIN-G4-ASM:	.align	4
; DARWIN-G4-ASM:L_puts$stub:
; DARWIN-G4-ASM:	.indirect_symbol	_puts
; DARWIN-G4-ASM:	mflr r0
; DARWIN-G4-ASM:	bcl 20, 31, L_puts$stub$tmp
; DARWIN-G4-ASM:L_puts$stub$tmp:
; DARWIN-G4-ASM:	mflr [[REGD:r[0-9]+]]
; DARWIN-G4-ASM:	addis [[REGD]], [[REGD]], ha16(L_puts$lazy_ptr-L_puts$stub$tmp)
; DARWIN-G4-ASM:	mtlr r0
; DARWIN-G4-ASM:	lwzu [[REGE:r[0-9]+]], lo16(L_puts$lazy_ptr-L_puts$stub$tmp)([[REGD]])
; DARWIN-G4-ASM:	mtctr [[REGE]]
; DARWIN-G4-ASM:	bctr
; DARWIN-G4-ASM:	.section	__DATA,__la_symbol_ptr,lazy_symbol_pointers
; DARWIN-G4-ASM:L_puts$lazy_ptr:
; DARWIN-G4-ASM:	.indirect_symbol	_puts
; DARWIN-G4-ASM:	.long	dyld_stub_binding_helper
; DARWIN-G4-ASM:.subsections_via_symbols
; DARWIN-G4-ASM:	.section	__TEXT,__cstring,cstring_literals
; DARWIN-G4-ASM:L_.str:                                 ; @.str
; DARWIN-G4-ASM:	.asciz	 "Hello, world!"

; DARWIN-G4-DUMP: ('cputype', 18)
; DARWIN-G4-DUMP: ('cpusubtype', 0)
; DARWIN-G4-DUMP: ('filetype', 1)
; DARWIN-G4-DUMP: ('num_load_commands', 3)
; DARWIN-G4-DUMP: ('load_commands_size', 500)
; DARWIN-G4-DUMP: ('flag', 8192)
; DARWIN-G4-DUMP: ('load_commands', [
; DARWIN-G4-DUMP: # Load Command 0
; DARWIN-G4-DUMP: (('command', 1)
; DARWIN-G4-DUMP: ('size', 396)
; DARWIN-G4-DUMP: ('segment_name', '\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('vm_addr', 0)
; DARWIN-G4-DUMP: ('vm_size', 130)
; DARWIN-G4-DUMP: ('file_offset', 528)
; DARWIN-G4-DUMP: ('file_size', 130)
; DARWIN-G4-DUMP: ('maxprot', 7)
; DARWIN-G4-DUMP: ('initprot', 7)
; DARWIN-G4-DUMP: ('num_sections', 5)
; DARWIN-G4-DUMP: ('flags', 0)
; DARWIN-G4-DUMP: ('sections', [
; DARWIN-G4-DUMP: # Section 0
; DARWIN-G4-DUMP: (('section_name', '__text\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('address', 0)
; DARWIN-G4-DUMP: ('size', 80)
; DARWIN-G4-DUMP: ('offset', 528)
; DARWIN-G4-DUMP: ('alignment', 4)
; DARWIN-G4-DUMP: ('reloc_offset', 660)
; DARWIN-G4-DUMP: ('num_reloc', 5)
; DARWIN-G4-DUMP: ('flags', 0x80000400)
; DARWIN-G4-DUMP: ('reserved1', 0)
; DARWIN-G4-DUMP: ('reserved2', 0)
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ('_relocations', [
; DARWIN-G4-DUMP: # Relocation 0
; DARWIN-G4-DUMP: (('word-0', 0x34),
; DARWIN-G4-DUMP: ('word-1', 0x3c3)),
; DARWIN-G4-DUMP: # Relocation 1
; DARWIN-G4-DUMP: (('word-0', 0xab000030),
; DARWIN-G4-DUMP: ('word-1', 0x74)),
; DARWIN-G4-DUMP: # Relocation 2
; DARWIN-G4-DUMP: (('word-0', 0xa1000000),
; DARWIN-G4-DUMP: ('word-1', 0x14)),
; DARWIN-G4-DUMP: # Relocation 3
; DARWIN-G4-DUMP: (('word-0', 0xac00002c),
; DARWIN-G4-DUMP: ('word-1', 0x74)),
; DARWIN-G4-DUMP: # Relocation 4
; DARWIN-G4-DUMP: (('word-0', 0xa1000060),
; DARWIN-G4-DUMP: ('word-1', 0x14)),
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: # Section 1
; DARWIN-G4-DUMP: (('section_name', '__textcoal_nt\x00\x00\x00')
; DARWIN-G4-DUMP: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('address', 80)
; DARWIN-G4-DUMP: ('size', 0)
; DARWIN-G4-DUMP: ('offset', 608)
; DARWIN-G4-DUMP: ('alignment', 0)
; DARWIN-G4-DUMP: ('reloc_offset', 0)
; DARWIN-G4-DUMP: ('num_reloc', 0)
; DARWIN-G4-DUMP: ('flags', 0x8000000b)
; DARWIN-G4-DUMP: ('reserved1', 0)
; DARWIN-G4-DUMP: ('reserved2', 0)
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ('_relocations', [
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: # Section 2
; DARWIN-G4-DUMP: (('section_name', '__picsymbolstub1')
; DARWIN-G4-DUMP: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('address', 80)
; DARWIN-G4-DUMP: ('size', 32)
; DARWIN-G4-DUMP: ('offset', 608)
; DARWIN-G4-DUMP: ('alignment', 4)
; DARWIN-G4-DUMP: ('reloc_offset', 700)
; DARWIN-G4-DUMP: ('num_reloc', 4)
; DARWIN-G4-DUMP: ('flags', 0x80000408)
; DARWIN-G4-DUMP: ('reserved1', 0)
; DARWIN-G4-DUMP: ('reserved2', 32)
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ('_relocations', [
; DARWIN-G4-DUMP: # Relocation 0
; DARWIN-G4-DUMP: (('word-0', 0xab000014),
; DARWIN-G4-DUMP: ('word-1', 0x70)),
; DARWIN-G4-DUMP: # Relocation 1
; DARWIN-G4-DUMP: (('word-0', 0xa1000000),
; DARWIN-G4-DUMP: ('word-1', 0x58)),
; DARWIN-G4-DUMP: # Relocation 2
; DARWIN-G4-DUMP: (('word-0', 0xac00000c),
; DARWIN-G4-DUMP: ('word-1', 0x70)),
; DARWIN-G4-DUMP: # Relocation 3
; DARWIN-G4-DUMP: (('word-0', 0xa1000018),
; DARWIN-G4-DUMP: ('word-1', 0x58)),
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: # Section 3
; DARWIN-G4-DUMP: (('section_name', '__la_symbol_ptr\x00')
; DARWIN-G4-DUMP: ('segment_name', '__DATA\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('address', 112)
; DARWIN-G4-DUMP: ('size', 4)
; DARWIN-G4-DUMP: ('offset', 640)
; DARWIN-G4-DUMP: ('alignment', 0)
; DARWIN-G4-DUMP: ('reloc_offset', 732)
; DARWIN-G4-DUMP: ('num_reloc', 1)
; DARWIN-G4-DUMP: ('flags', 0x7)
; DARWIN-G4-DUMP: ('reserved1', 1)
; DARWIN-G4-DUMP: ('reserved2', 0)
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ('_relocations', [
; DARWIN-G4-DUMP: # Relocation 0
; DARWIN-G4-DUMP: (('word-0', 0x0),
; DARWIN-G4-DUMP: ('word-1', 0x250)),
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: # Section 4
; DARWIN-G4-DUMP: (('section_name', '__cstring\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('segment_name', '__TEXT\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00')
; DARWIN-G4-DUMP: ('address', 116)
; DARWIN-G4-DUMP: ('size', 14)
; DARWIN-G4-DUMP: ('offset', 644)
; DARWIN-G4-DUMP: ('alignment', 0)
; DARWIN-G4-DUMP: ('reloc_offset', 0)
; DARWIN-G4-DUMP: ('num_reloc', 0)
; DARWIN-G4-DUMP: ('flags', 0x2)
; DARWIN-G4-DUMP: ('reserved1', 0)
; DARWIN-G4-DUMP: ('reserved2', 0)
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ('_relocations', [
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: # Load Command 1
; DARWIN-G4-DUMP: (('command', 2)
; DARWIN-G4-DUMP: ('size', 24)
; DARWIN-G4-DUMP: ('symoff', 748)
; DARWIN-G4-DUMP: ('nsyms', 3)
; DARWIN-G4-DUMP: ('stroff', 784)
; DARWIN-G4-DUMP: ('strsize', 40)
; DARWIN-G4-DUMP: ('_string_data', '\x00_main\x00dyld_stub_binding_helper\x00_puts\x00\x00\x00')
; DARWIN-G4-DUMP: ('_symbols', [
; DARWIN-G4-DUMP: # Symbol 0
; DARWIN-G4-DUMP: (('n_strx', 1)
; DARWIN-G4-DUMP: ('n_type', 0xf)
; DARWIN-G4-DUMP: ('n_sect', 1)
; DARWIN-G4-DUMP: ('n_desc', 0)
; DARWIN-G4-DUMP: ('n_value', 0)
; DARWIN-G4-DUMP: ('_string', '_main')
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: # Symbol 1
; DARWIN-G4-DUMP: (('n_strx', 32)
; DARWIN-G4-DUMP: ('n_type', 0x1)
; DARWIN-G4-DUMP: ('n_sect', 0)
; DARWIN-G4-DUMP: ('n_desc', 1)
; DARWIN-G4-DUMP: ('n_value', 0)
; DARWIN-G4-DUMP: ('_string', '_puts')
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: # Symbol 2
; DARWIN-G4-DUMP: (('n_strx', 7)
; DARWIN-G4-DUMP: ('n_type', 0x1)
; DARWIN-G4-DUMP: ('n_sect', 0)
; DARWIN-G4-DUMP: ('n_desc', 0)
; DARWIN-G4-DUMP: ('n_value', 0)
; DARWIN-G4-DUMP: ('_string', 'dyld_stub_binding_helper')
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: # Load Command 2
; DARWIN-G4-DUMP: (('command', 11)
; DARWIN-G4-DUMP: ('size', 80)
; DARWIN-G4-DUMP: ('ilocalsym', 0)
; DARWIN-G4-DUMP: ('nlocalsym', 0)
; DARWIN-G4-DUMP: ('iextdefsym', 0)
; DARWIN-G4-DUMP: ('nextdefsym', 1)
; DARWIN-G4-DUMP: ('iundefsym', 1)
; DARWIN-G4-DUMP: ('nundefsym', 2)
; DARWIN-G4-DUMP: ('tocoff', 0)
; DARWIN-G4-DUMP: ('ntoc', 0)
; DARWIN-G4-DUMP: ('modtaboff', 0)
; DARWIN-G4-DUMP: ('nmodtab', 0)
; DARWIN-G4-DUMP: ('extrefsymoff', 0)
; DARWIN-G4-DUMP: ('nextrefsyms', 0)
; DARWIN-G4-DUMP: ('indirectsymoff', 740)
; DARWIN-G4-DUMP: ('nindirectsyms', 2)
; DARWIN-G4-DUMP: ('extreloff', 0)
; DARWIN-G4-DUMP: ('nextrel', 0)
; DARWIN-G4-DUMP: ('locreloff', 0)
; DARWIN-G4-DUMP: ('nlocrel', 0)
; DARWIN-G4-DUMP: ('_indirect_symbols', [
; DARWIN-G4-DUMP: # Indirect Symbol 0
; DARWIN-G4-DUMP: (('symbol_index', 0x1),),
; DARWIN-G4-DUMP: # Indirect Symbol 1
; DARWIN-G4-DUMP: (('symbol_index', 0x1),),
; DARWIN-G4-DUMP: ])
; DARWIN-G4-DUMP: ),
; DARWIN-G4-DUMP: ])
