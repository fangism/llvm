//===-- ARM64ELFObjectWriter.cpp - ARM64 ELF Writer -----------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file handles ELF-specific object emission, converting LLVM's internal
// fixups into the appropriate relocations.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARM64FixupKinds.h"
#include "MCTargetDesc/ARM64MCExpr.h"
#include "MCTargetDesc/ARM64MCTargetDesc.h"
#include "llvm/MC/MCELFObjectWriter.h"
#include "llvm/MC/MCValue.h"
#include "llvm/Support/ErrorHandling.h"

using namespace llvm;

namespace {
class ARM64ELFObjectWriter : public MCELFObjectTargetWriter {
public:
  ARM64ELFObjectWriter(uint8_t OSABI);

  virtual ~ARM64ELFObjectWriter();

protected:
  unsigned GetRelocType(const MCValue &Target, const MCFixup &Fixup,
                        bool IsPCRel) const override;

private:
};
}

ARM64ELFObjectWriter::ARM64ELFObjectWriter(uint8_t OSABI)
    : MCELFObjectTargetWriter(/*Is64Bit*/ true, OSABI, ELF::EM_AARCH64,
                              /*HasRelocationAddend*/ true) {}

ARM64ELFObjectWriter::~ARM64ELFObjectWriter() {}

unsigned ARM64ELFObjectWriter::GetRelocType(const MCValue &Target,
                                            const MCFixup &Fixup,
                                            bool IsPCRel) const {
  ARM64MCExpr::VariantKind RefKind =
      static_cast<ARM64MCExpr::VariantKind>(Target.getRefKind());
  ARM64MCExpr::VariantKind SymLoc = ARM64MCExpr::getSymbolLoc(RefKind);
  bool IsNC = ARM64MCExpr::isNotChecked(RefKind);

  assert((!Target.getSymA() ||
          Target.getSymA()->getKind() == MCSymbolRefExpr::VK_None) &&
         "Should only be expression-level modifiers here");

  assert((!Target.getSymB() ||
          Target.getSymB()->getKind() == MCSymbolRefExpr::VK_None) &&
         "Should only be expression-level modifiers here");

  if (IsPCRel) {
    switch ((unsigned)Fixup.getKind()) {
    case FK_Data_2:
      return ELF::R_AARCH64_PREL16;
    case FK_Data_4:
      return ELF::R_AARCH64_PREL32;
    case FK_Data_8:
      return ELF::R_AARCH64_PREL64;
    case ARM64::fixup_arm64_pcrel_adr_imm21:
      llvm_unreachable("No ELF relocations supported for ADR at the moment");
    case ARM64::fixup_arm64_pcrel_adrp_imm21:
      if (SymLoc == ARM64MCExpr::VK_ABS && !IsNC)
        return ELF::R_AARCH64_ADR_PREL_PG_HI21;
      if (SymLoc == ARM64MCExpr::VK_GOT && !IsNC)
        return ELF::R_AARCH64_ADR_GOT_PAGE;
      if (SymLoc == ARM64MCExpr::VK_GOTTPREL && !IsNC)
        return ELF::R_AARCH64_TLSIE_ADR_GOTTPREL_PAGE21;
      if (SymLoc == ARM64MCExpr::VK_TLSDESC && !IsNC)
        return ELF::R_AARCH64_TLSDESC_ADR_PAGE;
      llvm_unreachable("invalid symbol kind for ADRP relocation");
    case ARM64::fixup_arm64_pcrel_branch26:
      return ELF::R_AARCH64_JUMP26;
    case ARM64::fixup_arm64_pcrel_call26:
      return ELF::R_AARCH64_CALL26;
    case ARM64::fixup_arm64_pcrel_imm19:
      return ELF::R_AARCH64_TLSIE_LD_GOTTPREL_PREL19;
    default:
      llvm_unreachable("Unsupported pc-relative fixup kind");
    }
  } else {
    switch ((unsigned)Fixup.getKind()) {
    case FK_Data_2:
      return ELF::R_AARCH64_ABS16;
    case FK_Data_4:
      return ELF::R_AARCH64_ABS32;
    case FK_Data_8:
      return ELF::R_AARCH64_ABS64;
    case ARM64::fixup_arm64_add_imm12:
      if (SymLoc == ARM64MCExpr::VK_DTPREL && IsNC)
        return ELF::R_AARCH64_TLSLD_ADD_DTPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && !IsNC)
        return ELF::R_AARCH64_TLSLD_ADD_DTPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_TPREL && IsNC)
        return ELF::R_AARCH64_TLSLE_ADD_TPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_TPREL && !IsNC)
        return ELF::R_AARCH64_TLSLE_ADD_TPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_TLSDESC && IsNC)
        return ELF::R_AARCH64_TLSDESC_ADD_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_ABS && IsNC)
        return ELF::R_AARCH64_ADD_ABS_LO12_NC;

      report_fatal_error("invalid fixup for add (uimm12) instruction");
      return 0;
    case ARM64::fixup_arm64_ldst_imm12_scale1:
      if (SymLoc == ARM64MCExpr::VK_ABS && IsNC)
        return ELF::R_AARCH64_LDST8_ABS_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && !IsNC)
        return ELF::R_AARCH64_TLSLD_LDST8_DTPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && IsNC)
        return ELF::R_AARCH64_TLSLD_LDST8_DTPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_TPREL && !IsNC)
        return ELF::R_AARCH64_TLSLE_LDST8_TPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_TPREL && IsNC)
        return ELF::R_AARCH64_TLSLE_LDST8_TPREL_LO12_NC;

      report_fatal_error("invalid fixup for 8-bit load/store instruction");
      return 0;
    case ARM64::fixup_arm64_ldst_imm12_scale2:
      if (SymLoc == ARM64MCExpr::VK_ABS && IsNC)
        return ELF::R_AARCH64_LDST16_ABS_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && !IsNC)
        return ELF::R_AARCH64_TLSLD_LDST16_DTPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && IsNC)
        return ELF::R_AARCH64_TLSLD_LDST16_DTPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_TPREL && !IsNC)
        return ELF::R_AARCH64_TLSLE_LDST16_TPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_TPREL && IsNC)
        return ELF::R_AARCH64_TLSLE_LDST16_TPREL_LO12_NC;

      report_fatal_error("invalid fixup for 16-bit load/store instruction");
      return 0;
    case ARM64::fixup_arm64_ldst_imm12_scale4:
      if (SymLoc == ARM64MCExpr::VK_ABS && IsNC)
        return ELF::R_AARCH64_LDST32_ABS_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && !IsNC)
        return ELF::R_AARCH64_TLSLD_LDST32_DTPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && IsNC)
        return ELF::R_AARCH64_TLSLD_LDST32_DTPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_TPREL && !IsNC)
        return ELF::R_AARCH64_TLSLE_LDST32_TPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_TPREL && IsNC)
        return ELF::R_AARCH64_TLSLE_LDST32_TPREL_LO12_NC;

      report_fatal_error("invalid fixup for 32-bit load/store instruction");
      return 0;
    case ARM64::fixup_arm64_ldst_imm12_scale8:
      if (SymLoc == ARM64MCExpr::VK_ABS && IsNC)
        return ELF::R_AARCH64_LDST64_ABS_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_GOT && IsNC)
        return ELF::R_AARCH64_LD64_GOT_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && !IsNC)
        return ELF::R_AARCH64_TLSLD_LDST64_DTPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_DTPREL && IsNC)
        return ELF::R_AARCH64_TLSLD_LDST64_DTPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_TPREL && !IsNC)
        return ELF::R_AARCH64_TLSLE_LDST64_TPREL_LO12;
      if (SymLoc == ARM64MCExpr::VK_TPREL && IsNC)
        return ELF::R_AARCH64_TLSLE_LDST64_TPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_GOTTPREL && IsNC)
        return ELF::R_AARCH64_TLSIE_LD64_GOTTPREL_LO12_NC;
      if (SymLoc == ARM64MCExpr::VK_TLSDESC && IsNC)
        return ELF::R_AARCH64_TLSDESC_LD64_LO12_NC;

      report_fatal_error("invalid fixup for 64-bit load/store instruction");
      return 0;
    case ARM64::fixup_arm64_ldst_imm12_scale16:
      if (SymLoc == ARM64MCExpr::VK_ABS && IsNC)
        return ELF::R_AARCH64_LDST128_ABS_LO12_NC;

      report_fatal_error("invalid fixup for 128-bit load/store instruction");
      return 0;
    case ARM64::fixup_arm64_movw:
      if (RefKind == ARM64MCExpr::VK_ABS_G3)
        return ELF::R_AARCH64_MOVW_UABS_G3;
      if (RefKind == ARM64MCExpr::VK_ABS_G2)
        return ELF::R_AARCH64_MOVW_UABS_G2;
      if (RefKind == ARM64MCExpr::VK_ABS_G2_NC)
        return ELF::R_AARCH64_MOVW_UABS_G2_NC;
      if (RefKind == ARM64MCExpr::VK_ABS_G1)
        return ELF::R_AARCH64_MOVW_UABS_G1;
      if (RefKind == ARM64MCExpr::VK_ABS_G1_NC)
        return ELF::R_AARCH64_MOVW_UABS_G1_NC;
      if (RefKind == ARM64MCExpr::VK_ABS_G0)
        return ELF::R_AARCH64_MOVW_UABS_G0;
      if (RefKind == ARM64MCExpr::VK_ABS_G0_NC)
        return ELF::R_AARCH64_MOVW_UABS_G0_NC;
      if (RefKind == ARM64MCExpr::VK_DTPREL_G2)
        return ELF::R_AARCH64_TLSLD_MOVW_DTPREL_G2;
      if (RefKind == ARM64MCExpr::VK_DTPREL_G1)
        return ELF::R_AARCH64_TLSLD_MOVW_DTPREL_G1;
      if (RefKind == ARM64MCExpr::VK_DTPREL_G1_NC)
        return ELF::R_AARCH64_TLSLD_MOVW_DTPREL_G1_NC;
      if (RefKind == ARM64MCExpr::VK_DTPREL_G0)
        return ELF::R_AARCH64_TLSLD_MOVW_DTPREL_G0;
      if (RefKind == ARM64MCExpr::VK_DTPREL_G0_NC)
        return ELF::R_AARCH64_TLSLD_MOVW_DTPREL_G0_NC;
      if (RefKind == ARM64MCExpr::VK_TPREL_G2)
        return ELF::R_AARCH64_TLSLE_MOVW_TPREL_G2;
      if (RefKind == ARM64MCExpr::VK_TPREL_G1)
        return ELF::R_AARCH64_TLSLE_MOVW_TPREL_G1;
      if (RefKind == ARM64MCExpr::VK_TPREL_G1_NC)
        return ELF::R_AARCH64_TLSLE_MOVW_TPREL_G1_NC;
      if (RefKind == ARM64MCExpr::VK_TPREL_G0)
        return ELF::R_AARCH64_TLSLE_MOVW_TPREL_G0;
      if (RefKind == ARM64MCExpr::VK_TPREL_G0_NC)
        return ELF::R_AARCH64_TLSLE_MOVW_TPREL_G0_NC;
      if (RefKind == ARM64MCExpr::VK_GOTTPREL_G1)
        return ELF::R_AARCH64_TLSIE_MOVW_GOTTPREL_G1;
      if (RefKind == ARM64MCExpr::VK_GOTTPREL_G0_NC)
        return ELF::R_AARCH64_TLSIE_MOVW_GOTTPREL_G0_NC;
      report_fatal_error("invalid fixup for movz/movk instruction");
      return 0;
    case ARM64::fixup_arm64_tlsdesc_call:
      return ELF::R_AARCH64_TLSDESC_CALL;
    default:
      llvm_unreachable("Unknown ELF relocation type");
    }
  }

  llvm_unreachable("Unimplemented fixup -> relocation");
}

MCObjectWriter *llvm::createARM64ELFObjectWriter(raw_ostream &OS,
                                                 uint8_t OSABI) {
  MCELFObjectTargetWriter *MOTW = new ARM64ELFObjectWriter(OSABI);
  return createELFObjectWriter(MOTW, OS, /*IsLittleEndian=*/true);
}
