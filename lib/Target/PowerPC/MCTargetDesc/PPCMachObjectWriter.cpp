//===-- PPCMachObjectWriter.cpp - PPC Mach-O Writer -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

/**
The skeleton of this file was ripped from X86MachObjectWriter.cpp.
NB: I have little clue what I'm doing. -- fangism
note to self: some clues might be found in gcc/config/{rs6000.c,darwin.h},
if I can ever decipher it. 
This file once existed before MC:
https://llvm.org/svn/llvm-project/llvm/tags/RELEASE_19/lib/Target/PowerPC/PPCMachOWriter.cpp
	authors: Nate Begeman, Louis Gerbarg, ...?
other references:
http://opensource.apple.com/source/cctools/cctools-809/as/ppc.c
**/

#include "MCTargetDesc/PPCMCTargetDesc.h"
#include "MCTargetDesc/PPCFixupKinds.h"
#include "llvm/ADT/Twine.h"
#include "llvm/MC/MCAsmLayout.h"
#include "llvm/MC/MCAssembler.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCMachObjectWriter.h"
#include "llvm/MC/MCSectionMachO.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Object/MachOFormat.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/Format.h"

#define	ENABLE_STACKTRACE		0

#include "llvm/Support/stacktrace.h"	// for debugging

using namespace llvm;
using namespace llvm::object;
#if ENABLE_STACKTRACE
using llvm::endl;
#endif

namespace {
class PPCMachObjectWriter : public MCMachObjectTargetWriter {
  bool RecordScatteredRelocation(MachObjectWriter *Writer,
                                 const MCAssembler &Asm,
                                 const MCAsmLayout &Layout,
                                 const MCFragment *Fragment,
                                 const MCFixup &Fixup,
                                 MCValue Target,
                                 unsigned Log2Size,
                                 uint64_t &FixedValue);

  void RecordPPCRelocation(MachObjectWriter *Writer,
                              const MCAssembler &Asm,
                              const MCAsmLayout &Layout,
                              const MCFragment *Fragment,
                              const MCFixup &Fixup,
                              MCValue Target,
                              uint64_t &FixedValue);
public:
  PPCMachObjectWriter(bool Is64Bit, uint32_t CPUType,
                      uint32_t CPUSubtype)
    : MCMachObjectTargetWriter(Is64Bit, CPUType, CPUSubtype,
                               /*UseAggressiveSymbolFolding=*/Is64Bit) {}

  void RecordRelocation(MachObjectWriter *Writer,
                        const MCAssembler &Asm, const MCAsmLayout &Layout,
                        const MCFragment *Fragment, const MCFixup &Fixup,
                        MCValue Target, uint64_t &FixedValue) {
    if (Writer->is64Bit()) {
      llvm_unreachable("Relocation emission for MachO/PPC64 unimplemented, until Fang gets around to hacking...");
    } else
      RecordPPCRelocation(Writer, Asm, Layout, Fragment, Fixup, Target,
                          FixedValue);
  }
};
}

/**
	Log2Size is used for relocation_info::r_length.
	See "include/llvm/MC/MCFixup.h"
	and "lib/Target/PowerPC/MCTargetDesc/PPCFixupKinds.h"
	See "lib/Target/PowerPC/MCTargetDesc/PPCAsmBackend.cpp":
		adjustFixupValue() for cases of handling fixups.
		getFixupKindInfo() for sizes.
 */
static unsigned getFixupKindLog2Size(unsigned Kind) {
  STACKTRACE_VERBOSE;
  switch (Kind) {
  default:
    STACKTRACE_INDENT_PRINT("Kind = " << Kind << endl);
    llvm_unreachable("invalid fixup kind!");
  case FK_PCRel_1:
  case FK_Data_1: return 0;
  case FK_PCRel_2:
  case FK_Data_2: return 1;
  case FK_PCRel_4:
#if 0
    // FIXME: Remove these!!!
  case PPC::reloc_riprel_4byte:
  case PPC::reloc_riprel_4byte_movq_load:
  case PPC::reloc_signed_4byte:
#endif
  case PPC::fixup_ppc_brcond14:
  case PPC::fixup_ppc_lo16:
  case PPC::fixup_ppc_ha16:
#if 0
  case PPC::fixup_ppc_lo14:
  case PPC::fixup_ppc_toc16:
  case PPC::fixup_ppc_toc16_ds:
#endif
  case PPC::fixup_ppc_br24:
  case FK_Data_4: return 2;
#if 0
  case PPC::fixup_ppc_toc:		// 64 bits
#endif
  case FK_PCRel_8:
  case FK_Data_8: return 3;
  }
  return 0;
}

/**
	Translates generic PPC fixup kind to Mach-O/PPC relocation type enum.
	Outline based on PPCELFObjectWriter::getRelocTypeInner().
 */
static
unsigned
getRelocType(
	const MCValue &Target,
	const MCFixupKind FixupKind,	// from Fixup.getKind()
	const bool IsPCRel) {
  STACKTRACE_BRIEF;
  STACKTRACE_INDENT_PRINT("getRelocType(): got FixupKind " << FixupKind << endl);
#if 0
  const MCSymbolRefExpr::VariantKind Modifier = Target.isAbsolute() ?
    MCSymbolRefExpr::VK_None : Target.getSymA()->getKind();
  // determine the type of the relocation
#endif
  unsigned Type = macho::RIT_PPC_VANILLA;
  if (IsPCRel) {			// relative to PC
    switch (FixupKind) {
    default:
      llvm_unreachable("Unimplemented (relative)");
    case PPC::fixup_ppc_br24:
      Type = macho::RIT_PPC_BR24;	// R_PPC_REL24
      break;
    case PPC::fixup_ppc_brcond14:
      Type = macho::RIT_PPC_BR14;
      break;
    case PPC::fixup_ppc_lo16:
      Type = macho::RIT_PPC_LO16;
      break;
    case PPC::fixup_ppc_ha16:
      Type = macho::RIT_PPC_HA16;
      break;
#if 0
    case PPC::fixup_ppc_lo14:
      Type = macho::RIT_PPC_LO14;
      break;
#endif
#if 0
    case FK_Data_4:
    case FK_PCRel_4:
      Type = macho::RIT_PPC_REL32;
      break;
    case FK_Data_8:
    case FK_PCRel_8:
      Type = macho::RIT_PPC64_REL64;
      break;
#endif
    }
  } else {
    switch (FixupKind) {
      default:
        llvm_unreachable("invalid fixup kind (absolute)!");
#if 0
    case PPC::fixup_ppc_br24:
      Type = macho::RIT_PPC_ADDR24;		// RIT_PPC_BR24?
      break;
    case PPC::fixup_ppc_brcond14:
      Type = macho::RIT_PPC_ADDR14;		// RIT_PPC_BR14?
      break;
#endif
    case PPC::fixup_ppc_ha16:
#if 0
      switch (Modifier) {
      default: llvm_unreachable("Unsupported Modifier");
      case MCSymbolRefExpr::VK_PPC_TPREL16_HA:
        Type = macho::RIT_PPC_HA16;		// RIT_PPC_TPREL16_HA;
        break;
#if 0
      case MCSymbolRefExpr::VK_PPC_DTPREL16_HA:
        Type = macho::RIT_PPC64_DTPREL16_HA;
        break;
#endif
      case MCSymbolRefExpr::VK_None:
        Type = macho::RIT_PPC_HA16;		// RIT_PPC_ADDR16_HA;
	break;
#if 0
      case MCSymbolRefExpr::VK_PPC_TOC16_HA:
        Type = macho::RIT_PPC64_TOC16_HA;
        break;
      case MCSymbolRefExpr::VK_PPC_GOT_TPREL16_HA:
        Type = macho::RIT_PPC64_GOT_TPREL16_HA;
        break;
      case MCSymbolRefExpr::VK_PPC_GOT_TLSGD16_HA:
        Type = macho::RIT_PPC64_GOT_TLSGD16_HA;
        break;
      case MCSymbolRefExpr::VK_PPC_GOT_TLSLD16_HA:
        Type = macho::RIT_PPC64_GOT_TLSLD16_HA;
        break;
#endif
      }
#else
	Type = macho::RIT_PPC_HA16_SECTDIFF;
#endif
      break;
    case PPC::fixup_ppc_lo16:
#if 0
      switch (Modifier) {
      default: llvm_unreachable("Unsupported Modifier");
      case MCSymbolRefExpr::VK_PPC_TPREL16_LO:
        Type = macho::RIT_PPC_LO16;		// RIT_PPC_TPREL16_LO;
        break;
      case MCSymbolRefExpr::VK_PPC_DTPREL16_LO:
        Type = macho::RIT_PPC64_DTPREL16_LO;
        break;
      case MCSymbolRefExpr::VK_None:
        Type = macho::RIT_PPC_ADDR16_LO;
	break;
      case MCSymbolRefExpr::VK_PPC_TOC16_LO:
        Type = macho::RIT_PPC64_TOC16_LO;
        break;
      case MCSymbolRefExpr::VK_PPC_GOT_TLSGD16_LO:
        Type = macho::RIT_PPC64_GOT_TLSGD16_LO;
        break;
      case MCSymbolRefExpr::VK_PPC_GOT_TLSLD16_LO:
        Type = macho::RIT_PPC64_GOT_TLSLD16_LO;
        break;
      }
#else
	Type = macho::RIT_PPC_LO16_SECTDIFF;
#endif
      break;
#if 0
    case PPC::fixup_ppc_lo14:
      Type = macho::RIT_PPC_LO14_SECTDIFF;
      break;
    case PPC::fixup_ppc_toc:
      Type = macho::RIT_PPC64_TOC;
      break;
    case PPC::fixup_ppc_toc16:
      Type = macho::RIT_PPC64_TOC16;
      break;
    case PPC::fixup_ppc_toc16_ds:
      switch (Modifier) {
      default: llvm_unreachable("Unsupported Modifier");
      case MCSymbolRefExpr::VK_PPC_TOC_ENTRY:
        Type = macho::RIT_PPC64_TOC16_DS;
	break;
      case MCSymbolRefExpr::VK_PPC_TOC16_LO:
        Type = macho::RIT_PPC64_TOC16_LO_DS;
        break;
      case MCSymbolRefExpr::VK_PPC_GOT_TPREL16_LO:
        Type = macho::RIT_PPC64_GOT_TPREL16_LO_DS;
        break;
      }
      break;
    case PPC::fixup_ppc_tlsreg:
      Type = macho::RIT_PPC64_TLS;
      break;
    case PPC::fixup_ppc_nofixup:
      switch (Modifier) {
      default: llvm_unreachable("Unsupported Modifier");
      case MCSymbolRefExpr::VK_PPC_TLSGD:
        Type = macho::RIT_PPC64_TLSGD;
        break;
      case MCSymbolRefExpr::VK_PPC_TLSLD:
        Type = macho::RIT_PPC64_TLSLD;
        break;
      }
      break;
    case FK_Data_8:
      switch (Modifier) {
      default: llvm_unreachable("Unsupported Modifier");
      case MCSymbolRefExpr::VK_PPC_TOC:
        Type = macho::RIT_PPC64_TOC;
        break;
      case MCSymbolRefExpr::VK_None:
        Type = macho::RIT_PPC64_ADDR64;
	break;
      }
      break;
#endif
    // go with RIT_PPC_VANILLA
    case FK_Data_4:
      // Type = macho::RIT_PPC_ADDR32;	// ELF: R_PPC_ADDR32
      break;
    case FK_Data_2:
      // Type = macho::RIT_PPC_ADDR16;	// ELF: R_PPC_ADDR16
      break;
    }
  }
  return Type;
}

static
// inline
void
makeRelocationInfo(
	macho::RelocationEntry& MRE,
	const uint32_t FixupOffset,
	const uint32_t Index,
	const unsigned IsPCRel,
	const unsigned Log2Size,
	const unsigned IsExtern, 
	const unsigned Type) {
  STACKTRACE_BRIEF;
  MRE.Word0 = FixupOffset;
// experimenting with order (endian fishiness on PPC?)
  MRE.Word1 = ((Index     << 8) |	// was << 0
               (IsPCRel   << 7) |	// was << 24
               (Log2Size  << 5) |	// was << 25
               (IsExtern  << 4) |	// was << 27
               (Type      << 0));	// was << 28
  STACKTRACE_INDENT_PRINT("FixupOffset: " << hex(FixupOffset) <<
	", Index: " << Index <<
	", IsPCRel: " << IsPCRel <<
	", Log2Size: " << Log2Size <<
	", IsExtern: " << IsExtern <<
	", Type: " << Type << endl);
  STACKTRACE_INDENT_PRINT("MRE.Word0 = " << hex(MRE.Word0) << endl);
  STACKTRACE_INDENT_PRINT("MRE.Word1 = " << hex(MRE.Word1) << endl);
}

static
// inline
void
makeScatteredRelocationInfo(
	macho::RelocationEntry& MRE,
	const uint32_t Addr,
	const unsigned Type,
	const unsigned Log2Size,
	const unsigned IsPCRel, 
	const uint32_t Value2) {
  STACKTRACE_BRIEF;
  // FIXME: see <mach-o/reloc.h> for note on endianness
  MRE.Word0 = ((Addr      <<  0) |
               (Type      << 24) |
               (Log2Size  << 28) |
               (IsPCRel   << 30) |
               macho::RF_Scattered);
  MRE.Word1 = Value2;
  STACKTRACE_INDENT_PRINT("Addr: " << hex(Addr) <<
	", Type: " << Type <<
	", Log2Size: " << Log2Size <<
	", IsPCRel: " << IsPCRel << endl);
  STACKTRACE_INDENT_PRINT("MRE.Word0 = " << hex(MRE.Word0) << endl);
  STACKTRACE_INDENT_PRINT("MRE.Word1 = " << hex(MRE.Word1) << endl);
}

/**
	\return false if falling back to using non-scattered relocation,
		otherwise true for normal scattered relocation.
 */
// based on X86MachObjectWriter::RecordScatteredRelocation
// and ARMMachObjectWriter::RecordScatteredRelocation
bool PPCMachObjectWriter::RecordScatteredRelocation(MachObjectWriter *Writer,
                                                    const MCAssembler &Asm,
                                                    const MCAsmLayout &Layout,
                                                    const MCFragment *Fragment,
                                                    const MCFixup &Fixup,
                                                    MCValue Target,
                                                    unsigned Log2Size,
                                                    uint64_t &FixedValue) {
  STACKTRACE_VERBOSE;
  // caller already computes these, can we just pass and reuse?
  uint32_t FixupOffset = Layout.getFragmentOffset(Fragment)+Fixup.getOffset();
  const MCFixupKind FK = Fixup.getKind();
  unsigned IsPCRel = Writer->isFixupKindPCRel(Asm, FK);
//  unsigned Type = macho::RIT_PPC_VANILLA;
  unsigned Type = getRelocType(Target, FK, IsPCRel);
  STACKTRACE_INDENT_PRINT("mach-o scattered reloc type: " << Type << endl);
    /*
     * odcctools-20090808:as/write_object.c:fix_to_relocation_entries():
     * Determine if this is left as a local relocation entry or
     * changed to a SECTDIFF relocation entry.  If this comes from a fix
     * that has a subtract symbol it is a SECTDIFF relocation.  Which is
     * "addsy - subsy + constant" where both symbols are defined in
     * sections.  To encode all this information two scattered
     * relocation entries are used.  The first has the add symbol value
     * and the second has the subtract symbol value.
     */


  // See <reloc.h>.
  const MCSymbol *A = &Target.getSymA()->getSymbol();
  MCSymbolData *A_SD = &Asm.getSymbolData(*A);

  if (!A_SD->getFragment())
    report_fatal_error("symbol '" + A->getName() +
                       "' can not be undefined in a subtraction expression");

  uint32_t Value = Writer->getSymbolAddress(A_SD, Layout);
  uint64_t SecAddr = Writer->getSectionAddress(A_SD->getFragment()->getParent());
  STACKTRACE_INDENT_PRINT("A symbol address: " << hex(Value) << endl);
  STACKTRACE_INDENT_PRINT("A section address: " << hex(SecAddr) << endl);
  FixedValue += SecAddr;
  uint32_t Value2 = 0;

  if (const MCSymbolRefExpr *B = Target.getSymB()) {
    MCSymbolData *B_SD = &Asm.getSymbolData(B->getSymbol());

    if (!B_SD->getFragment())
      report_fatal_error("symbol '" + B->getSymbol().getName() +
                         "' can not be undefined in a subtraction expression");

    // Select the appropriate difference relocation type.
    //
    // Note that there is no longer any semantic difference between these two
    // relocation types from the linkers point of view, this is done solely for
    // pedantic compatibility with 'as'.
#if 0
    Type = A_SD->isExternal() ? (unsigned)macho::RIT_PPC_SECTDIFF :
      (unsigned)macho::RIT_PPC_LO16_SECTDIFF;
//      (unsigned)macho::RIT_PPC_LOCAL_SECTDIFF;
//    Type = A_SD->isExternal() ? (unsigned)macho::RIT_Difference :
//      (unsigned)macho::RIT_Generic_LocalDifference;
#endif
    // FIXME: is Type correct? see include/llvm/Object/MachOFormat.h
    Value2 = Writer->getSymbolAddress(B_SD, Layout);
    STACKTRACE_INDENT_PRINT("B symbol address: " << hex(Value2) << endl);
    FixedValue -= Writer->getSectionAddress(B_SD->getFragment()->getParent());
  }
  STACKTRACE_INDENT_PRINT("Type = " << Type << endl);
  STACKTRACE_INDENT_PRINT("FixedValue: " << hex(FixedValue) << endl);
  // FIXME: does FixedValue get used??

  // Relocations are written out in reverse order, so the PAIR comes first.
  if (Type == macho::RIT_PPC_SECTDIFF ||
      Type == macho::RIT_PPC_HI16_SECTDIFF ||
      Type == macho::RIT_PPC_LO16_SECTDIFF ||
      Type == macho::RIT_PPC_HA16_SECTDIFF ||
      Type == macho::RIT_PPC_LO14_SECTDIFF ||
      Type == macho::RIT_PPC_LOCAL_SECTDIFF)
//  if (Type == macho::RIT_Difference ||
//      Type == macho::RIT_Generic_LocalDifference)
  {
    STACKTRACE_INDENT_PRINT("Type is Difference or Generic_LocalDifference" << endl);
#if 1
    // X86 had this piece, but ARM does not
    // If the offset is too large to fit in a scattered relocation,
    // we're hosed. It's an unfortunate limitation of the MachO format.
    if (FixupOffset > 0xffffff) {
      char Buffer[32];
      format("0x%x", FixupOffset).print(Buffer, sizeof(Buffer));
      Asm.getContext().FatalError(Fixup.getLoc(),
                         Twine("Section too large, can't encode "
                                "r_address (") + Buffer +
                         ") into 24 bits of scattered "
                         "relocation entry.");
      llvm_unreachable("fatal error returned?!");
    }
#endif

    // Is this supposed to follow MCTarget/PPCAsmBackend.cpp:adjustFixupValue()?
    uint32_t other_half = 0;
    switch (Type) {
    case macho::RIT_PPC_LO16_SECTDIFF:
      other_half = (FixedValue >> 16) & 0xffff;
      break;
    case macho::RIT_PPC_HA16_SECTDIFF:
      other_half = FixedValue & 0xffff;
      break;
    default:
//      llvm_unreachable("Unhandled PPC scattered relocation type.");
      break;
    }

    STACKTRACE_INDENT_PRINT("scattered relocation entry, part 1" << endl);
    macho::RelocationEntry MRE;
    makeScatteredRelocationInfo(MRE,
//	0,
	other_half,		// guessing by trial and error...
	macho::RIT_PPC_PAIR, Log2Size, IsPCRel, Value2);
    Writer->addRelocation(Fragment->getParent(), MRE);
#if 1
  } else {
    // If the offset is more than 24-bits, it won't fit in a scattered
    // relocation offset field, so we fall back to using a non-scattered
    // relocation. This is a bit risky, as if the offset reaches out of
    // the block and the linker is doing scattered loading on this
    // symbol, things can go badly.
    //
    // Required for 'as' compatibility.
    if (FixupOffset > 0xffffff)
      return false;
#endif
  }
  STACKTRACE_INDENT_PRINT("scattered relocation entry" << endl);
  macho::RelocationEntry MRE;
  makeScatteredRelocationInfo(MRE,
	FixupOffset, Type, Log2Size, IsPCRel, Value);
  Writer->addRelocation(Fragment->getParent(), MRE);
  return true;
}

// see PPCELFObjectWriter for a general outline of cases
void PPCMachObjectWriter::RecordPPCRelocation(MachObjectWriter *Writer,
                                              const MCAssembler &Asm,
                                              const MCAsmLayout &Layout,
                                              const MCFragment *Fragment,
                                              const MCFixup &Fixup,
                                              MCValue Target,
                                              uint64_t &FixedValue) {
  STACKTRACE_VERBOSE;
  const MCFixupKind FK = Fixup.getKind();	// unsigned
  STACKTRACE_INDENT_PRINT("FK = " << FK << endl);
  const unsigned Log2Size = getFixupKindLog2Size(FK);
  STACKTRACE_INDENT_PRINT("Log2Size = " << Log2Size << endl);
  const bool IsPCRel = Writer->isFixupKindPCRel(Asm, FK);
  STACKTRACE_INDENT_PRINT("IsPCRel = " << IsPCRel << endl);
  const unsigned RelocType = getRelocType(Target, FK, IsPCRel);
  STACKTRACE_INDENT_PRINT("RelocType = " << RelocType << endl);

#if 0
  if (!getRelocType(Target, FK, IsPCRel))
    // If we failed to get fixup kind info, it's because there's no legal
    // relocation type for the fixup kind. This happens when it's a fixup that's
    // expected to always be resolvable at assembly time and not have any
    // relocations needed.
    Asm.getContext().FatalError(Fixup.getLoc(),
                                "unsupported relocation on symbol");
#endif

  // If this is a difference or a defined symbol plus an offset, then we need a
  // scattered relocation entry. Differences always require scattered
  // relocations.
  if (Target.getSymB() &&
// Q: are branch targets ever scattered?
	RelocType != macho::RIT_PPC_BR24 &&
	RelocType != macho::RIT_PPC_BR14
	) {
    RecordScatteredRelocation(Writer, Asm, Layout, Fragment, Fixup,
                              Target, Log2Size, FixedValue);
    return;
  }

  // this doesn't seem right for RIT_PPC_BR24
  STACKTRACE_INDENT_PRINT("getting symbol A" << endl);
  // Get the symbol data, if any.
  MCSymbolData *SD = 0;
  if (Target.getSymA())
    SD = &Asm.getSymbolData(Target.getSymA()->getSymbol());
  if (SD) STACKTRACE_INDENT_PRINT("using symbol A data" << endl);

if (0) {
  // If this is an internal relocation with an offset, it also needs a scattered
  // relocation entry.
  uint32_t Offset = Target.getConstant();
  STACKTRACE_INDENT_PRINT("Target.getConstant(): " << hex(Offset) << endl);
  if (IsPCRel)
    Offset += 1 << Log2Size;
  STACKTRACE_INDENT_PRINT("Offset: " << hex(Offset) << endl);
  // Try to record the scattered relocation if needed. Fall back to non
  // scattered if necessary (see comments in RecordScatteredRelocation()
  // for details).
  if (Offset && SD && !Writer->doesSymbolRequireExternRelocation(SD) &&
      RecordScatteredRelocation(Writer, Asm, Layout, Fragment, Fixup,
                                Target, Log2Size, FixedValue))
    return;
}

  // See <reloc.h>.
  const uint32_t FixupOffset = Layout.getFragmentOffset(Fragment)+Fixup.getOffset();
  STACKTRACE_INDENT_PRINT("FixupOffset = " << hex(FixupOffset) << endl);
  unsigned Index = 0;
  unsigned IsExtern = 0;
//  unsigned Type = 0;
  unsigned Type = RelocType;

#if ENABLE_STACKTRACE
  MCSectionData* FragmentParent = Fragment->getParent();
  const MCSection& Sec(FragmentParent->getSection());
  STACKTRACE_INDENT_PRINT("section kind: " << Sec.getKind().getKindEnum() << endl);
  STACKTRACE_INDENT_PRINT("section label: " << Sec.getLabelBeginName() << endl);
#endif

  if (Target.isAbsolute()) { // constant
    // SymbolNum of 0 indicates the absolute section.
    //
    // FIXME: Currently, these are never generated (see code below). I cannot
    // find a case where they are actually emitted.
    report_fatal_error("FIXME: relocations to absolute targets "
                       "not yet implemented");
    // the above line stolen from ARM, not sure
//    Type = macho::RIT_PPC_VANILLA;
  } else {
    STACKTRACE_INDENT_PRINT("target is relative" << endl);
    // Resolve constant variables.
    if (SD->getSymbol().isVariable()) {
      STACKTRACE_INDENT_PRINT("symbol is variable" << endl);
      int64_t Res;
      if (SD->getSymbol().getVariableValue()->EvaluateAsAbsolute(
            Res, Layout, Writer->getSectionAddressMap())) {
        FixedValue = Res;
        STACKTRACE_INDENT_PRINT("evaluated FixedValue = " <<
		hex(FixedValue) << endl);
        return;
      }
    }

    // Check whether we need an external or internal relocation.
    if (Writer->doesSymbolRequireExternRelocation(SD)) {
      STACKTRACE_INDENT_PRINT("requires extern relocation" << endl);
      IsExtern = 1;
      Index = SD->getIndex();
      // For external relocations, make sure to offset the fixup value to
      // compensate for the addend of the symbol address, if it was
      // undefined. This occurs with weak definitions, for example.
      if (!SD->Symbol->isUndefined())
        FixedValue -= Layout.getSymbolOffset(SD);
    } else {
      STACKTRACE_INDENT_PRINT("no extern relocation" << endl);
      // The index is the section ordinal (1-based).
      const MCSectionData &SymSD = Asm.getSectionData(
        SD->getSymbol().getSection());
      Index = SymSD.getOrdinal() + 1;
      FixedValue += Writer->getSectionAddress(&SymSD);
    }
    if (IsPCRel)
      FixedValue -= Writer->getSectionAddress(Fragment->getParent());
    STACKTRACE_INDENT_PRINT("Index = " << Index << endl);
    STACKTRACE_INDENT_PRINT("fixed address = " << FixedValue << endl);

//    Type = macho::RIT_PPC_VANILLA;
  }

  // struct relocation_info (8 bytes)
  macho::RelocationEntry MRE;
  makeRelocationInfo(MRE,
	FixupOffset, Index, IsPCRel, Log2Size, IsExtern, Type);
  Writer->addRelocation(Fragment->getParent(), MRE);
}

MCObjectWriter *llvm::createPPCMachObjectWriter(raw_ostream &OS,
                                                bool Is64Bit,
                                                uint32_t CPUType,
                                                uint32_t CPUSubtype) {
  STACKTRACE_VERBOSE;
  return createMachObjectWriter(new PPCMachObjectWriter(Is64Bit,
                                                        CPUType,
                                                        CPUSubtype),
                                OS, /*IsLittleEndian=*/false);
}
