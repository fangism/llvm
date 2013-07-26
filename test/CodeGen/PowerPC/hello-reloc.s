; This test is paired with test/CodeGen/PowerPC/hello-reloc.ll, 
; which tests llc.
; I took the asm produced by llc -filetype=asm and syntactically translated
; it to the supported PPCAsmParser syntax for this case.

; RUN: llvm-mc -filetype=obj -relocation-model=pic -mcpu=g4 -triple=powerpc-apple-darwin8 %s -o - | tee %t1 | macho-dump | tee %t2 | FileCheck -check-prefix=DARWIN-G4-DUMP %s

; Ideally we'd like to combine this test with hello-reloc.ll, but automating
; the assembly transformation would require GNU sed.
; The asm transformation will no longer be needed once darwin-asm syntax 
; is supported in PPCAsmParser, at which point these tests can be combined.

;	.machine ppc7400
	.section	__TEXT,__textcoal_nt,coalesced,pure_instructions
	.section	__TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.section	__TEXT,__text,regular,pure_instructions
	.globl	_main
	.align	4
_main:                                  ; @main
; BB#0:                                 ; %entry
	mflr 0
	stw 31, -4(1)
	stw 0, 8(1)
	stwu 1, -80(1)
	bl L0$pb
L0$pb:
	mr 31, 1
	li 5, 0
	mflr 2
	stw 3, 68(31)
	stw 5, 72(31)
	stw 4, 64(31)
	addis 2, 2, (L_.str-L0$pb)@ha
	la 3, (L_.str-L0$pb)@l(2)
	bl L_puts$stub
	li 3, 0
	addi 1, 1, 80
	lwz 0, 8(1)
	lwz 31, -4(1)
	mtlr 0
	blr

	.section	__TEXT,__picsymbolstub1,symbol_stubs,pure_instructions,32
	.align	4
L_puts$stub:
	.indirect_symbol	_puts
	mflr 0
	bcl 20, 31, L_puts$stub$tmp
L_puts$stub$tmp:
	mflr 11
	addis 11, 11, (L_puts$lazy_ptr-L_puts$stub$tmp)@ha
	mtlr 0
	lwzu 12, (L_puts$lazy_ptr-L_puts$stub$tmp)@l(11)
	mtctr 12
	bctr
	.section	__DATA,__la_symbol_ptr,lazy_symbol_pointers
L_puts$lazy_ptr:
	.indirect_symbol	_puts
	.long	dyld_stub_binding_helper

.subsections_via_symbols
	.section	__TEXT,__cstring,cstring_literals
L_.str:                                 ; @.str
	.asciz	 "Hello, world!"


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
