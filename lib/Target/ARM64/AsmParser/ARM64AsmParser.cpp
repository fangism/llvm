//===-- ARM64AsmParser.cpp - Parse ARM64 assembly to MCInst instructions --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "MCTargetDesc/ARM64AddressingModes.h"
#include "MCTargetDesc/ARM64MCExpr.h"
#include "Utils/ARM64BaseInfo.h"
#include "llvm/MC/MCParser/MCAsmLexer.h"
#include "llvm/MC/MCParser/MCAsmParser.h"
#include "llvm/MC/MCParser/MCParsedAsmOperand.h"
#include "llvm/MC/MCContext.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/MC/MCRegisterInfo.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/MC/MCSubtargetInfo.h"
#include "llvm/MC/MCSymbol.h"
#include "llvm/MC/MCTargetAsmParser.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/TargetRegistry.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/StringSwitch.h"
#include "llvm/ADT/Twine.h"
#include <cstdio>
using namespace llvm;

namespace {

class ARM64Operand;

class ARM64AsmParser : public MCTargetAsmParser {
public:
  typedef SmallVectorImpl<MCParsedAsmOperand *> OperandVector;

private:
  StringRef Mnemonic; ///< Instruction mnemonic.
  MCSubtargetInfo &STI;
  MCAsmParser &Parser;

  MCAsmParser &getParser() const { return Parser; }
  MCAsmLexer &getLexer() const { return Parser.getLexer(); }

  SMLoc getLoc() const { return Parser.getTok().getLoc(); }

  bool parseSysAlias(StringRef Name, SMLoc NameLoc, OperandVector &Operands);
  ARM64CC::CondCode parseCondCodeString(StringRef Cond);
  bool parseCondCode(OperandVector &Operands, bool invertCondCode);
  int tryParseRegister();
  int tryMatchVectorRegister(StringRef &Kind, bool expected);
  bool parseRegister(OperandVector &Operands);
  bool parseSymbolicImmVal(const MCExpr *&ImmVal);
  bool parseVectorList(OperandVector &Operands);
  bool parseOperand(OperandVector &Operands, bool isCondCode,
                    bool invertCondCode);

  void Warning(SMLoc L, const Twine &Msg) { Parser.Warning(L, Msg); }
  bool Error(SMLoc L, const Twine &Msg) { return Parser.Error(L, Msg); }
  bool showMatchError(SMLoc Loc, unsigned ErrCode);

  bool parseDirectiveWord(unsigned Size, SMLoc L);
  bool parseDirectiveTLSDescCall(SMLoc L);

  bool parseDirectiveLOH(StringRef LOH, SMLoc L);

  bool validateInstruction(MCInst &Inst, SmallVectorImpl<SMLoc> &Loc);
  bool MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                               OperandVector &Operands, MCStreamer &Out,
                               unsigned &ErrorInfo,
                               bool MatchingInlineAsm) override;
/// @name Auto-generated Match Functions
/// {

#define GET_ASSEMBLER_HEADER
#include "ARM64GenAsmMatcher.inc"

  /// }

  OperandMatchResultTy tryParseOptionalShiftExtend(OperandVector &Operands);
  OperandMatchResultTy tryParseBarrierOperand(OperandVector &Operands);
  OperandMatchResultTy tryParseMRSSystemRegister(OperandVector &Operands);
  OperandMatchResultTy tryParseSysReg(OperandVector &Operands);
  OperandMatchResultTy tryParseSysCROperand(OperandVector &Operands);
  OperandMatchResultTy tryParsePrefetch(OperandVector &Operands);
  OperandMatchResultTy tryParseAdrpLabel(OperandVector &Operands);
  OperandMatchResultTy tryParseAdrLabel(OperandVector &Operands);
  OperandMatchResultTy tryParseFPImm(OperandVector &Operands);
  OperandMatchResultTy tryParseAddSubImm(OperandVector &Operands);
  OperandMatchResultTy tryParseGPR64sp0Operand(OperandVector &Operands);
  bool tryParseVectorRegister(OperandVector &Operands);

public:
  enum ARM64MatchResultTy {
    Match_InvalidSuffix = FIRST_TARGET_MATCH_RESULT_TY,
#define GET_OPERAND_DIAGNOSTIC_TYPES
#include "ARM64GenAsmMatcher.inc"
  };
  ARM64AsmParser(MCSubtargetInfo &_STI, MCAsmParser &_Parser,
                 const MCInstrInfo &MII,
                 const MCTargetOptions &Options)
      : MCTargetAsmParser(), STI(_STI), Parser(_Parser) {
    MCAsmParserExtension::Initialize(_Parser);

    // Initialize the set of available features.
    setAvailableFeatures(ComputeAvailableFeatures(STI.getFeatureBits()));
  }

  bool ParseInstruction(ParseInstructionInfo &Info, StringRef Name,
                        SMLoc NameLoc, OperandVector &Operands) override;
  bool ParseRegister(unsigned &RegNo, SMLoc &StartLoc, SMLoc &EndLoc) override;
  bool ParseDirective(AsmToken DirectiveID) override;
  unsigned validateTargetOperandClass(MCParsedAsmOperand *Op,
                                      unsigned Kind) override;

  static bool classifySymbolRef(const MCExpr *Expr,
                                ARM64MCExpr::VariantKind &ELFRefKind,
                                MCSymbolRefExpr::VariantKind &DarwinRefKind,
                                int64_t &Addend);
};
} // end anonymous namespace

namespace {

/// ARM64Operand - Instances of this class represent a parsed ARM64 machine
/// instruction.
class ARM64Operand : public MCParsedAsmOperand {
private:
  enum KindTy {
    k_Immediate,
    k_ShiftedImm,
    k_CondCode,
    k_Register,
    k_VectorList,
    k_VectorIndex,
    k_Token,
    k_SysReg,
    k_SysCR,
    k_Prefetch,
    k_ShiftExtend,
    k_FPImm,
    k_Barrier
  } Kind;

  SMLoc StartLoc, EndLoc;

  struct TokOp {
    const char *Data;
    unsigned Length;
    bool IsSuffix; // Is the operand actually a suffix on the mnemonic.
  };

  struct RegOp {
    unsigned RegNum;
    bool isVector;
  };

  struct VectorListOp {
    unsigned RegNum;
    unsigned Count;
    unsigned NumElements;
    unsigned ElementKind;
  };

  struct VectorIndexOp {
    unsigned Val;
  };

  struct ImmOp {
    const MCExpr *Val;
  };

  struct ShiftedImmOp {
    const MCExpr *Val;
    unsigned ShiftAmount;
  };

  struct CondCodeOp {
    ARM64CC::CondCode Code;
  };

  struct FPImmOp {
    unsigned Val; // Encoded 8-bit representation.
  };

  struct BarrierOp {
    unsigned Val; // Not the enum since not all values have names.
  };

  struct SysRegOp {
    const char *Data;
    unsigned Length;
    uint64_t FeatureBits; // We need to pass through information about which
                          // core we are compiling for so that the SysReg
                          // Mappers can appropriately conditionalize.
  };

  struct SysCRImmOp {
    unsigned Val;
  };

  struct PrefetchOp {
    unsigned Val;
  };

  struct ShiftExtendOp {
    ARM64_AM::ShiftExtendType Type;
    unsigned Amount;
    bool HasExplicitAmount;
  };

  struct ExtendOp {
    unsigned Val;
  };

  union {
    struct TokOp Tok;
    struct RegOp Reg;
    struct VectorListOp VectorList;
    struct VectorIndexOp VectorIndex;
    struct ImmOp Imm;
    struct ShiftedImmOp ShiftedImm;
    struct CondCodeOp CondCode;
    struct FPImmOp FPImm;
    struct BarrierOp Barrier;
    struct SysRegOp SysReg;
    struct SysCRImmOp SysCRImm;
    struct PrefetchOp Prefetch;
    struct ShiftExtendOp ShiftExtend;
  };

  // Keep the MCContext around as the MCExprs may need manipulated during
  // the add<>Operands() calls.
  MCContext &Ctx;

  ARM64Operand(KindTy K, MCContext &_Ctx)
      : MCParsedAsmOperand(), Kind(K), Ctx(_Ctx) {}

public:
  ARM64Operand(const ARM64Operand &o) : MCParsedAsmOperand(), Ctx(o.Ctx) {
    Kind = o.Kind;
    StartLoc = o.StartLoc;
    EndLoc = o.EndLoc;
    switch (Kind) {
    case k_Token:
      Tok = o.Tok;
      break;
    case k_Immediate:
      Imm = o.Imm;
      break;
    case k_ShiftedImm:
      ShiftedImm = o.ShiftedImm;
      break;
    case k_CondCode:
      CondCode = o.CondCode;
      break;
    case k_FPImm:
      FPImm = o.FPImm;
      break;
    case k_Barrier:
      Barrier = o.Barrier;
      break;
    case k_Register:
      Reg = o.Reg;
      break;
    case k_VectorList:
      VectorList = o.VectorList;
      break;
    case k_VectorIndex:
      VectorIndex = o.VectorIndex;
      break;
    case k_SysReg:
      SysReg = o.SysReg;
      break;
    case k_SysCR:
      SysCRImm = o.SysCRImm;
      break;
    case k_Prefetch:
      Prefetch = o.Prefetch;
      break;
    case k_ShiftExtend:
      ShiftExtend = o.ShiftExtend;
      break;
    }
  }

  /// getStartLoc - Get the location of the first token of this operand.
  SMLoc getStartLoc() const override { return StartLoc; }
  /// getEndLoc - Get the location of the last token of this operand.
  SMLoc getEndLoc() const override { return EndLoc; }

  StringRef getToken() const {
    assert(Kind == k_Token && "Invalid access!");
    return StringRef(Tok.Data, Tok.Length);
  }

  bool isTokenSuffix() const {
    assert(Kind == k_Token && "Invalid access!");
    return Tok.IsSuffix;
  }

  const MCExpr *getImm() const {
    assert(Kind == k_Immediate && "Invalid access!");
    return Imm.Val;
  }

  const MCExpr *getShiftedImmVal() const {
    assert(Kind == k_ShiftedImm && "Invalid access!");
    return ShiftedImm.Val;
  }

  unsigned getShiftedImmShift() const {
    assert(Kind == k_ShiftedImm && "Invalid access!");
    return ShiftedImm.ShiftAmount;
  }

  ARM64CC::CondCode getCondCode() const {
    assert(Kind == k_CondCode && "Invalid access!");
    return CondCode.Code;
  }

  unsigned getFPImm() const {
    assert(Kind == k_FPImm && "Invalid access!");
    return FPImm.Val;
  }

  unsigned getBarrier() const {
    assert(Kind == k_Barrier && "Invalid access!");
    return Barrier.Val;
  }

  unsigned getReg() const override {
    assert(Kind == k_Register && "Invalid access!");
    return Reg.RegNum;
  }

  unsigned getVectorListStart() const {
    assert(Kind == k_VectorList && "Invalid access!");
    return VectorList.RegNum;
  }

  unsigned getVectorListCount() const {
    assert(Kind == k_VectorList && "Invalid access!");
    return VectorList.Count;
  }

  unsigned getVectorIndex() const {
    assert(Kind == k_VectorIndex && "Invalid access!");
    return VectorIndex.Val;
  }

  StringRef getSysReg() const {
    assert(Kind == k_SysReg && "Invalid access!");
    return StringRef(SysReg.Data, SysReg.Length);
  }

  uint64_t getSysRegFeatureBits() const {
    assert(Kind == k_SysReg && "Invalid access!");
    return SysReg.FeatureBits;
  }

  unsigned getSysCR() const {
    assert(Kind == k_SysCR && "Invalid access!");
    return SysCRImm.Val;
  }

  unsigned getPrefetch() const {
    assert(Kind == k_Prefetch && "Invalid access!");
    return Prefetch.Val;
  }

  ARM64_AM::ShiftExtendType getShiftExtendType() const {
    assert(Kind == k_ShiftExtend && "Invalid access!");
    return ShiftExtend.Type;
  }

  unsigned getShiftExtendAmount() const {
    assert(Kind == k_ShiftExtend && "Invalid access!");
    return ShiftExtend.Amount;
  }

  bool hasShiftExtendAmount() const {
    assert(Kind == k_ShiftExtend && "Invalid access!");
    return ShiftExtend.HasExplicitAmount;
  }

  bool isImm() const override { return Kind == k_Immediate; }
  bool isMem() const override { return false; }
  bool isSImm9() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= -256 && Val < 256);
  }
  bool isSImm7s4() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= -256 && Val <= 252 && (Val & 3) == 0);
  }
  bool isSImm7s8() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= -512 && Val <= 504 && (Val & 7) == 0);
  }
  bool isSImm7s16() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= -1024 && Val <= 1008 && (Val & 15) == 0);
  }

  bool isSymbolicUImm12Offset(const MCExpr *Expr, unsigned Scale) const {
    ARM64MCExpr::VariantKind ELFRefKind;
    MCSymbolRefExpr::VariantKind DarwinRefKind;
    int64_t Addend;
    if (!ARM64AsmParser::classifySymbolRef(Expr, ELFRefKind, DarwinRefKind,
                                           Addend)) {
      // If we don't understand the expression, assume the best and
      // let the fixup and relocation code deal with it.
      return true;
    }

    if (DarwinRefKind == MCSymbolRefExpr::VK_PAGEOFF ||
        ELFRefKind == ARM64MCExpr::VK_LO12 ||
        ELFRefKind == ARM64MCExpr::VK_GOT_LO12 ||
        ELFRefKind == ARM64MCExpr::VK_DTPREL_LO12 ||
        ELFRefKind == ARM64MCExpr::VK_DTPREL_LO12_NC ||
        ELFRefKind == ARM64MCExpr::VK_TPREL_LO12 ||
        ELFRefKind == ARM64MCExpr::VK_TPREL_LO12_NC ||
        ELFRefKind == ARM64MCExpr::VK_GOTTPREL_LO12_NC ||
        ELFRefKind == ARM64MCExpr::VK_TLSDESC_LO12) {
      // Note that we don't range-check the addend. It's adjusted modulo page
      // size when converted, so there is no "out of range" condition when using
      // @pageoff.
      return Addend >= 0 && (Addend % Scale) == 0;
    } else if (DarwinRefKind == MCSymbolRefExpr::VK_GOTPAGEOFF ||
               DarwinRefKind == MCSymbolRefExpr::VK_TLVPPAGEOFF) {
      // @gotpageoff/@tlvppageoff can only be used directly, not with an addend.
      return Addend == 0;
    }

    return false;
  }

  template <int Scale> bool isUImm12Offset() const {
    if (!isImm())
      return false;

    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return isSymbolicUImm12Offset(getImm(), Scale);

    int64_t Val = MCE->getValue();
    return (Val % Scale) == 0 && Val >= 0 && (Val / Scale) < 0x1000;
  }

  bool isImm0_7() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 8);
  }
  bool isImm1_8() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val > 0 && Val < 9);
  }
  bool isImm0_15() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 16);
  }
  bool isImm1_16() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val > 0 && Val < 17);
  }
  bool isImm0_31() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 32);
  }
  bool isImm1_31() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 1 && Val < 32);
  }
  bool isImm1_32() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 1 && Val < 33);
  }
  bool isImm0_63() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 64);
  }
  bool isImm1_63() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 1 && Val < 64);
  }
  bool isImm1_64() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 1 && Val < 65);
  }
  bool isImm0_127() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 128);
  }
  bool isImm0_255() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 256);
  }
  bool isImm0_65535() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 0 && Val < 65536);
  }
  bool isImm32_63() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    int64_t Val = MCE->getValue();
    return (Val >= 32 && Val < 64);
  }
  bool isLogicalImm32() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    return ARM64_AM::isLogicalImmediate(MCE->getValue(), 32);
  }
  bool isLogicalImm64() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    return ARM64_AM::isLogicalImmediate(MCE->getValue(), 64);
  }
  bool isShiftedImm() const { return Kind == k_ShiftedImm; }
  bool isAddSubImm() const {
    if (!isShiftedImm() && !isImm())
      return false;

    const MCExpr *Expr;

    // An ADD/SUB shifter is either 'lsl #0' or 'lsl #12'.
    if (isShiftedImm()) {
      unsigned Shift = ShiftedImm.ShiftAmount;
      Expr = ShiftedImm.Val;
      if (Shift != 0 && Shift != 12)
        return false;
    } else {
      Expr = getImm();
    }

    ARM64MCExpr::VariantKind ELFRefKind;
    MCSymbolRefExpr::VariantKind DarwinRefKind;
    int64_t Addend;
    if (ARM64AsmParser::classifySymbolRef(Expr, ELFRefKind,
                                          DarwinRefKind, Addend)) {
      return DarwinRefKind == MCSymbolRefExpr::VK_PAGEOFF
          || DarwinRefKind == MCSymbolRefExpr::VK_TLVPPAGEOFF
          || (DarwinRefKind == MCSymbolRefExpr::VK_GOTPAGEOFF && Addend == 0)
          || ELFRefKind == ARM64MCExpr::VK_LO12
          || ELFRefKind == ARM64MCExpr::VK_DTPREL_HI12
          || ELFRefKind == ARM64MCExpr::VK_DTPREL_LO12
          || ELFRefKind == ARM64MCExpr::VK_DTPREL_LO12_NC
          || ELFRefKind == ARM64MCExpr::VK_TPREL_HI12
          || ELFRefKind == ARM64MCExpr::VK_TPREL_LO12
          || ELFRefKind == ARM64MCExpr::VK_TPREL_LO12_NC
          || ELFRefKind == ARM64MCExpr::VK_TLSDESC_LO12;
    }

    // Otherwise it should be a real immediate in range:
    const MCConstantExpr *CE = cast<MCConstantExpr>(Expr);
    return CE->getValue() >= 0 && CE->getValue() <= 0xfff;
  }
  bool isCondCode() const { return Kind == k_CondCode; }
  bool isSIMDImmType10() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return false;
    return ARM64_AM::isAdvSIMDModImmType10(MCE->getValue());
  }
  bool isBranchTarget26() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return true;
    int64_t Val = MCE->getValue();
    if (Val & 0x3)
      return false;
    return (Val >= -(0x2000000 << 2) && Val <= (0x1ffffff << 2));
  }
  bool isPCRelLabel19() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return true;
    int64_t Val = MCE->getValue();
    if (Val & 0x3)
      return false;
    return (Val >= -(0x40000 << 2) && Val <= (0x3ffff << 2));
  }
  bool isBranchTarget14() const {
    if (!isImm())
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      return true;
    int64_t Val = MCE->getValue();
    if (Val & 0x3)
      return false;
    return (Val >= -(0x2000 << 2) && Val <= (0x1fff << 2));
  }

  bool isMovWSymbol(ArrayRef<ARM64MCExpr::VariantKind> AllowedModifiers) const {
    if (!isImm())
      return false;

    ARM64MCExpr::VariantKind ELFRefKind;
    MCSymbolRefExpr::VariantKind DarwinRefKind;
    int64_t Addend;
    if (!ARM64AsmParser::classifySymbolRef(getImm(), ELFRefKind, DarwinRefKind,
                                           Addend)) {
      return false;
    }
    if (DarwinRefKind != MCSymbolRefExpr::VK_None)
      return false;

    for (unsigned i = 0; i != AllowedModifiers.size(); ++i) {
      if (ELFRefKind == AllowedModifiers[i])
        return Addend == 0;
    }

    return false;
  }

  bool isMovZSymbolG3() const {
    static ARM64MCExpr::VariantKind Variants[] = { ARM64MCExpr::VK_ABS_G3 };
    return isMovWSymbol(Variants);
  }

  bool isMovZSymbolG2() const {
    static ARM64MCExpr::VariantKind Variants[] = { ARM64MCExpr::VK_ABS_G2,
                                                   ARM64MCExpr::VK_ABS_G2_S,
                                                   ARM64MCExpr::VK_TPREL_G2,
                                                   ARM64MCExpr::VK_DTPREL_G2 };
    return isMovWSymbol(Variants);
  }

  bool isMovZSymbolG1() const {
    static ARM64MCExpr::VariantKind Variants[] = { ARM64MCExpr::VK_ABS_G1,
                                                   ARM64MCExpr::VK_ABS_G1_S,
                                                   ARM64MCExpr::VK_GOTTPREL_G1,
                                                   ARM64MCExpr::VK_TPREL_G1,
                                                   ARM64MCExpr::VK_DTPREL_G1, };
    return isMovWSymbol(Variants);
  }

  bool isMovZSymbolG0() const {
    static ARM64MCExpr::VariantKind Variants[] = { ARM64MCExpr::VK_ABS_G0,
                                                   ARM64MCExpr::VK_ABS_G0_S,
                                                   ARM64MCExpr::VK_TPREL_G0,
                                                   ARM64MCExpr::VK_DTPREL_G0 };
    return isMovWSymbol(Variants);
  }

  bool isMovKSymbolG3() const {
    static ARM64MCExpr::VariantKind Variants[] = { ARM64MCExpr::VK_ABS_G3 };
    return isMovWSymbol(Variants);
  }

  bool isMovKSymbolG2() const {
    static ARM64MCExpr::VariantKind Variants[] = { ARM64MCExpr::VK_ABS_G2_NC };
    return isMovWSymbol(Variants);
  }

  bool isMovKSymbolG1() const {
    static ARM64MCExpr::VariantKind Variants[] = {
      ARM64MCExpr::VK_ABS_G1_NC, ARM64MCExpr::VK_TPREL_G1_NC,
      ARM64MCExpr::VK_DTPREL_G1_NC
    };
    return isMovWSymbol(Variants);
  }

  bool isMovKSymbolG0() const {
    static ARM64MCExpr::VariantKind Variants[] = {
      ARM64MCExpr::VK_ABS_G0_NC,   ARM64MCExpr::VK_GOTTPREL_G0_NC,
      ARM64MCExpr::VK_TPREL_G0_NC, ARM64MCExpr::VK_DTPREL_G0_NC
    };
    return isMovWSymbol(Variants);
  }

  template<int RegWidth, int Shift>
  bool isMOVZMovAlias() const {
    if (!isImm()) return false;

    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(getImm());
    if (!CE) return false;
    uint64_t Value = CE->getValue();

    if (RegWidth == 32)
      Value &= 0xffffffffULL;

    // "lsl #0" takes precedence: in practice this only affects "#0, lsl #0".
    if (Value == 0 && Shift != 0)
      return false;

    return (Value & ~(0xffffULL << Shift)) == 0;
  }

  template<int RegWidth, int Shift>
  bool isMOVNMovAlias() const {
    if (!isImm()) return false;

    const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(getImm());
    if (!CE) return false;
    uint64_t Value = CE->getValue();

    // MOVZ takes precedence over MOVN.
    for (int MOVZShift = 0; MOVZShift <= 48; MOVZShift += 16)
      if ((Value & ~(0xffffULL << MOVZShift)) == 0)
        return false;

    Value = ~Value;
    if (RegWidth == 32)
      Value &= 0xffffffffULL;

    return (Value & ~(0xffffULL << Shift)) == 0;
  }

  bool isFPImm() const { return Kind == k_FPImm; }
  bool isBarrier() const { return Kind == k_Barrier; }
  bool isSysReg() const { return Kind == k_SysReg; }
  bool isMRSSystemRegister() const {
    if (!isSysReg()) return false;

    bool IsKnownRegister;
    auto Mapper = ARM64SysReg::MRSMapper(getSysRegFeatureBits());
    Mapper.fromString(getSysReg(), IsKnownRegister);

    return IsKnownRegister;
  }
  bool isMSRSystemRegister() const {
    if (!isSysReg()) return false;

    bool IsKnownRegister;
    auto Mapper = ARM64SysReg::MSRMapper(getSysRegFeatureBits());
    Mapper.fromString(getSysReg(), IsKnownRegister);

    return IsKnownRegister;
  }
  bool isSystemPStateField() const {
    if (!isSysReg()) return false;

    bool IsKnownRegister;
    ARM64PState::PStateMapper().fromString(getSysReg(), IsKnownRegister);

    return IsKnownRegister;
  }
  bool isReg() const override { return Kind == k_Register && !Reg.isVector; }
  bool isVectorReg() const { return Kind == k_Register && Reg.isVector; }
  bool isVectorRegLo() const {
    return Kind == k_Register && Reg.isVector &&
      ARM64MCRegisterClasses[ARM64::FPR128_loRegClassID].contains(Reg.RegNum);
  }
  bool isGPR32as64() const {
    return Kind == k_Register && !Reg.isVector &&
      ARM64MCRegisterClasses[ARM64::GPR64RegClassID].contains(Reg.RegNum);
  }

  bool isGPR64sp0() const {
    return Kind == k_Register && !Reg.isVector &&
      ARM64MCRegisterClasses[ARM64::GPR64spRegClassID].contains(Reg.RegNum);
  }

  /// Is this a vector list with the type implicit (presumably attached to the
  /// instruction itself)?
  template <unsigned NumRegs> bool isImplicitlyTypedVectorList() const {
    return Kind == k_VectorList && VectorList.Count == NumRegs &&
           !VectorList.ElementKind;
  }

  template <unsigned NumRegs, unsigned NumElements, char ElementKind>
  bool isTypedVectorList() const {
    if (Kind != k_VectorList)
      return false;
    if (VectorList.Count != NumRegs)
      return false;
    if (VectorList.ElementKind != ElementKind)
      return false;
    return VectorList.NumElements == NumElements;
  }

  bool isVectorIndex1() const {
    return Kind == k_VectorIndex && VectorIndex.Val == 1;
  }
  bool isVectorIndexB() const {
    return Kind == k_VectorIndex && VectorIndex.Val < 16;
  }
  bool isVectorIndexH() const {
    return Kind == k_VectorIndex && VectorIndex.Val < 8;
  }
  bool isVectorIndexS() const {
    return Kind == k_VectorIndex && VectorIndex.Val < 4;
  }
  bool isVectorIndexD() const {
    return Kind == k_VectorIndex && VectorIndex.Val < 2;
  }
  bool isToken() const override { return Kind == k_Token; }
  bool isTokenEqual(StringRef Str) const {
    return Kind == k_Token && getToken() == Str;
  }
  bool isSysCR() const { return Kind == k_SysCR; }
  bool isPrefetch() const { return Kind == k_Prefetch; }
  bool isShiftExtend() const { return Kind == k_ShiftExtend; }
  bool isShifter() const {
    if (!isShiftExtend())
      return false;

    ARM64_AM::ShiftExtendType ST = getShiftExtendType();
    return (ST == ARM64_AM::LSL || ST == ARM64_AM::LSR || ST == ARM64_AM::ASR ||
            ST == ARM64_AM::ROR || ST == ARM64_AM::MSL);
  }
  bool isExtend() const {
    if (!isShiftExtend())
      return false;

    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    return (ET == ARM64_AM::UXTB || ET == ARM64_AM::SXTB ||
            ET == ARM64_AM::UXTH || ET == ARM64_AM::SXTH ||
            ET == ARM64_AM::UXTW || ET == ARM64_AM::SXTW ||
            ET == ARM64_AM::UXTX || ET == ARM64_AM::SXTX ||
            ET == ARM64_AM::LSL) &&
           getShiftExtendAmount() <= 4;
  }

  bool isExtend64() const {
    if (!isExtend())
      return false;
    // UXTX and SXTX require a 64-bit source register (the ExtendLSL64 class).
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    return ET != ARM64_AM::UXTX && ET != ARM64_AM::SXTX;
  }
  bool isExtendLSL64() const {
    if (!isExtend())
      return false;
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    return (ET == ARM64_AM::UXTX || ET == ARM64_AM::SXTX || ET == ARM64_AM::LSL) &&
      getShiftExtendAmount() <= 4;
  }

  template<int Width> bool isMemXExtend() const {
    if (!isExtend())
      return false;
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    return (ET == ARM64_AM::LSL || ET == ARM64_AM::SXTX) &&
           (getShiftExtendAmount() == Log2_32(Width / 8) ||
            getShiftExtendAmount() == 0);
  }

  template<int Width> bool isMemWExtend() const {
    if (!isExtend())
      return false;
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    return (ET == ARM64_AM::UXTW || ET == ARM64_AM::SXTW) &&
           (getShiftExtendAmount() == Log2_32(Width / 8) ||
            getShiftExtendAmount() == 0);
  }

  template <unsigned width>
  bool isArithmeticShifter() const {
    if (!isShifter())
      return false;

    // An arithmetic shifter is LSL, LSR, or ASR.
    ARM64_AM::ShiftExtendType ST = getShiftExtendType();
    return (ST == ARM64_AM::LSL || ST == ARM64_AM::LSR ||
            ST == ARM64_AM::ASR) && getShiftExtendAmount() < width;
  }

  template <unsigned width>
  bool isLogicalShifter() const {
    if (!isShifter())
      return false;

    // A logical shifter is LSL, LSR, ASR or ROR.
    ARM64_AM::ShiftExtendType ST = getShiftExtendType();
    return (ST == ARM64_AM::LSL || ST == ARM64_AM::LSR || ST == ARM64_AM::ASR ||
            ST == ARM64_AM::ROR) &&
           getShiftExtendAmount() < width;
  }

  bool isMovImm32Shifter() const {
    if (!isShifter())
      return false;

    // A MOVi shifter is LSL of 0, 16, 32, or 48.
    ARM64_AM::ShiftExtendType ST = getShiftExtendType();
    if (ST != ARM64_AM::LSL)
      return false;
    uint64_t Val = getShiftExtendAmount();
    return (Val == 0 || Val == 16);
  }

  bool isMovImm64Shifter() const {
    if (!isShifter())
      return false;

    // A MOVi shifter is LSL of 0 or 16.
    ARM64_AM::ShiftExtendType ST = getShiftExtendType();
    if (ST != ARM64_AM::LSL)
      return false;
    uint64_t Val = getShiftExtendAmount();
    return (Val == 0 || Val == 16 || Val == 32 || Val == 48);
  }

  bool isLogicalVecShifter() const {
    if (!isShifter())
      return false;

    // A logical vector shifter is a left shift by 0, 8, 16, or 24.
    unsigned Shift = getShiftExtendAmount();
    return getShiftExtendType() == ARM64_AM::LSL &&
           (Shift == 0 || Shift == 8 || Shift == 16 || Shift == 24);
  }

  bool isLogicalVecHalfWordShifter() const {
    if (!isLogicalVecShifter())
      return false;

    // A logical vector shifter is a left shift by 0 or 8.
    unsigned Shift = getShiftExtendAmount();
    return getShiftExtendType() == ARM64_AM::LSL && (Shift == 0 || Shift == 8);
  }

  bool isMoveVecShifter() const {
    if (!isShiftExtend())
      return false;

    // A logical vector shifter is a left shift by 8 or 16.
    unsigned Shift = getShiftExtendAmount();
    return getShiftExtendType() == ARM64_AM::MSL && (Shift == 8 || Shift == 16);
  }

  // Fallback unscaled operands are for aliases of LDR/STR that fall back
  // to LDUR/STUR when the offset is not legal for the former but is for
  // the latter. As such, in addition to checking for being a legal unscaled
  // address, also check that it is not a legal scaled address. This avoids
  // ambiguity in the matcher.
  template<int Width>
  bool isSImm9OffsetFB() const {
    return isSImm9() && !isUImm12Offset<Width / 8>();
  }

  bool isAdrpLabel() const {
    // Validation was handled during parsing, so we just sanity check that
    // something didn't go haywire.
    if (!isImm())
        return false;

    if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Imm.Val)) {
      int64_t Val = CE->getValue();
      int64_t Min = - (4096 * (1LL << (21 - 1)));
      int64_t Max = 4096 * ((1LL << (21 - 1)) - 1);
      return (Val % 4096) == 0 && Val >= Min && Val <= Max;
    }

    return true;
  }

  bool isAdrLabel() const {
    // Validation was handled during parsing, so we just sanity check that
    // something didn't go haywire.
    if (!isImm())
        return false;

    if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Imm.Val)) {
      int64_t Val = CE->getValue();
      int64_t Min = - (1LL << (21 - 1));
      int64_t Max = ((1LL << (21 - 1)) - 1);
      return Val >= Min && Val <= Max;
    }

    return true;
  }

  void addExpr(MCInst &Inst, const MCExpr *Expr) const {
    // Add as immediates when possible.  Null MCExpr = 0.
    if (!Expr)
      Inst.addOperand(MCOperand::CreateImm(0));
    else if (const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Expr))
      Inst.addOperand(MCOperand::CreateImm(CE->getValue()));
    else
      Inst.addOperand(MCOperand::CreateExpr(Expr));
  }

  void addRegOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  void addGPR32as64Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    assert(ARM64MCRegisterClasses[ARM64::GPR64RegClassID].contains(getReg()));

    const MCRegisterInfo *RI = Ctx.getRegisterInfo();
    uint32_t Reg = RI->getRegClass(ARM64::GPR32RegClassID).getRegister(
        RI->getEncodingValue(getReg()));

    Inst.addOperand(MCOperand::CreateReg(Reg));
  }

  void addVectorReg64Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    assert(ARM64MCRegisterClasses[ARM64::FPR128RegClassID].contains(getReg()));
    Inst.addOperand(MCOperand::CreateReg(ARM64::D0 + getReg() - ARM64::Q0));
  }

  void addVectorReg128Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    assert(ARM64MCRegisterClasses[ARM64::FPR128RegClassID].contains(getReg()));
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  void addVectorRegLoOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateReg(getReg()));
  }

  template <unsigned NumRegs>
  void addVectorList64Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    static unsigned FirstRegs[] = { ARM64::D0,       ARM64::D0_D1,
                                    ARM64::D0_D1_D2, ARM64::D0_D1_D2_D3 };
    unsigned FirstReg = FirstRegs[NumRegs - 1];

    Inst.addOperand(
        MCOperand::CreateReg(FirstReg + getVectorListStart() - ARM64::Q0));
  }

  template <unsigned NumRegs>
  void addVectorList128Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    static unsigned FirstRegs[] = { ARM64::Q0,       ARM64::Q0_Q1,
                                    ARM64::Q0_Q1_Q2, ARM64::Q0_Q1_Q2_Q3 };
    unsigned FirstReg = FirstRegs[NumRegs - 1];

    Inst.addOperand(
        MCOperand::CreateReg(FirstReg + getVectorListStart() - ARM64::Q0));
  }

  void addVectorIndex1Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getVectorIndex()));
  }

  void addVectorIndexBOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getVectorIndex()));
  }

  void addVectorIndexHOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getVectorIndex()));
  }

  void addVectorIndexSOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getVectorIndex()));
  }

  void addVectorIndexDOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getVectorIndex()));
  }

  void addImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    // If this is a pageoff symrefexpr with an addend, adjust the addend
    // to be only the page-offset portion. Otherwise, just add the expr
    // as-is.
    addExpr(Inst, getImm());
  }

  void addAddSubImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");
    if (isShiftedImm()) {
      addExpr(Inst, getShiftedImmVal());
      Inst.addOperand(MCOperand::CreateImm(getShiftedImmShift()));
    } else {
      addExpr(Inst, getImm());
      Inst.addOperand(MCOperand::CreateImm(0));
    }
  }

  void addCondCodeOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getCondCode()));
  }

  void addAdrpLabelOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE)
      addExpr(Inst, getImm());
    else
      Inst.addOperand(MCOperand::CreateImm(MCE->getValue() >> 12));
  }

  void addAdrLabelOperands(MCInst &Inst, unsigned N) const {
    addImmOperands(Inst, N);
  }

  template<int Scale>
  void addUImm12OffsetOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());

    if (!MCE) {
      Inst.addOperand(MCOperand::CreateExpr(getImm()));
      return;
    }
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() / Scale));
  }

  void addSImm9Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addSImm7s4Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() / 4));
  }

  void addSImm7s8Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() / 8));
  }

  void addSImm7s16Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() / 16));
  }

  void addImm0_7Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm1_8Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm0_15Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm1_16Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm0_31Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm1_31Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm1_32Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm0_63Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm1_63Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm1_64Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm0_127Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm0_255Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm0_65535Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addImm32_63Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue()));
  }

  void addLogicalImm32Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid logical immediate operand!");
    uint64_t encoding = ARM64_AM::encodeLogicalImmediate(MCE->getValue(), 32);
    Inst.addOperand(MCOperand::CreateImm(encoding));
  }

  void addLogicalImm64Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid logical immediate operand!");
    uint64_t encoding = ARM64_AM::encodeLogicalImmediate(MCE->getValue(), 64);
    Inst.addOperand(MCOperand::CreateImm(encoding));
  }

  void addSIMDImmType10Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    assert(MCE && "Invalid immediate operand!");
    uint64_t encoding = ARM64_AM::encodeAdvSIMDModImmType10(MCE->getValue());
    Inst.addOperand(MCOperand::CreateImm(encoding));
  }

  void addBranchTarget26Operands(MCInst &Inst, unsigned N) const {
    // Branch operands don't encode the low bits, so shift them off
    // here. If it's a label, however, just put it on directly as there's
    // not enough information now to do anything.
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE) {
      addExpr(Inst, getImm());
      return;
    }
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() >> 2));
  }

  void addPCRelLabel19Operands(MCInst &Inst, unsigned N) const {
    // Branch operands don't encode the low bits, so shift them off
    // here. If it's a label, however, just put it on directly as there's
    // not enough information now to do anything.
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE) {
      addExpr(Inst, getImm());
      return;
    }
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() >> 2));
  }

  void addBranchTarget14Operands(MCInst &Inst, unsigned N) const {
    // Branch operands don't encode the low bits, so shift them off
    // here. If it's a label, however, just put it on directly as there's
    // not enough information now to do anything.
    assert(N == 1 && "Invalid number of operands!");
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(getImm());
    if (!MCE) {
      addExpr(Inst, getImm());
      return;
    }
    assert(MCE && "Invalid constant immediate operand!");
    Inst.addOperand(MCOperand::CreateImm(MCE->getValue() >> 2));
  }

  void addFPImmOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getFPImm()));
  }

  void addBarrierOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getBarrier()));
  }

  void addMRSSystemRegisterOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");

    bool Valid;
    auto Mapper = ARM64SysReg::MRSMapper(getSysRegFeatureBits());
    uint32_t Bits = Mapper.fromString(getSysReg(), Valid);

    Inst.addOperand(MCOperand::CreateImm(Bits));
  }

  void addMSRSystemRegisterOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");

    bool Valid;
    auto Mapper = ARM64SysReg::MSRMapper(getSysRegFeatureBits());
    uint32_t Bits = Mapper.fromString(getSysReg(), Valid);

    Inst.addOperand(MCOperand::CreateImm(Bits));
  }

  void addSystemPStateFieldOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");

    bool Valid;
    uint32_t Bits = ARM64PState::PStateMapper().fromString(getSysReg(), Valid);

    Inst.addOperand(MCOperand::CreateImm(Bits));
  }

  void addSysCROperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getSysCR()));
  }

  void addPrefetchOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    Inst.addOperand(MCOperand::CreateImm(getPrefetch()));
  }

  void addShifterOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    unsigned Imm =
        ARM64_AM::getShifterImm(getShiftExtendType(), getShiftExtendAmount());
    Inst.addOperand(MCOperand::CreateImm(Imm));
  }

  void addExtendOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    if (ET == ARM64_AM::LSL) ET = ARM64_AM::UXTW;
    unsigned Imm = ARM64_AM::getArithExtendImm(ET, getShiftExtendAmount());
    Inst.addOperand(MCOperand::CreateImm(Imm));
  }

  void addExtend64Operands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    if (ET == ARM64_AM::LSL) ET = ARM64_AM::UXTX;
    unsigned Imm = ARM64_AM::getArithExtendImm(ET, getShiftExtendAmount());
    Inst.addOperand(MCOperand::CreateImm(Imm));
  }

  void addMemExtendOperands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    bool IsSigned = ET == ARM64_AM::SXTW || ET == ARM64_AM::SXTX;
    Inst.addOperand(MCOperand::CreateImm(IsSigned));
    Inst.addOperand(MCOperand::CreateImm(getShiftExtendAmount() != 0));
  }

  // For 8-bit load/store instructions with a register offset, both the
  // "DoShift" and "NoShift" variants have a shift of 0. Because of this,
  // they're disambiguated by whether the shift was explicit or implicit rather
  // than its size.
  void addMemExtend8Operands(MCInst &Inst, unsigned N) const {
    assert(N == 2 && "Invalid number of operands!");
    ARM64_AM::ShiftExtendType ET = getShiftExtendType();
    bool IsSigned = ET == ARM64_AM::SXTW || ET == ARM64_AM::SXTX;
    Inst.addOperand(MCOperand::CreateImm(IsSigned));
    Inst.addOperand(MCOperand::CreateImm(hasShiftExtendAmount()));
  }

  template<int Shift>
  void addMOVZMovAliasOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");

    const MCConstantExpr *CE = cast<MCConstantExpr>(getImm());
    uint64_t Value = CE->getValue();
    Inst.addOperand(MCOperand::CreateImm((Value >> Shift) & 0xffff));
  }

  template<int Shift>
  void addMOVNMovAliasOperands(MCInst &Inst, unsigned N) const {
    assert(N == 1 && "Invalid number of operands!");

    const MCConstantExpr *CE = cast<MCConstantExpr>(getImm());
    uint64_t Value = CE->getValue();
    Inst.addOperand(MCOperand::CreateImm((~Value >> Shift) & 0xffff));
  }

  void print(raw_ostream &OS) const override;

  static ARM64Operand *CreateToken(StringRef Str, bool IsSuffix, SMLoc S,
                                   MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_Token, Ctx);
    Op->Tok.Data = Str.data();
    Op->Tok.Length = Str.size();
    Op->Tok.IsSuffix = IsSuffix;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static ARM64Operand *CreateReg(unsigned RegNum, bool isVector, SMLoc S,
                                 SMLoc E, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_Register, Ctx);
    Op->Reg.RegNum = RegNum;
    Op->Reg.isVector = isVector;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreateVectorList(unsigned RegNum, unsigned Count,
                                        unsigned NumElements, char ElementKind,
                                        SMLoc S, SMLoc E, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_VectorList, Ctx);
    Op->VectorList.RegNum = RegNum;
    Op->VectorList.Count = Count;
    Op->VectorList.NumElements = NumElements;
    Op->VectorList.ElementKind = ElementKind;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreateVectorIndex(unsigned Idx, SMLoc S, SMLoc E,
                                         MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_VectorIndex, Ctx);
    Op->VectorIndex.Val = Idx;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreateImm(const MCExpr *Val, SMLoc S, SMLoc E,
                                 MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_Immediate, Ctx);
    Op->Imm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreateShiftedImm(const MCExpr *Val, unsigned ShiftAmount,
                                        SMLoc S, SMLoc E, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_ShiftedImm, Ctx);
    Op->ShiftedImm .Val = Val;
    Op->ShiftedImm.ShiftAmount = ShiftAmount;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreateCondCode(ARM64CC::CondCode Code, SMLoc S, SMLoc E,
                                      MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_CondCode, Ctx);
    Op->CondCode.Code = Code;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreateFPImm(unsigned Val, SMLoc S, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_FPImm, Ctx);
    Op->FPImm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static ARM64Operand *CreateBarrier(unsigned Val, SMLoc S, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_Barrier, Ctx);
    Op->Barrier.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static ARM64Operand *CreateSysReg(StringRef Str, SMLoc S,
                                    uint64_t FeatureBits, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_SysReg, Ctx);
    Op->SysReg.Data = Str.data();
    Op->SysReg.Length = Str.size();
    Op->SysReg.FeatureBits = FeatureBits;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static ARM64Operand *CreateSysCR(unsigned Val, SMLoc S, SMLoc E,
                                   MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_SysCR, Ctx);
    Op->SysCRImm.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }

  static ARM64Operand *CreatePrefetch(unsigned Val, SMLoc S, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_Prefetch, Ctx);
    Op->Prefetch.Val = Val;
    Op->StartLoc = S;
    Op->EndLoc = S;
    return Op;
  }

  static ARM64Operand *CreateShiftExtend(ARM64_AM::ShiftExtendType ShOp,
                                         unsigned Val, bool HasExplicitAmount,
                                         SMLoc S, SMLoc E, MCContext &Ctx) {
    ARM64Operand *Op = new ARM64Operand(k_ShiftExtend, Ctx);
    Op->ShiftExtend.Type = ShOp;
    Op->ShiftExtend.Amount = Val;
    Op->ShiftExtend.HasExplicitAmount = HasExplicitAmount;
    Op->StartLoc = S;
    Op->EndLoc = E;
    return Op;
  }
};

} // end anonymous namespace.

void ARM64Operand::print(raw_ostream &OS) const {
  switch (Kind) {
  case k_FPImm:
    OS << "<fpimm " << getFPImm() << "(" << ARM64_AM::getFPImmFloat(getFPImm())
       << ") >";
    break;
  case k_Barrier: {
    bool Valid;
    StringRef Name = ARM64DB::DBarrierMapper().toString(getBarrier(), Valid);
    if (Valid)
      OS << "<barrier " << Name << ">";
    else
      OS << "<barrier invalid #" << getBarrier() << ">";
    break;
  }
  case k_Immediate:
    getImm()->print(OS);
    break;
  case k_ShiftedImm: {
    unsigned Shift = getShiftedImmShift();
    OS << "<shiftedimm ";
    getShiftedImmVal()->print(OS);
    OS << ", lsl #" << ARM64_AM::getShiftValue(Shift) << ">";
    break;
  }
  case k_CondCode:
    OS << "<condcode " << getCondCode() << ">";
    break;
  case k_Register:
    OS << "<register " << getReg() << ">";
    break;
  case k_VectorList: {
    OS << "<vectorlist ";
    unsigned Reg = getVectorListStart();
    for (unsigned i = 0, e = getVectorListCount(); i != e; ++i)
      OS << Reg + i << " ";
    OS << ">";
    break;
  }
  case k_VectorIndex:
    OS << "<vectorindex " << getVectorIndex() << ">";
    break;
  case k_SysReg:
    OS << "<sysreg: " << getSysReg() << '>';
    break;
  case k_Token:
    OS << "'" << getToken() << "'";
    break;
  case k_SysCR:
    OS << "c" << getSysCR();
    break;
  case k_Prefetch: {
    bool Valid;
    StringRef Name = ARM64PRFM::PRFMMapper().toString(getPrefetch(), Valid);
    if (Valid)
      OS << "<prfop " << Name << ">";
    else
      OS << "<prfop invalid #" << getPrefetch() << ">";
    break;
  }
  case k_ShiftExtend: {
    OS << "<" << ARM64_AM::getShiftExtendName(getShiftExtendType()) << " #"
       << getShiftExtendAmount();
    if (!hasShiftExtendAmount())
      OS << "<imp>";
    OS << '>';
    break;
  }
  }
}

/// @name Auto-generated Match Functions
/// {

static unsigned MatchRegisterName(StringRef Name);

/// }

static unsigned matchVectorRegName(StringRef Name) {
  return StringSwitch<unsigned>(Name)
      .Case("v0", ARM64::Q0)
      .Case("v1", ARM64::Q1)
      .Case("v2", ARM64::Q2)
      .Case("v3", ARM64::Q3)
      .Case("v4", ARM64::Q4)
      .Case("v5", ARM64::Q5)
      .Case("v6", ARM64::Q6)
      .Case("v7", ARM64::Q7)
      .Case("v8", ARM64::Q8)
      .Case("v9", ARM64::Q9)
      .Case("v10", ARM64::Q10)
      .Case("v11", ARM64::Q11)
      .Case("v12", ARM64::Q12)
      .Case("v13", ARM64::Q13)
      .Case("v14", ARM64::Q14)
      .Case("v15", ARM64::Q15)
      .Case("v16", ARM64::Q16)
      .Case("v17", ARM64::Q17)
      .Case("v18", ARM64::Q18)
      .Case("v19", ARM64::Q19)
      .Case("v20", ARM64::Q20)
      .Case("v21", ARM64::Q21)
      .Case("v22", ARM64::Q22)
      .Case("v23", ARM64::Q23)
      .Case("v24", ARM64::Q24)
      .Case("v25", ARM64::Q25)
      .Case("v26", ARM64::Q26)
      .Case("v27", ARM64::Q27)
      .Case("v28", ARM64::Q28)
      .Case("v29", ARM64::Q29)
      .Case("v30", ARM64::Q30)
      .Case("v31", ARM64::Q31)
      .Default(0);
}

static bool isValidVectorKind(StringRef Name) {
  return StringSwitch<bool>(Name.lower())
      .Case(".8b", true)
      .Case(".16b", true)
      .Case(".4h", true)
      .Case(".8h", true)
      .Case(".2s", true)
      .Case(".4s", true)
      .Case(".1d", true)
      .Case(".2d", true)
      .Case(".1q", true)
      // Accept the width neutral ones, too, for verbose syntax. If those
      // aren't used in the right places, the token operand won't match so
      // all will work out.
      .Case(".b", true)
      .Case(".h", true)
      .Case(".s", true)
      .Case(".d", true)
      .Default(false);
}

static void parseValidVectorKind(StringRef Name, unsigned &NumElements,
                                 char &ElementKind) {
  assert(isValidVectorKind(Name));

  ElementKind = Name.lower()[Name.size() - 1];
  NumElements = 0;

  if (Name.size() == 2)
    return;

  // Parse the lane count
  Name = Name.drop_front();
  while (isdigit(Name.front())) {
    NumElements = 10 * NumElements + (Name.front() - '0');
    Name = Name.drop_front();
  }
}

bool ARM64AsmParser::ParseRegister(unsigned &RegNo, SMLoc &StartLoc,
                                   SMLoc &EndLoc) {
  StartLoc = getLoc();
  RegNo = tryParseRegister();
  EndLoc = SMLoc::getFromPointer(getLoc().getPointer() - 1);
  return (RegNo == (unsigned)-1);
}

/// tryParseRegister - Try to parse a register name. The token must be an
/// Identifier when called, and if it is a register name the token is eaten and
/// the register is added to the operand list.
int ARM64AsmParser::tryParseRegister() {
  const AsmToken &Tok = Parser.getTok();
  assert(Tok.is(AsmToken::Identifier) && "Token is not an Identifier");

  std::string lowerCase = Tok.getString().lower();
  unsigned RegNum = MatchRegisterName(lowerCase);
  // Also handle a few aliases of registers.
  if (RegNum == 0)
    RegNum = StringSwitch<unsigned>(lowerCase)
                 .Case("fp",  ARM64::FP)
                 .Case("lr",  ARM64::LR)
                 .Case("x31", ARM64::XZR)
                 .Case("w31", ARM64::WZR)
                 .Default(0);

  if (RegNum == 0)
    return -1;

  Parser.Lex(); // Eat identifier token.
  return RegNum;
}

/// tryMatchVectorRegister - Try to parse a vector register name with optional
/// kind specifier. If it is a register specifier, eat the token and return it.
int ARM64AsmParser::tryMatchVectorRegister(StringRef &Kind, bool expected) {
  if (Parser.getTok().isNot(AsmToken::Identifier)) {
    TokError("vector register expected");
    return -1;
  }

  StringRef Name = Parser.getTok().getString();
  // If there is a kind specifier, it's separated from the register name by
  // a '.'.
  size_t Start = 0, Next = Name.find('.');
  StringRef Head = Name.slice(Start, Next);
  unsigned RegNum = matchVectorRegName(Head);
  if (RegNum) {
    if (Next != StringRef::npos) {
      Kind = Name.slice(Next, StringRef::npos);
      if (!isValidVectorKind(Kind)) {
        TokError("invalid vector kind qualifier");
        return -1;
      }
    }
    Parser.Lex(); // Eat the register token.
    return RegNum;
  }

  if (expected)
    TokError("vector register expected");
  return -1;
}

/// tryParseSysCROperand - Try to parse a system instruction CR operand name.
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseSysCROperand(OperandVector &Operands) {
  SMLoc S = getLoc();

  if (Parser.getTok().isNot(AsmToken::Identifier)) {
    Error(S, "Expected cN operand where 0 <= N <= 15");
    return MatchOperand_ParseFail;
  }

  StringRef Tok = Parser.getTok().getIdentifier();
  if (Tok[0] != 'c' && Tok[0] != 'C') {
    Error(S, "Expected cN operand where 0 <= N <= 15");
    return MatchOperand_ParseFail;
  }

  uint32_t CRNum;
  bool BadNum = Tok.drop_front().getAsInteger(10, CRNum);
  if (BadNum || CRNum > 15) {
    Error(S, "Expected cN operand where 0 <= N <= 15");
    return MatchOperand_ParseFail;
  }

  Parser.Lex(); // Eat identifier token.
  Operands.push_back(ARM64Operand::CreateSysCR(CRNum, S, getLoc(), getContext()));
  return MatchOperand_Success;
}

/// tryParsePrefetch - Try to parse a prefetch operand.
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParsePrefetch(OperandVector &Operands) {
  SMLoc S = getLoc();
  const AsmToken &Tok = Parser.getTok();
  // Either an identifier for named values or a 5-bit immediate.
  bool Hash = Tok.is(AsmToken::Hash);
  if (Hash || Tok.is(AsmToken::Integer)) {
    if (Hash)
      Parser.Lex(); // Eat hash token.
    const MCExpr *ImmVal;
    if (getParser().parseExpression(ImmVal))
      return MatchOperand_ParseFail;

    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(ImmVal);
    if (!MCE) {
      TokError("immediate value expected for prefetch operand");
      return MatchOperand_ParseFail;
    }
    unsigned prfop = MCE->getValue();
    if (prfop > 31) {
      TokError("prefetch operand out of range, [0,31] expected");
      return MatchOperand_ParseFail;
    }

    Operands.push_back(ARM64Operand::CreatePrefetch(prfop, S, getContext()));
    return MatchOperand_Success;
  }

  if (Tok.isNot(AsmToken::Identifier)) {
    TokError("pre-fetch hint expected");
    return MatchOperand_ParseFail;
  }

  bool Valid;
  unsigned prfop = ARM64PRFM::PRFMMapper().fromString(Tok.getString(), Valid);
  if (!Valid) {
    TokError("pre-fetch hint expected");
    return MatchOperand_ParseFail;
  }

  Parser.Lex(); // Eat identifier token.
  Operands.push_back(ARM64Operand::CreatePrefetch(prfop, S, getContext()));
  return MatchOperand_Success;
}

/// tryParseAdrpLabel - Parse and validate a source label for the ADRP
/// instruction.
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseAdrpLabel(OperandVector &Operands) {
  SMLoc S = getLoc();
  const MCExpr *Expr;

  if (Parser.getTok().is(AsmToken::Hash)) {
    Parser.Lex(); // Eat hash token.
  }

  if (parseSymbolicImmVal(Expr))
    return MatchOperand_ParseFail;

  ARM64MCExpr::VariantKind ELFRefKind;
  MCSymbolRefExpr::VariantKind DarwinRefKind;
  int64_t Addend;
  if (classifySymbolRef(Expr, ELFRefKind, DarwinRefKind, Addend)) {
    if (DarwinRefKind == MCSymbolRefExpr::VK_None &&
        ELFRefKind == ARM64MCExpr::VK_INVALID) {
      // No modifier was specified at all; this is the syntax for an ELF basic
      // ADRP relocation (unfortunately).
      Expr = ARM64MCExpr::Create(Expr, ARM64MCExpr::VK_ABS_PAGE, getContext());
    } else if ((DarwinRefKind == MCSymbolRefExpr::VK_GOTPAGE ||
                DarwinRefKind == MCSymbolRefExpr::VK_TLVPPAGE) &&
               Addend != 0) {
      Error(S, "gotpage label reference not allowed an addend");
      return MatchOperand_ParseFail;
    } else if (DarwinRefKind != MCSymbolRefExpr::VK_PAGE &&
               DarwinRefKind != MCSymbolRefExpr::VK_GOTPAGE &&
               DarwinRefKind != MCSymbolRefExpr::VK_TLVPPAGE &&
               ELFRefKind != ARM64MCExpr::VK_GOT_PAGE &&
               ELFRefKind != ARM64MCExpr::VK_GOTTPREL_PAGE &&
               ELFRefKind != ARM64MCExpr::VK_TLSDESC_PAGE) {
      // The operand must be an @page or @gotpage qualified symbolref.
      Error(S, "page or gotpage label reference expected");
      return MatchOperand_ParseFail;
    }
  }

  // We have either a label reference possibly with addend or an immediate. The
  // addend is a raw value here. The linker will adjust it to only reference the
  // page.
  SMLoc E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
  Operands.push_back(ARM64Operand::CreateImm(Expr, S, E, getContext()));

  return MatchOperand_Success;
}

/// tryParseAdrLabel - Parse and validate a source label for the ADR
/// instruction.
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseAdrLabel(OperandVector &Operands) {
  SMLoc S = getLoc();
  const MCExpr *Expr;

  if (Parser.getTok().is(AsmToken::Hash)) {
    Parser.Lex(); // Eat hash token.
  }

  if (getParser().parseExpression(Expr))
    return MatchOperand_ParseFail;

  SMLoc E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
  Operands.push_back(ARM64Operand::CreateImm(Expr, S, E, getContext()));

  return MatchOperand_Success;
}

/// tryParseFPImm - A floating point immediate expression operand.
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseFPImm(OperandVector &Operands) {
  SMLoc S = getLoc();

  bool Hash = false;
  if (Parser.getTok().is(AsmToken::Hash)) {
    Parser.Lex(); // Eat '#'
    Hash = true;
  }

  // Handle negation, as that still comes through as a separate token.
  bool isNegative = false;
  if (Parser.getTok().is(AsmToken::Minus)) {
    isNegative = true;
    Parser.Lex();
  }
  const AsmToken &Tok = Parser.getTok();
  if (Tok.is(AsmToken::Real)) {
    APFloat RealVal(APFloat::IEEEdouble, Tok.getString());
    uint64_t IntVal = RealVal.bitcastToAPInt().getZExtValue();
    // If we had a '-' in front, toggle the sign bit.
    IntVal ^= (uint64_t)isNegative << 63;
    int Val = ARM64_AM::getFP64Imm(APInt(64, IntVal));
    Parser.Lex(); // Eat the token.
    // Check for out of range values. As an exception, we let Zero through,
    // as we handle that special case in post-processing before matching in
    // order to use the zero register for it.
    if (Val == -1 && !RealVal.isZero()) {
      TokError("expected compatible register or floating-point constant");
      return MatchOperand_ParseFail;
    }
    Operands.push_back(ARM64Operand::CreateFPImm(Val, S, getContext()));
    return MatchOperand_Success;
  }
  if (Tok.is(AsmToken::Integer)) {
    int64_t Val;
    if (!isNegative && Tok.getString().startswith("0x")) {
      Val = Tok.getIntVal();
      if (Val > 255 || Val < 0) {
        TokError("encoded floating point value out of range");
        return MatchOperand_ParseFail;
      }
    } else {
      APFloat RealVal(APFloat::IEEEdouble, Tok.getString());
      uint64_t IntVal = RealVal.bitcastToAPInt().getZExtValue();
      // If we had a '-' in front, toggle the sign bit.
      IntVal ^= (uint64_t)isNegative << 63;
      Val = ARM64_AM::getFP64Imm(APInt(64, IntVal));
    }
    Parser.Lex(); // Eat the token.
    Operands.push_back(ARM64Operand::CreateFPImm(Val, S, getContext()));
    return MatchOperand_Success;
  }

  if (!Hash)
    return MatchOperand_NoMatch;

  TokError("invalid floating point immediate");
  return MatchOperand_ParseFail;
}

/// tryParseAddSubImm - Parse ADD/SUB shifted immediate operand
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseAddSubImm(OperandVector &Operands) {
  SMLoc S = getLoc();

  if (Parser.getTok().is(AsmToken::Hash))
    Parser.Lex(); // Eat '#'
  else if (Parser.getTok().isNot(AsmToken::Integer))
    // Operand should start from # or should be integer, emit error otherwise.
    return MatchOperand_NoMatch;

  const MCExpr *Imm;
  if (parseSymbolicImmVal(Imm))
    return MatchOperand_ParseFail;
  else if (Parser.getTok().isNot(AsmToken::Comma)) {
    uint64_t ShiftAmount = 0;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(Imm);
    if (MCE) {
      int64_t Val = MCE->getValue();
      if (Val > 0xfff && (Val & 0xfff) == 0) {
        Imm = MCConstantExpr::Create(Val >> 12, getContext());
        ShiftAmount = 12;
      }
    }
    SMLoc E = Parser.getTok().getLoc();
    Operands.push_back(ARM64Operand::CreateShiftedImm(Imm, ShiftAmount, S, E,
                                                      getContext()));
    return MatchOperand_Success;
  }

  // Eat ','
  Parser.Lex();

  // The optional operand must be "lsl #N" where N is non-negative.
  if (!Parser.getTok().is(AsmToken::Identifier) ||
      !Parser.getTok().getIdentifier().equals_lower("lsl")) {
    Error(Parser.getTok().getLoc(), "only 'lsl #+N' valid after immediate");
    return MatchOperand_ParseFail;
  }

  // Eat 'lsl'
  Parser.Lex();

  if (Parser.getTok().is(AsmToken::Hash)) {
    Parser.Lex();
  }

  if (Parser.getTok().isNot(AsmToken::Integer)) {
    Error(Parser.getTok().getLoc(), "only 'lsl #+N' valid after immediate");
    return MatchOperand_ParseFail;
  }

  int64_t ShiftAmount = Parser.getTok().getIntVal();

  if (ShiftAmount < 0) {
    Error(Parser.getTok().getLoc(), "positive shift amount required");
    return MatchOperand_ParseFail;
  }
  Parser.Lex(); // Eat the number

  SMLoc E = Parser.getTok().getLoc();
  Operands.push_back(ARM64Operand::CreateShiftedImm(Imm, ShiftAmount,
                                                    S, E, getContext()));
  return MatchOperand_Success;
}

/// parseCondCodeString - Parse a Condition Code string.
ARM64CC::CondCode ARM64AsmParser::parseCondCodeString(StringRef Cond) {
  ARM64CC::CondCode CC = StringSwitch<ARM64CC::CondCode>(Cond.lower())
                    .Case("eq", ARM64CC::EQ)
                    .Case("ne", ARM64CC::NE)
                    .Case("cs", ARM64CC::HS)
                    .Case("hs", ARM64CC::HS)
                    .Case("cc", ARM64CC::LO)
                    .Case("lo", ARM64CC::LO)
                    .Case("mi", ARM64CC::MI)
                    .Case("pl", ARM64CC::PL)
                    .Case("vs", ARM64CC::VS)
                    .Case("vc", ARM64CC::VC)
                    .Case("hi", ARM64CC::HI)
                    .Case("ls", ARM64CC::LS)
                    .Case("ge", ARM64CC::GE)
                    .Case("lt", ARM64CC::LT)
                    .Case("gt", ARM64CC::GT)
                    .Case("le", ARM64CC::LE)
                    .Case("al", ARM64CC::AL)
                    .Case("nv", ARM64CC::NV)
                    .Default(ARM64CC::Invalid);
  return CC;
}

/// parseCondCode - Parse a Condition Code operand.
bool ARM64AsmParser::parseCondCode(OperandVector &Operands,
                                   bool invertCondCode) {
  SMLoc S = getLoc();
  const AsmToken &Tok = Parser.getTok();
  assert(Tok.is(AsmToken::Identifier) && "Token is not an Identifier");

  StringRef Cond = Tok.getString();
  ARM64CC::CondCode CC = parseCondCodeString(Cond);
  if (CC == ARM64CC::Invalid)
    return TokError("invalid condition code");
  Parser.Lex(); // Eat identifier token.

  if (invertCondCode)
    CC = ARM64CC::getInvertedCondCode(ARM64CC::CondCode(CC));

  Operands.push_back(
      ARM64Operand::CreateCondCode(CC, S, getLoc(), getContext()));
  return false;
}

/// tryParseOptionalShift - Some operands take an optional shift argument. Parse
/// them if present.
ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseOptionalShiftExtend(OperandVector &Operands) {
  const AsmToken &Tok = Parser.getTok();
  std::string LowerID = Tok.getString().lower();
  ARM64_AM::ShiftExtendType ShOp =
      StringSwitch<ARM64_AM::ShiftExtendType>(LowerID)
          .Case("lsl", ARM64_AM::LSL)
          .Case("lsr", ARM64_AM::LSR)
          .Case("asr", ARM64_AM::ASR)
          .Case("ror", ARM64_AM::ROR)
          .Case("msl", ARM64_AM::MSL)
          .Case("uxtb", ARM64_AM::UXTB)
          .Case("uxth", ARM64_AM::UXTH)
          .Case("uxtw", ARM64_AM::UXTW)
          .Case("uxtx", ARM64_AM::UXTX)
          .Case("sxtb", ARM64_AM::SXTB)
          .Case("sxth", ARM64_AM::SXTH)
          .Case("sxtw", ARM64_AM::SXTW)
          .Case("sxtx", ARM64_AM::SXTX)
          .Default(ARM64_AM::InvalidShiftExtend);

  if (ShOp == ARM64_AM::InvalidShiftExtend)
    return MatchOperand_NoMatch;

  SMLoc S = Tok.getLoc();
  Parser.Lex();

  bool Hash = getLexer().is(AsmToken::Hash);
  if (!Hash && getLexer().isNot(AsmToken::Integer)) {
    if (ShOp == ARM64_AM::LSL || ShOp == ARM64_AM::LSR ||
        ShOp == ARM64_AM::ASR || ShOp == ARM64_AM::ROR ||
        ShOp == ARM64_AM::MSL) {
      // We expect a number here.
      TokError("expected #imm after shift specifier");
      return MatchOperand_ParseFail;
    }

    // "extend" type operatoins don't need an immediate, #0 is implicit.
    SMLoc E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
    Operands.push_back(
        ARM64Operand::CreateShiftExtend(ShOp, 0, false, S, E, getContext()));
    return MatchOperand_Success;
  }

  if (Hash)
    Parser.Lex(); // Eat the '#'.

  // Make sure we do actually have a number
  if (!Parser.getTok().is(AsmToken::Integer)) {
    Error(Parser.getTok().getLoc(),
          "expected integer shift amount");
    return MatchOperand_ParseFail;
  }

  const MCExpr *ImmVal;
  if (getParser().parseExpression(ImmVal))
    return MatchOperand_ParseFail;

  const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(ImmVal);
  if (!MCE) {
    TokError("expected #imm after shift specifier");
    return MatchOperand_ParseFail;
  }

  SMLoc E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
  Operands.push_back(ARM64Operand::CreateShiftExtend(ShOp, MCE->getValue(),
                                                     true, S, E, getContext()));
  return MatchOperand_Success;
}

/// parseSysAlias - The IC, DC, AT, and TLBI instructions are simple aliases for
/// the SYS instruction. Parse them specially so that we create a SYS MCInst.
bool ARM64AsmParser::parseSysAlias(StringRef Name, SMLoc NameLoc,
                                   OperandVector &Operands) {
  if (Name.find('.') != StringRef::npos)
    return TokError("invalid operand");

  Mnemonic = Name;
  Operands.push_back(
      ARM64Operand::CreateToken("sys", false, NameLoc, getContext()));

  const AsmToken &Tok = Parser.getTok();
  StringRef Op = Tok.getString();
  SMLoc S = Tok.getLoc();

  const MCExpr *Expr = nullptr;

#define SYS_ALIAS(op1, Cn, Cm, op2)                                            \
  do {                                                                         \
    Expr = MCConstantExpr::Create(op1, getContext());                          \
    Operands.push_back(                                                        \
        ARM64Operand::CreateImm(Expr, S, getLoc(), getContext()));             \
    Operands.push_back(                                                        \
        ARM64Operand::CreateSysCR(Cn, S, getLoc(), getContext()));             \
    Operands.push_back(                                                        \
        ARM64Operand::CreateSysCR(Cm, S, getLoc(), getContext()));             \
    Expr = MCConstantExpr::Create(op2, getContext());                          \
    Operands.push_back(                                                        \
        ARM64Operand::CreateImm(Expr, S, getLoc(), getContext()));             \
  } while (0)

  if (Mnemonic == "ic") {
    if (!Op.compare_lower("ialluis")) {
      // SYS #0, C7, C1, #0
      SYS_ALIAS(0, 7, 1, 0);
    } else if (!Op.compare_lower("iallu")) {
      // SYS #0, C7, C5, #0
      SYS_ALIAS(0, 7, 5, 0);
    } else if (!Op.compare_lower("ivau")) {
      // SYS #3, C7, C5, #1
      SYS_ALIAS(3, 7, 5, 1);
    } else {
      return TokError("invalid operand for IC instruction");
    }
  } else if (Mnemonic == "dc") {
    if (!Op.compare_lower("zva")) {
      // SYS #3, C7, C4, #1
      SYS_ALIAS(3, 7, 4, 1);
    } else if (!Op.compare_lower("ivac")) {
      // SYS #3, C7, C6, #1
      SYS_ALIAS(0, 7, 6, 1);
    } else if (!Op.compare_lower("isw")) {
      // SYS #0, C7, C6, #2
      SYS_ALIAS(0, 7, 6, 2);
    } else if (!Op.compare_lower("cvac")) {
      // SYS #3, C7, C10, #1
      SYS_ALIAS(3, 7, 10, 1);
    } else if (!Op.compare_lower("csw")) {
      // SYS #0, C7, C10, #2
      SYS_ALIAS(0, 7, 10, 2);
    } else if (!Op.compare_lower("cvau")) {
      // SYS #3, C7, C11, #1
      SYS_ALIAS(3, 7, 11, 1);
    } else if (!Op.compare_lower("civac")) {
      // SYS #3, C7, C14, #1
      SYS_ALIAS(3, 7, 14, 1);
    } else if (!Op.compare_lower("cisw")) {
      // SYS #0, C7, C14, #2
      SYS_ALIAS(0, 7, 14, 2);
    } else {
      return TokError("invalid operand for DC instruction");
    }
  } else if (Mnemonic == "at") {
    if (!Op.compare_lower("s1e1r")) {
      // SYS #0, C7, C8, #0
      SYS_ALIAS(0, 7, 8, 0);
    } else if (!Op.compare_lower("s1e2r")) {
      // SYS #4, C7, C8, #0
      SYS_ALIAS(4, 7, 8, 0);
    } else if (!Op.compare_lower("s1e3r")) {
      // SYS #6, C7, C8, #0
      SYS_ALIAS(6, 7, 8, 0);
    } else if (!Op.compare_lower("s1e1w")) {
      // SYS #0, C7, C8, #1
      SYS_ALIAS(0, 7, 8, 1);
    } else if (!Op.compare_lower("s1e2w")) {
      // SYS #4, C7, C8, #1
      SYS_ALIAS(4, 7, 8, 1);
    } else if (!Op.compare_lower("s1e3w")) {
      // SYS #6, C7, C8, #1
      SYS_ALIAS(6, 7, 8, 1);
    } else if (!Op.compare_lower("s1e0r")) {
      // SYS #0, C7, C8, #3
      SYS_ALIAS(0, 7, 8, 2);
    } else if (!Op.compare_lower("s1e0w")) {
      // SYS #0, C7, C8, #3
      SYS_ALIAS(0, 7, 8, 3);
    } else if (!Op.compare_lower("s12e1r")) {
      // SYS #4, C7, C8, #4
      SYS_ALIAS(4, 7, 8, 4);
    } else if (!Op.compare_lower("s12e1w")) {
      // SYS #4, C7, C8, #5
      SYS_ALIAS(4, 7, 8, 5);
    } else if (!Op.compare_lower("s12e0r")) {
      // SYS #4, C7, C8, #6
      SYS_ALIAS(4, 7, 8, 6);
    } else if (!Op.compare_lower("s12e0w")) {
      // SYS #4, C7, C8, #7
      SYS_ALIAS(4, 7, 8, 7);
    } else {
      return TokError("invalid operand for AT instruction");
    }
  } else if (Mnemonic == "tlbi") {
    if (!Op.compare_lower("vmalle1is")) {
      // SYS #0, C8, C3, #0
      SYS_ALIAS(0, 8, 3, 0);
    } else if (!Op.compare_lower("alle2is")) {
      // SYS #4, C8, C3, #0
      SYS_ALIAS(4, 8, 3, 0);
    } else if (!Op.compare_lower("alle3is")) {
      // SYS #6, C8, C3, #0
      SYS_ALIAS(6, 8, 3, 0);
    } else if (!Op.compare_lower("vae1is")) {
      // SYS #0, C8, C3, #1
      SYS_ALIAS(0, 8, 3, 1);
    } else if (!Op.compare_lower("vae2is")) {
      // SYS #4, C8, C3, #1
      SYS_ALIAS(4, 8, 3, 1);
    } else if (!Op.compare_lower("vae3is")) {
      // SYS #6, C8, C3, #1
      SYS_ALIAS(6, 8, 3, 1);
    } else if (!Op.compare_lower("aside1is")) {
      // SYS #0, C8, C3, #2
      SYS_ALIAS(0, 8, 3, 2);
    } else if (!Op.compare_lower("vaae1is")) {
      // SYS #0, C8, C3, #3
      SYS_ALIAS(0, 8, 3, 3);
    } else if (!Op.compare_lower("alle1is")) {
      // SYS #4, C8, C3, #4
      SYS_ALIAS(4, 8, 3, 4);
    } else if (!Op.compare_lower("vale1is")) {
      // SYS #0, C8, C3, #5
      SYS_ALIAS(0, 8, 3, 5);
    } else if (!Op.compare_lower("vaale1is")) {
      // SYS #0, C8, C3, #7
      SYS_ALIAS(0, 8, 3, 7);
    } else if (!Op.compare_lower("vmalle1")) {
      // SYS #0, C8, C7, #0
      SYS_ALIAS(0, 8, 7, 0);
    } else if (!Op.compare_lower("alle2")) {
      // SYS #4, C8, C7, #0
      SYS_ALIAS(4, 8, 7, 0);
    } else if (!Op.compare_lower("vale2is")) {
      // SYS #4, C8, C3, #5
      SYS_ALIAS(4, 8, 3, 5);
    } else if (!Op.compare_lower("vale3is")) {
      // SYS #6, C8, C3, #5
      SYS_ALIAS(6, 8, 3, 5);
    } else if (!Op.compare_lower("alle3")) {
      // SYS #6, C8, C7, #0
      SYS_ALIAS(6, 8, 7, 0);
    } else if (!Op.compare_lower("vae1")) {
      // SYS #0, C8, C7, #1
      SYS_ALIAS(0, 8, 7, 1);
    } else if (!Op.compare_lower("vae2")) {
      // SYS #4, C8, C7, #1
      SYS_ALIAS(4, 8, 7, 1);
    } else if (!Op.compare_lower("vae3")) {
      // SYS #6, C8, C7, #1
      SYS_ALIAS(6, 8, 7, 1);
    } else if (!Op.compare_lower("aside1")) {
      // SYS #0, C8, C7, #2
      SYS_ALIAS(0, 8, 7, 2);
    } else if (!Op.compare_lower("vaae1")) {
      // SYS #0, C8, C7, #3
      SYS_ALIAS(0, 8, 7, 3);
    } else if (!Op.compare_lower("alle1")) {
      // SYS #4, C8, C7, #4
      SYS_ALIAS(4, 8, 7, 4);
    } else if (!Op.compare_lower("vale1")) {
      // SYS #0, C8, C7, #5
      SYS_ALIAS(0, 8, 7, 5);
    } else if (!Op.compare_lower("vale2")) {
      // SYS #4, C8, C7, #5
      SYS_ALIAS(4, 8, 7, 5);
    } else if (!Op.compare_lower("vale3")) {
      // SYS #6, C8, C7, #5
      SYS_ALIAS(6, 8, 7, 5);
    } else if (!Op.compare_lower("vaale1")) {
      // SYS #0, C8, C7, #7
      SYS_ALIAS(0, 8, 7, 7);
    } else if (!Op.compare_lower("ipas2e1")) {
      // SYS #4, C8, C4, #1
      SYS_ALIAS(4, 8, 4, 1);
    } else if (!Op.compare_lower("ipas2le1")) {
      // SYS #4, C8, C4, #5
      SYS_ALIAS(4, 8, 4, 5);
    } else if (!Op.compare_lower("ipas2e1is")) {
      // SYS #4, C8, C4, #1
      SYS_ALIAS(4, 8, 0, 1);
    } else if (!Op.compare_lower("ipas2le1is")) {
      // SYS #4, C8, C4, #5
      SYS_ALIAS(4, 8, 0, 5);
    } else if (!Op.compare_lower("vmalls12e1")) {
      // SYS #4, C8, C7, #6
      SYS_ALIAS(4, 8, 7, 6);
    } else if (!Op.compare_lower("vmalls12e1is")) {
      // SYS #4, C8, C3, #6
      SYS_ALIAS(4, 8, 3, 6);
    } else {
      return TokError("invalid operand for TLBI instruction");
    }
  }

#undef SYS_ALIAS

  Parser.Lex(); // Eat operand.

  bool ExpectRegister = (Op.lower().find("all") == StringRef::npos);
  bool HasRegister = false;

  // Check for the optional register operand.
  if (getLexer().is(AsmToken::Comma)) {
    Parser.Lex(); // Eat comma.

    if (Tok.isNot(AsmToken::Identifier) || parseRegister(Operands))
      return TokError("expected register operand");

    HasRegister = true;
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    Parser.eatToEndOfStatement();
    return TokError("unexpected token in argument list");
  }

  if (ExpectRegister && !HasRegister) {
    return TokError("specified " + Mnemonic + " op requires a register");
  }
  else if (!ExpectRegister && HasRegister) {
    return TokError("specified " + Mnemonic + " op does not use a register");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseBarrierOperand(OperandVector &Operands) {
  const AsmToken &Tok = Parser.getTok();

  // Can be either a #imm style literal or an option name
  bool Hash = Tok.is(AsmToken::Hash);
  if (Hash || Tok.is(AsmToken::Integer)) {
    // Immediate operand.
    if (Hash)
      Parser.Lex(); // Eat the '#'
    const MCExpr *ImmVal;
    SMLoc ExprLoc = getLoc();
    if (getParser().parseExpression(ImmVal))
      return MatchOperand_ParseFail;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(ImmVal);
    if (!MCE) {
      Error(ExprLoc, "immediate value expected for barrier operand");
      return MatchOperand_ParseFail;
    }
    if (MCE->getValue() < 0 || MCE->getValue() > 15) {
      Error(ExprLoc, "barrier operand out of range");
      return MatchOperand_ParseFail;
    }
    Operands.push_back(
        ARM64Operand::CreateBarrier(MCE->getValue(), ExprLoc, getContext()));
    return MatchOperand_Success;
  }

  if (Tok.isNot(AsmToken::Identifier)) {
    TokError("invalid operand for instruction");
    return MatchOperand_ParseFail;
  }

  bool Valid;
  unsigned Opt = ARM64DB::DBarrierMapper().fromString(Tok.getString(), Valid);
  if (!Valid) {
    TokError("invalid barrier option name");
    return MatchOperand_ParseFail;
  }

  // The only valid named option for ISB is 'sy'
  if (Mnemonic == "isb" && Opt != ARM64DB::SY) {
    TokError("'sy' or #imm operand expected");
    return MatchOperand_ParseFail;
  }

  Operands.push_back(ARM64Operand::CreateBarrier(Opt, getLoc(), getContext()));
  Parser.Lex(); // Consume the option

  return MatchOperand_Success;
}

ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseSysReg(OperandVector &Operands) {
  const AsmToken &Tok = Parser.getTok();

  if (Tok.isNot(AsmToken::Identifier))
    return MatchOperand_NoMatch;

  Operands.push_back(ARM64Operand::CreateSysReg(Tok.getString(), getLoc(),
                     STI.getFeatureBits(), getContext()));
  Parser.Lex(); // Eat identifier

  return MatchOperand_Success;
}

/// tryParseVectorRegister - Parse a vector register operand.
bool ARM64AsmParser::tryParseVectorRegister(OperandVector &Operands) {
  if (Parser.getTok().isNot(AsmToken::Identifier))
    return true;

  SMLoc S = getLoc();
  // Check for a vector register specifier first.
  StringRef Kind;
  int64_t Reg = tryMatchVectorRegister(Kind, false);
  if (Reg == -1)
    return true;
  Operands.push_back(
      ARM64Operand::CreateReg(Reg, true, S, getLoc(), getContext()));
  // If there was an explicit qualifier, that goes on as a literal text
  // operand.
  if (!Kind.empty())
    Operands.push_back(ARM64Operand::CreateToken(Kind, false, S, getContext()));

  // If there is an index specifier following the register, parse that too.
  if (Parser.getTok().is(AsmToken::LBrac)) {
    SMLoc SIdx = getLoc();
    Parser.Lex(); // Eat left bracket token.

    const MCExpr *ImmVal;
    if (getParser().parseExpression(ImmVal))
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(ImmVal);
    if (!MCE) {
      TokError("immediate value expected for vector index");
      return false;
    }

    SMLoc E = getLoc();
    if (Parser.getTok().isNot(AsmToken::RBrac)) {
      Error(E, "']' expected");
      return false;
    }

    Parser.Lex(); // Eat right bracket token.

    Operands.push_back(ARM64Operand::CreateVectorIndex(MCE->getValue(), SIdx, E,
                                                       getContext()));
  }

  return false;
}

/// parseRegister - Parse a non-vector register operand.
bool ARM64AsmParser::parseRegister(OperandVector &Operands) {
  SMLoc S = getLoc();
  // Try for a vector register.
  if (!tryParseVectorRegister(Operands))
    return false;

  // Try for a scalar register.
  int64_t Reg = tryParseRegister();
  if (Reg == -1)
    return true;
  Operands.push_back(
      ARM64Operand::CreateReg(Reg, false, S, getLoc(), getContext()));

  // A small number of instructions (FMOVXDhighr, for example) have "[1]"
  // as a string token in the instruction itself.
  if (getLexer().getKind() == AsmToken::LBrac) {
    SMLoc LBracS = getLoc();
    Parser.Lex();
    const AsmToken &Tok = Parser.getTok();
    if (Tok.is(AsmToken::Integer)) {
      SMLoc IntS = getLoc();
      int64_t Val = Tok.getIntVal();
      if (Val == 1) {
        Parser.Lex();
        if (getLexer().getKind() == AsmToken::RBrac) {
          SMLoc RBracS = getLoc();
          Parser.Lex();
          Operands.push_back(
              ARM64Operand::CreateToken("[", false, LBracS, getContext()));
          Operands.push_back(
              ARM64Operand::CreateToken("1", false, IntS, getContext()));
          Operands.push_back(
              ARM64Operand::CreateToken("]", false, RBracS, getContext()));
          return false;
        }
      }
    }
  }

  return false;
}

bool ARM64AsmParser::parseSymbolicImmVal(const MCExpr *&ImmVal) {
  bool HasELFModifier = false;
  ARM64MCExpr::VariantKind RefKind;

  if (Parser.getTok().is(AsmToken::Colon)) {
    Parser.Lex(); // Eat ':"
    HasELFModifier = true;

    if (Parser.getTok().isNot(AsmToken::Identifier)) {
      Error(Parser.getTok().getLoc(),
            "expect relocation specifier in operand after ':'");
      return true;
    }

    std::string LowerCase = Parser.getTok().getIdentifier().lower();
    RefKind = StringSwitch<ARM64MCExpr::VariantKind>(LowerCase)
                  .Case("lo12", ARM64MCExpr::VK_LO12)
                  .Case("abs_g3", ARM64MCExpr::VK_ABS_G3)
                  .Case("abs_g2", ARM64MCExpr::VK_ABS_G2)
                  .Case("abs_g2_s", ARM64MCExpr::VK_ABS_G2_S)
                  .Case("abs_g2_nc", ARM64MCExpr::VK_ABS_G2_NC)
                  .Case("abs_g1", ARM64MCExpr::VK_ABS_G1)
                  .Case("abs_g1_s", ARM64MCExpr::VK_ABS_G1_S)
                  .Case("abs_g1_nc", ARM64MCExpr::VK_ABS_G1_NC)
                  .Case("abs_g0", ARM64MCExpr::VK_ABS_G0)
                  .Case("abs_g0_s", ARM64MCExpr::VK_ABS_G0_S)
                  .Case("abs_g0_nc", ARM64MCExpr::VK_ABS_G0_NC)
                  .Case("dtprel_g2", ARM64MCExpr::VK_DTPREL_G2)
                  .Case("dtprel_g1", ARM64MCExpr::VK_DTPREL_G1)
                  .Case("dtprel_g1_nc", ARM64MCExpr::VK_DTPREL_G1_NC)
                  .Case("dtprel_g0", ARM64MCExpr::VK_DTPREL_G0)
                  .Case("dtprel_g0_nc", ARM64MCExpr::VK_DTPREL_G0_NC)
                  .Case("dtprel_hi12", ARM64MCExpr::VK_DTPREL_HI12)
                  .Case("dtprel_lo12", ARM64MCExpr::VK_DTPREL_LO12)
                  .Case("dtprel_lo12_nc", ARM64MCExpr::VK_DTPREL_LO12_NC)
                  .Case("tprel_g2", ARM64MCExpr::VK_TPREL_G2)
                  .Case("tprel_g1", ARM64MCExpr::VK_TPREL_G1)
                  .Case("tprel_g1_nc", ARM64MCExpr::VK_TPREL_G1_NC)
                  .Case("tprel_g0", ARM64MCExpr::VK_TPREL_G0)
                  .Case("tprel_g0_nc", ARM64MCExpr::VK_TPREL_G0_NC)
                  .Case("tprel_hi12", ARM64MCExpr::VK_TPREL_HI12)
                  .Case("tprel_lo12", ARM64MCExpr::VK_TPREL_LO12)
                  .Case("tprel_lo12_nc", ARM64MCExpr::VK_TPREL_LO12_NC)
                  .Case("tlsdesc_lo12", ARM64MCExpr::VK_TLSDESC_LO12)
                  .Case("got", ARM64MCExpr::VK_GOT_PAGE)
                  .Case("got_lo12", ARM64MCExpr::VK_GOT_LO12)
                  .Case("gottprel", ARM64MCExpr::VK_GOTTPREL_PAGE)
                  .Case("gottprel_lo12", ARM64MCExpr::VK_GOTTPREL_LO12_NC)
                  .Case("gottprel_g1", ARM64MCExpr::VK_GOTTPREL_G1)
                  .Case("gottprel_g0_nc", ARM64MCExpr::VK_GOTTPREL_G0_NC)
                  .Case("tlsdesc", ARM64MCExpr::VK_TLSDESC_PAGE)
                  .Default(ARM64MCExpr::VK_INVALID);

    if (RefKind == ARM64MCExpr::VK_INVALID) {
      Error(Parser.getTok().getLoc(),
            "expect relocation specifier in operand after ':'");
      return true;
    }

    Parser.Lex(); // Eat identifier

    if (Parser.getTok().isNot(AsmToken::Colon)) {
      Error(Parser.getTok().getLoc(), "expect ':' after relocation specifier");
      return true;
    }
    Parser.Lex(); // Eat ':'
  }

  if (getParser().parseExpression(ImmVal))
    return true;

  if (HasELFModifier)
    ImmVal = ARM64MCExpr::Create(ImmVal, RefKind, getContext());

  return false;
}

/// parseVectorList - Parse a vector list operand for AdvSIMD instructions.
bool ARM64AsmParser::parseVectorList(OperandVector &Operands) {
  assert(Parser.getTok().is(AsmToken::LCurly) && "Token is not a Left Bracket");
  SMLoc S = getLoc();
  Parser.Lex(); // Eat left bracket token.
  StringRef Kind;
  int64_t FirstReg = tryMatchVectorRegister(Kind, true);
  if (FirstReg == -1)
    return true;
  int64_t PrevReg = FirstReg;
  unsigned Count = 1;

  if (Parser.getTok().is(AsmToken::Minus)) {
    Parser.Lex(); // Eat the minus.

    SMLoc Loc = getLoc();
    StringRef NextKind;
    int64_t Reg = tryMatchVectorRegister(NextKind, true);
    if (Reg == -1)
      return true;
    // Any Kind suffices must match on all regs in the list.
    if (Kind != NextKind)
      return Error(Loc, "mismatched register size suffix");

    unsigned Space = (PrevReg < Reg) ? (Reg - PrevReg) : (Reg + 32 - PrevReg);

    if (Space == 0 || Space > 3) {
      return Error(Loc, "invalid number of vectors");
    }

    Count += Space;
  }
  else {
    while (Parser.getTok().is(AsmToken::Comma)) {
      Parser.Lex(); // Eat the comma token.

      SMLoc Loc = getLoc();
      StringRef NextKind;
      int64_t Reg = tryMatchVectorRegister(NextKind, true);
      if (Reg == -1)
        return true;
      // Any Kind suffices must match on all regs in the list.
      if (Kind != NextKind)
        return Error(Loc, "mismatched register size suffix");

      // Registers must be incremental (with wraparound at 31)
      if (getContext().getRegisterInfo()->getEncodingValue(Reg) !=
          (getContext().getRegisterInfo()->getEncodingValue(PrevReg) + 1) % 32)
       return Error(Loc, "registers must be sequential");

      PrevReg = Reg;
      ++Count;
    }
  }

  if (Parser.getTok().isNot(AsmToken::RCurly))
    return Error(getLoc(), "'}' expected");
  Parser.Lex(); // Eat the '}' token.

  if (Count > 4)
    return Error(S, "invalid number of vectors");

  unsigned NumElements = 0;
  char ElementKind = 0;
  if (!Kind.empty())
    parseValidVectorKind(Kind, NumElements, ElementKind);

  Operands.push_back(ARM64Operand::CreateVectorList(
      FirstReg, Count, NumElements, ElementKind, S, getLoc(), getContext()));

  // If there is an index specifier following the list, parse that too.
  if (Parser.getTok().is(AsmToken::LBrac)) {
    SMLoc SIdx = getLoc();
    Parser.Lex(); // Eat left bracket token.

    const MCExpr *ImmVal;
    if (getParser().parseExpression(ImmVal))
      return false;
    const MCConstantExpr *MCE = dyn_cast<MCConstantExpr>(ImmVal);
    if (!MCE) {
      TokError("immediate value expected for vector index");
      return false;
    }

    SMLoc E = getLoc();
    if (Parser.getTok().isNot(AsmToken::RBrac)) {
      Error(E, "']' expected");
      return false;
    }

    Parser.Lex(); // Eat right bracket token.

    Operands.push_back(ARM64Operand::CreateVectorIndex(MCE->getValue(), SIdx, E,
                                                       getContext()));
  }
  return false;
}

ARM64AsmParser::OperandMatchResultTy
ARM64AsmParser::tryParseGPR64sp0Operand(OperandVector &Operands) {
  const AsmToken &Tok = Parser.getTok();
  if (!Tok.is(AsmToken::Identifier))
    return MatchOperand_NoMatch;

  unsigned RegNum = MatchRegisterName(Tok.getString().lower());

  MCContext &Ctx = getContext();
  const MCRegisterInfo *RI = Ctx.getRegisterInfo();
  if (!RI->getRegClass(ARM64::GPR64spRegClassID).contains(RegNum))
    return MatchOperand_NoMatch;

  SMLoc S = getLoc();
  Parser.Lex(); // Eat register

  if (Parser.getTok().isNot(AsmToken::Comma)) {
    Operands.push_back(ARM64Operand::CreateReg(RegNum, false, S, getLoc(), Ctx));
    return MatchOperand_Success;
  }
  Parser.Lex(); // Eat comma.

  if (Parser.getTok().is(AsmToken::Hash))
    Parser.Lex(); // Eat hash

  if (Parser.getTok().isNot(AsmToken::Integer)) {
    Error(getLoc(), "index must be absent or #0");
    return MatchOperand_ParseFail;
  }

  const MCExpr *ImmVal;
  if (Parser.parseExpression(ImmVal) || !isa<MCConstantExpr>(ImmVal) ||
      cast<MCConstantExpr>(ImmVal)->getValue() != 0) {
    Error(getLoc(), "index must be absent or #0");
    return MatchOperand_ParseFail;
  }

  Operands.push_back(ARM64Operand::CreateReg(RegNum, false, S, getLoc(), Ctx));
  return MatchOperand_Success;
}

/// parseOperand - Parse a arm instruction operand.  For now this parses the
/// operand regardless of the mnemonic.
bool ARM64AsmParser::parseOperand(OperandVector &Operands, bool isCondCode,
                                  bool invertCondCode) {
  // Check if the current operand has a custom associated parser, if so, try to
  // custom parse the operand, or fallback to the general approach.
  OperandMatchResultTy ResTy = MatchOperandParserImpl(Operands, Mnemonic);
  if (ResTy == MatchOperand_Success)
    return false;
  // If there wasn't a custom match, try the generic matcher below. Otherwise,
  // there was a match, but an error occurred, in which case, just return that
  // the operand parsing failed.
  if (ResTy == MatchOperand_ParseFail)
    return true;

  // Nothing custom, so do general case parsing.
  SMLoc S, E;
  switch (getLexer().getKind()) {
  default: {
    SMLoc S = getLoc();
    const MCExpr *Expr;
    if (parseSymbolicImmVal(Expr))
      return Error(S, "invalid operand");

    SMLoc E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
    Operands.push_back(ARM64Operand::CreateImm(Expr, S, E, getContext()));
    return false;
  }
  case AsmToken::LBrac: {
    SMLoc Loc = Parser.getTok().getLoc();
    Operands.push_back(ARM64Operand::CreateToken("[", false, Loc,
                                                 getContext()));
    Parser.Lex(); // Eat '['

    // There's no comma after a '[', so we can parse the next operand
    // immediately.
    return parseOperand(Operands, false, false);
  }
  case AsmToken::LCurly:
    return parseVectorList(Operands);
  case AsmToken::Identifier: {
    // If we're expecting a Condition Code operand, then just parse that.
    if (isCondCode)
      return parseCondCode(Operands, invertCondCode);

    // If it's a register name, parse it.
    if (!parseRegister(Operands))
      return false;

    // This could be an optional "shift" or "extend" operand.
    OperandMatchResultTy GotShift = tryParseOptionalShiftExtend(Operands);
    // We can only continue if no tokens were eaten.
    if (GotShift != MatchOperand_NoMatch)
      return GotShift;

    // This was not a register so parse other operands that start with an
    // identifier (like labels) as expressions and create them as immediates.
    const MCExpr *IdVal;
    S = getLoc();
    if (getParser().parseExpression(IdVal))
      return true;

    E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
    Operands.push_back(ARM64Operand::CreateImm(IdVal, S, E, getContext()));
    return false;
  }
  case AsmToken::Integer:
  case AsmToken::Real:
  case AsmToken::Hash: {
    // #42 -> immediate.
    S = getLoc();
    if (getLexer().is(AsmToken::Hash))
      Parser.Lex();

    // Parse a negative sign
    bool isNegative = false;
    if (Parser.getTok().is(AsmToken::Minus)) {
      isNegative = true;
      // We need to consume this token only when we have a Real, otherwise
      // we let parseSymbolicImmVal take care of it
      if (Parser.getLexer().peekTok().is(AsmToken::Real))
        Parser.Lex();
    }

    // The only Real that should come through here is a literal #0.0 for
    // the fcmp[e] r, #0.0 instructions. They expect raw token operands,
    // so convert the value.
    const AsmToken &Tok = Parser.getTok();
    if (Tok.is(AsmToken::Real)) {
      APFloat RealVal(APFloat::IEEEdouble, Tok.getString());
      uint64_t IntVal = RealVal.bitcastToAPInt().getZExtValue();
      if (Mnemonic != "fcmp" && Mnemonic != "fcmpe" && Mnemonic != "fcmeq" &&
          Mnemonic != "fcmge" && Mnemonic != "fcmgt" && Mnemonic != "fcmle" &&
          Mnemonic != "fcmlt")
        return TokError("unexpected floating point literal");
      else if (IntVal != 0 || isNegative)
        return TokError("expected floating-point constant #0.0");
      Parser.Lex(); // Eat the token.

      Operands.push_back(
          ARM64Operand::CreateToken("#0", false, S, getContext()));
      Operands.push_back(
          ARM64Operand::CreateToken(".0", false, S, getContext()));
      return false;
    }

    const MCExpr *ImmVal;
    if (parseSymbolicImmVal(ImmVal))
      return true;

    E = SMLoc::getFromPointer(getLoc().getPointer() - 1);
    Operands.push_back(ARM64Operand::CreateImm(ImmVal, S, E, getContext()));
    return false;
  }
  }
}

/// ParseInstruction - Parse an ARM64 instruction mnemonic followed by its
/// operands.
bool ARM64AsmParser::ParseInstruction(ParseInstructionInfo &Info,
                                      StringRef Name, SMLoc NameLoc,
                                      OperandVector &Operands) {
  Name = StringSwitch<StringRef>(Name.lower())
             .Case("beq", "b.eq")
             .Case("bne", "b.ne")
             .Case("bhs", "b.hs")
             .Case("bcs", "b.cs")
             .Case("blo", "b.lo")
             .Case("bcc", "b.cc")
             .Case("bmi", "b.mi")
             .Case("bpl", "b.pl")
             .Case("bvs", "b.vs")
             .Case("bvc", "b.vc")
             .Case("bhi", "b.hi")
             .Case("bls", "b.ls")
             .Case("bge", "b.ge")
             .Case("blt", "b.lt")
             .Case("bgt", "b.gt")
             .Case("ble", "b.le")
             .Case("bal", "b.al")
             .Case("bnv", "b.nv")
             .Default(Name);

  // Create the leading tokens for the mnemonic, split by '.' characters.
  size_t Start = 0, Next = Name.find('.');
  StringRef Head = Name.slice(Start, Next);

  // IC, DC, AT, and TLBI instructions are aliases for the SYS instruction.
  if (Head == "ic" || Head == "dc" || Head == "at" || Head == "tlbi") {
    bool IsError = parseSysAlias(Head, NameLoc, Operands);
    if (IsError && getLexer().isNot(AsmToken::EndOfStatement))
      Parser.eatToEndOfStatement();
    return IsError;
  }

  Operands.push_back(
      ARM64Operand::CreateToken(Head, false, NameLoc, getContext()));
  Mnemonic = Head;

  // Handle condition codes for a branch mnemonic
  if (Head == "b" && Next != StringRef::npos) {
    Start = Next;
    Next = Name.find('.', Start + 1);
    Head = Name.slice(Start + 1, Next);

    SMLoc SuffixLoc = SMLoc::getFromPointer(NameLoc.getPointer() +
                                            (Head.data() - Name.data()));
    ARM64CC::CondCode CC = parseCondCodeString(Head);
    if (CC == ARM64CC::Invalid)
      return Error(SuffixLoc, "invalid condition code");
    Operands.push_back(
        ARM64Operand::CreateToken(".", true, SuffixLoc, getContext()));
    Operands.push_back(
        ARM64Operand::CreateCondCode(CC, NameLoc, NameLoc, getContext()));
  }

  // Add the remaining tokens in the mnemonic.
  while (Next != StringRef::npos) {
    Start = Next;
    Next = Name.find('.', Start + 1);
    Head = Name.slice(Start, Next);
    SMLoc SuffixLoc = SMLoc::getFromPointer(NameLoc.getPointer() +
                                            (Head.data() - Name.data()) + 1);
    Operands.push_back(
        ARM64Operand::CreateToken(Head, true, SuffixLoc, getContext()));
  }

  // Conditional compare instructions have a Condition Code operand, which needs
  // to be parsed and an immediate operand created.
  bool condCodeFourthOperand =
      (Head == "ccmp" || Head == "ccmn" || Head == "fccmp" ||
       Head == "fccmpe" || Head == "fcsel" || Head == "csel" ||
       Head == "csinc" || Head == "csinv" || Head == "csneg");

  // These instructions are aliases to some of the conditional select
  // instructions. However, the condition code is inverted in the aliased
  // instruction.
  //
  // FIXME: Is this the correct way to handle these? Or should the parser
  //        generate the aliased instructions directly?
  bool condCodeSecondOperand = (Head == "cset" || Head == "csetm");
  bool condCodeThirdOperand =
      (Head == "cinc" || Head == "cinv" || Head == "cneg");

  // Read the remaining operands.
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    // Read the first operand.
    if (parseOperand(Operands, false, false)) {
      Parser.eatToEndOfStatement();
      return true;
    }

    unsigned N = 2;
    while (getLexer().is(AsmToken::Comma)) {
      Parser.Lex(); // Eat the comma.

      // Parse and remember the operand.
      if (parseOperand(Operands, (N == 4 && condCodeFourthOperand) ||
                                     (N == 3 && condCodeThirdOperand) ||
                                     (N == 2 && condCodeSecondOperand),
                       condCodeSecondOperand || condCodeThirdOperand)) {
        Parser.eatToEndOfStatement();
        return true;
      }

      // After successfully parsing some operands there are two special cases to
      // consider (i.e. notional operands not separated by commas). Both are due
      // to memory specifiers:
      //  + An RBrac will end an address for load/store/prefetch
      //  + An '!' will indicate a pre-indexed operation.
      //
      // It's someone else's responsibility to make sure these tokens are sane
      // in the given context!
      if (Parser.getTok().is(AsmToken::RBrac)) {
        SMLoc Loc = Parser.getTok().getLoc();
        Operands.push_back(ARM64Operand::CreateToken("]", false, Loc,
                                                     getContext()));
        Parser.Lex();
      }

      if (Parser.getTok().is(AsmToken::Exclaim)) {
        SMLoc Loc = Parser.getTok().getLoc();
        Operands.push_back(ARM64Operand::CreateToken("!", false, Loc,
                                                     getContext()));
        Parser.Lex();
      }

      ++N;
    }
  }

  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    SMLoc Loc = Parser.getTok().getLoc();
    Parser.eatToEndOfStatement();
    return Error(Loc, "unexpected token in argument list");
  }

  Parser.Lex(); // Consume the EndOfStatement
  return false;
}

// FIXME: This entire function is a giant hack to provide us with decent
// operand range validation/diagnostics until TableGen/MC can be extended
// to support autogeneration of this kind of validation.
bool ARM64AsmParser::validateInstruction(MCInst &Inst,
                                         SmallVectorImpl<SMLoc> &Loc) {
  const MCRegisterInfo *RI = getContext().getRegisterInfo();
  // Check for indexed addressing modes w/ the base register being the
  // same as a destination/source register or pair load where
  // the Rt == Rt2. All of those are undefined behaviour.
  switch (Inst.getOpcode()) {
  case ARM64::LDPSWpre:
  case ARM64::LDPWpost:
  case ARM64::LDPWpre:
  case ARM64::LDPXpost:
  case ARM64::LDPXpre: {
    unsigned Rt = Inst.getOperand(1).getReg();
    unsigned Rt2 = Inst.getOperand(2).getReg();
    unsigned Rn = Inst.getOperand(3).getReg();
    if (RI->isSubRegisterEq(Rn, Rt))
      return Error(Loc[0], "unpredictable LDP instruction, writeback base "
                           "is also a destination");
    if (RI->isSubRegisterEq(Rn, Rt2))
      return Error(Loc[1], "unpredictable LDP instruction, writeback base "
                           "is also a destination");
    // FALLTHROUGH
  }
  case ARM64::LDPDi:
  case ARM64::LDPQi:
  case ARM64::LDPSi:
  case ARM64::LDPSWi:
  case ARM64::LDPWi:
  case ARM64::LDPXi: {
    unsigned Rt = Inst.getOperand(0).getReg();
    unsigned Rt2 = Inst.getOperand(1).getReg();
    if (Rt == Rt2)
      return Error(Loc[1], "unpredictable LDP instruction, Rt2==Rt");
    break;
  }
  case ARM64::LDPDpost:
  case ARM64::LDPDpre:
  case ARM64::LDPQpost:
  case ARM64::LDPQpre:
  case ARM64::LDPSpost:
  case ARM64::LDPSpre:
  case ARM64::LDPSWpost: {
    unsigned Rt = Inst.getOperand(1).getReg();
    unsigned Rt2 = Inst.getOperand(2).getReg();
    if (Rt == Rt2)
      return Error(Loc[1], "unpredictable LDP instruction, Rt2==Rt");
    break;
  }
  case ARM64::STPDpost:
  case ARM64::STPDpre:
  case ARM64::STPQpost:
  case ARM64::STPQpre:
  case ARM64::STPSpost:
  case ARM64::STPSpre:
  case ARM64::STPWpost:
  case ARM64::STPWpre:
  case ARM64::STPXpost:
  case ARM64::STPXpre: {
    unsigned Rt = Inst.getOperand(1).getReg();
    unsigned Rt2 = Inst.getOperand(2).getReg();
    unsigned Rn = Inst.getOperand(3).getReg();
    if (RI->isSubRegisterEq(Rn, Rt))
      return Error(Loc[0], "unpredictable STP instruction, writeback base "
                           "is also a source");
    if (RI->isSubRegisterEq(Rn, Rt2))
      return Error(Loc[1], "unpredictable STP instruction, writeback base "
                           "is also a source");
    break;
  }
  case ARM64::LDRBBpre:
  case ARM64::LDRBpre:
  case ARM64::LDRHHpre:
  case ARM64::LDRHpre:
  case ARM64::LDRSBWpre:
  case ARM64::LDRSBXpre:
  case ARM64::LDRSHWpre:
  case ARM64::LDRSHXpre:
  case ARM64::LDRSWpre:
  case ARM64::LDRWpre:
  case ARM64::LDRXpre:
  case ARM64::LDRBBpost:
  case ARM64::LDRBpost:
  case ARM64::LDRHHpost:
  case ARM64::LDRHpost:
  case ARM64::LDRSBWpost:
  case ARM64::LDRSBXpost:
  case ARM64::LDRSHWpost:
  case ARM64::LDRSHXpost:
  case ARM64::LDRSWpost:
  case ARM64::LDRWpost:
  case ARM64::LDRXpost: {
    unsigned Rt = Inst.getOperand(1).getReg();
    unsigned Rn = Inst.getOperand(2).getReg();
    if (RI->isSubRegisterEq(Rn, Rt))
      return Error(Loc[0], "unpredictable LDR instruction, writeback base "
                           "is also a source");
    break;
  }
  case ARM64::STRBBpost:
  case ARM64::STRBpost:
  case ARM64::STRHHpost:
  case ARM64::STRHpost:
  case ARM64::STRWpost:
  case ARM64::STRXpost:
  case ARM64::STRBBpre:
  case ARM64::STRBpre:
  case ARM64::STRHHpre:
  case ARM64::STRHpre:
  case ARM64::STRWpre:
  case ARM64::STRXpre: {
    unsigned Rt = Inst.getOperand(1).getReg();
    unsigned Rn = Inst.getOperand(2).getReg();
    if (RI->isSubRegisterEq(Rn, Rt))
      return Error(Loc[0], "unpredictable STR instruction, writeback base "
                           "is also a source");
    break;
  }
  }

  // Now check immediate ranges. Separate from the above as there is overlap
  // in the instructions being checked and this keeps the nested conditionals
  // to a minimum.
  switch (Inst.getOpcode()) {
  case ARM64::ADDSWri:
  case ARM64::ADDSXri:
  case ARM64::ADDWri:
  case ARM64::ADDXri:
  case ARM64::SUBSWri:
  case ARM64::SUBSXri:
  case ARM64::SUBWri:
  case ARM64::SUBXri: {
    // Annoyingly we can't do this in the isAddSubImm predicate, so there is
    // some slight duplication here.
    if (Inst.getOperand(2).isExpr()) {
      const MCExpr *Expr = Inst.getOperand(2).getExpr();
      ARM64MCExpr::VariantKind ELFRefKind;
      MCSymbolRefExpr::VariantKind DarwinRefKind;
      int64_t Addend;
      if (!classifySymbolRef(Expr, ELFRefKind, DarwinRefKind, Addend)) {
        return Error(Loc[2], "invalid immediate expression");
      }

      // Only allow these with ADDXri.
      if ((DarwinRefKind == MCSymbolRefExpr::VK_PAGEOFF ||
          DarwinRefKind == MCSymbolRefExpr::VK_TLVPPAGEOFF) &&
          Inst.getOpcode() == ARM64::ADDXri)
        return false;

      // Only allow these with ADDXri/ADDWri
      if ((ELFRefKind == ARM64MCExpr::VK_LO12 ||
          ELFRefKind == ARM64MCExpr::VK_DTPREL_HI12 ||
          ELFRefKind == ARM64MCExpr::VK_DTPREL_LO12 ||
          ELFRefKind == ARM64MCExpr::VK_DTPREL_LO12_NC ||
          ELFRefKind == ARM64MCExpr::VK_TPREL_HI12 ||
          ELFRefKind == ARM64MCExpr::VK_TPREL_LO12 ||
          ELFRefKind == ARM64MCExpr::VK_TPREL_LO12_NC ||
          ELFRefKind == ARM64MCExpr::VK_TLSDESC_LO12) &&
          (Inst.getOpcode() == ARM64::ADDXri ||
          Inst.getOpcode() == ARM64::ADDWri))
        return false;

      // Don't allow expressions in the immediate field otherwise
      return Error(Loc[2], "invalid immediate expression");
    }
    return false;
  }
  default:
    return false;
  }
}

bool ARM64AsmParser::showMatchError(SMLoc Loc, unsigned ErrCode) {
  switch (ErrCode) {
  case Match_MissingFeature:
    return Error(Loc,
                 "instruction requires a CPU feature not currently enabled");
  case Match_InvalidOperand:
    return Error(Loc, "invalid operand for instruction");
  case Match_InvalidSuffix:
    return Error(Loc, "invalid type suffix for instruction");
  case Match_InvalidCondCode:
    return Error(Loc, "expected AArch64 condition code");
  case Match_AddSubRegExtendSmall:
    return Error(Loc,
      "expected '[su]xt[bhw]' or 'lsl' with optional integer in range [0, 4]");
  case Match_AddSubRegExtendLarge:
    return Error(Loc,
      "expected 'sxtx' 'uxtx' or 'lsl' with optional integer in range [0, 4]");
  case Match_AddSubSecondSource:
    return Error(Loc,
      "expected compatible register, symbol or integer in range [0, 4095]");
  case Match_LogicalSecondSource:
    return Error(Loc, "expected compatible register or logical immediate");
  case Match_InvalidMovImm32Shift:
    return Error(Loc, "expected 'lsl' with optional integer 0 or 16");
  case Match_InvalidMovImm64Shift:
    return Error(Loc, "expected 'lsl' with optional integer 0, 16, 32 or 48");
  case Match_AddSubRegShift32:
    return Error(Loc,
       "expected 'lsl', 'lsr' or 'asr' with optional integer in range [0, 31]");
  case Match_AddSubRegShift64:
    return Error(Loc,
       "expected 'lsl', 'lsr' or 'asr' with optional integer in range [0, 63]");
  case Match_InvalidFPImm:
    return Error(Loc,
                 "expected compatible register or floating-point constant");
  case Match_InvalidMemoryIndexedSImm9:
    return Error(Loc, "index must be an integer in range [-256, 255].");
  case Match_InvalidMemoryIndexed4SImm7:
    return Error(Loc, "index must be a multiple of 4 in range [-256, 252].");
  case Match_InvalidMemoryIndexed8SImm7:
    return Error(Loc, "index must be a multiple of 8 in range [-512, 504].");
  case Match_InvalidMemoryIndexed16SImm7:
    return Error(Loc, "index must be a multiple of 16 in range [-1024, 1008].");
  case Match_InvalidMemoryWExtend8:
    return Error(Loc,
                 "expected 'uxtw' or 'sxtw' with optional shift of #0");
  case Match_InvalidMemoryWExtend16:
    return Error(Loc,
                 "expected 'uxtw' or 'sxtw' with optional shift of #0 or #1");
  case Match_InvalidMemoryWExtend32:
    return Error(Loc,
                 "expected 'uxtw' or 'sxtw' with optional shift of #0 or #2");
  case Match_InvalidMemoryWExtend64:
    return Error(Loc,
                 "expected 'uxtw' or 'sxtw' with optional shift of #0 or #3");
  case Match_InvalidMemoryWExtend128:
    return Error(Loc,
                 "expected 'uxtw' or 'sxtw' with optional shift of #0 or #4");
  case Match_InvalidMemoryXExtend8:
    return Error(Loc,
                 "expected 'lsl' or 'sxtx' with optional shift of #0");
  case Match_InvalidMemoryXExtend16:
    return Error(Loc,
                 "expected 'lsl' or 'sxtx' with optional shift of #0 or #1");
  case Match_InvalidMemoryXExtend32:
    return Error(Loc,
                 "expected 'lsl' or 'sxtx' with optional shift of #0 or #2");
  case Match_InvalidMemoryXExtend64:
    return Error(Loc,
                 "expected 'lsl' or 'sxtx' with optional shift of #0 or #3");
  case Match_InvalidMemoryXExtend128:
    return Error(Loc,
                 "expected 'lsl' or 'sxtx' with optional shift of #0 or #4");
  case Match_InvalidMemoryIndexed1:
    return Error(Loc, "index must be an integer in range [0, 4095].");
  case Match_InvalidMemoryIndexed2:
    return Error(Loc, "index must be a multiple of 2 in range [0, 8190].");
  case Match_InvalidMemoryIndexed4:
    return Error(Loc, "index must be a multiple of 4 in range [0, 16380].");
  case Match_InvalidMemoryIndexed8:
    return Error(Loc, "index must be a multiple of 8 in range [0, 32760].");
  case Match_InvalidMemoryIndexed16:
    return Error(Loc, "index must be a multiple of 16 in range [0, 65520].");
  case Match_InvalidImm0_7:
    return Error(Loc, "immediate must be an integer in range [0, 7].");
  case Match_InvalidImm0_15:
    return Error(Loc, "immediate must be an integer in range [0, 15].");
  case Match_InvalidImm0_31:
    return Error(Loc, "immediate must be an integer in range [0, 31].");
  case Match_InvalidImm0_63:
    return Error(Loc, "immediate must be an integer in range [0, 63].");
  case Match_InvalidImm0_127:
    return Error(Loc, "immediate must be an integer in range [0, 127].");
  case Match_InvalidImm0_65535:
    return Error(Loc, "immediate must be an integer in range [0, 65535].");
  case Match_InvalidImm1_8:
    return Error(Loc, "immediate must be an integer in range [1, 8].");
  case Match_InvalidImm1_16:
    return Error(Loc, "immediate must be an integer in range [1, 16].");
  case Match_InvalidImm1_32:
    return Error(Loc, "immediate must be an integer in range [1, 32].");
  case Match_InvalidImm1_64:
    return Error(Loc, "immediate must be an integer in range [1, 64].");
  case Match_InvalidIndex1:
    return Error(Loc, "expected lane specifier '[1]'");
  case Match_InvalidIndexB:
    return Error(Loc, "vector lane must be an integer in range [0, 15].");
  case Match_InvalidIndexH:
    return Error(Loc, "vector lane must be an integer in range [0, 7].");
  case Match_InvalidIndexS:
    return Error(Loc, "vector lane must be an integer in range [0, 3].");
  case Match_InvalidIndexD:
    return Error(Loc, "vector lane must be an integer in range [0, 1].");
  case Match_InvalidLabel:
    return Error(Loc, "expected label or encodable integer pc offset");
  case Match_MRS:
    return Error(Loc, "expected readable system register");
  case Match_MSR:
    return Error(Loc, "expected writable system register or pstate");
  case Match_MnemonicFail:
    return Error(Loc, "unrecognized instruction mnemonic");
  default:
    assert(0 && "unexpected error code!");
    return Error(Loc, "invalid instruction format");
  }
}

static const char *getSubtargetFeatureName(unsigned Val);

bool ARM64AsmParser::MatchAndEmitInstruction(SMLoc IDLoc, unsigned &Opcode,
                                             OperandVector &Operands,
                                             MCStreamer &Out,
                                             unsigned &ErrorInfo,
                                             bool MatchingInlineAsm) {
  assert(!Operands.empty() && "Unexpect empty operand list!");
  ARM64Operand *Op = static_cast<ARM64Operand *>(Operands[0]);
  assert(Op->isToken() && "Leading operand should always be a mnemonic!");

  StringRef Tok = Op->getToken();
  unsigned NumOperands = Operands.size();

  if (NumOperands == 4 && Tok == "lsl") {
    ARM64Operand *Op2 = static_cast<ARM64Operand *>(Operands[2]);
    ARM64Operand *Op3 = static_cast<ARM64Operand *>(Operands[3]);
    if (Op2->isReg() && Op3->isImm()) {
      const MCConstantExpr *Op3CE = dyn_cast<MCConstantExpr>(Op3->getImm());
      if (Op3CE) {
        uint64_t Op3Val = Op3CE->getValue();
        uint64_t NewOp3Val = 0;
        uint64_t NewOp4Val = 0;
        if (ARM64MCRegisterClasses[ARM64::GPR32allRegClassID].contains(
                Op2->getReg())) {
          NewOp3Val = (32 - Op3Val) & 0x1f;
          NewOp4Val = 31 - Op3Val;
        } else {
          NewOp3Val = (64 - Op3Val) & 0x3f;
          NewOp4Val = 63 - Op3Val;
        }

        const MCExpr *NewOp3 = MCConstantExpr::Create(NewOp3Val, getContext());
        const MCExpr *NewOp4 = MCConstantExpr::Create(NewOp4Val, getContext());

        Operands[0] = ARM64Operand::CreateToken(
            "ubfm", false, Op->getStartLoc(), getContext());
        Operands[3] = ARM64Operand::CreateImm(NewOp3, Op3->getStartLoc(),
                                              Op3->getEndLoc(), getContext());
        Operands.push_back(ARM64Operand::CreateImm(
            NewOp4, Op3->getStartLoc(), Op3->getEndLoc(), getContext()));
        delete Op3;
        delete Op;
      }
    }
  } else if (NumOperands == 5) {
    // FIXME: Horrible hack to handle the BFI -> BFM, SBFIZ->SBFM, and
    // UBFIZ -> UBFM aliases.
    if (Tok == "bfi" || Tok == "sbfiz" || Tok == "ubfiz") {
      ARM64Operand *Op1 = static_cast<ARM64Operand *>(Operands[1]);
      ARM64Operand *Op3 = static_cast<ARM64Operand *>(Operands[3]);
      ARM64Operand *Op4 = static_cast<ARM64Operand *>(Operands[4]);

      if (Op1->isReg() && Op3->isImm() && Op4->isImm()) {
        const MCConstantExpr *Op3CE = dyn_cast<MCConstantExpr>(Op3->getImm());
        const MCConstantExpr *Op4CE = dyn_cast<MCConstantExpr>(Op4->getImm());

        if (Op3CE && Op4CE) {
          uint64_t Op3Val = Op3CE->getValue();
          uint64_t Op4Val = Op4CE->getValue();

          uint64_t RegWidth = 0;
          if (ARM64MCRegisterClasses[ARM64::GPR64allRegClassID].contains(
              Op1->getReg()))
            RegWidth = 64;
          else
            RegWidth = 32;

          if (Op3Val >= RegWidth)
            return Error(Op3->getStartLoc(),
                         "expected integer in range [0, 31]");
          if (Op4Val < 1 || Op4Val > RegWidth)
            return Error(Op4->getStartLoc(),
                         "expected integer in range [1, 32]");

          uint64_t NewOp3Val = 0;
          if (ARM64MCRegisterClasses[ARM64::GPR32allRegClassID].contains(
                  Op1->getReg()))
            NewOp3Val = (32 - Op3Val) & 0x1f;
          else
            NewOp3Val = (64 - Op3Val) & 0x3f;

          uint64_t NewOp4Val = Op4Val - 1;

          if (NewOp3Val != 0 && NewOp4Val >= NewOp3Val)
            return Error(Op4->getStartLoc(),
                         "requested insert overflows register");

          const MCExpr *NewOp3 =
              MCConstantExpr::Create(NewOp3Val, getContext());
          const MCExpr *NewOp4 =
              MCConstantExpr::Create(NewOp4Val, getContext());
          Operands[3] = ARM64Operand::CreateImm(NewOp3, Op3->getStartLoc(),
                                                Op3->getEndLoc(), getContext());
          Operands[4] = ARM64Operand::CreateImm(NewOp4, Op4->getStartLoc(),
                                                Op4->getEndLoc(), getContext());
          if (Tok == "bfi")
            Operands[0] = ARM64Operand::CreateToken(
                "bfm", false, Op->getStartLoc(), getContext());
          else if (Tok == "sbfiz")
            Operands[0] = ARM64Operand::CreateToken(
                "sbfm", false, Op->getStartLoc(), getContext());
          else if (Tok == "ubfiz")
            Operands[0] = ARM64Operand::CreateToken(
                "ubfm", false, Op->getStartLoc(), getContext());
          else
            llvm_unreachable("No valid mnemonic for alias?");

          delete Op;
          delete Op3;
          delete Op4;
        }
      }

      // FIXME: Horrible hack to handle the BFXIL->BFM, SBFX->SBFM, and
      // UBFX -> UBFM aliases.
    } else if (NumOperands == 5 &&
               (Tok == "bfxil" || Tok == "sbfx" || Tok == "ubfx")) {
      ARM64Operand *Op1 = static_cast<ARM64Operand *>(Operands[1]);
      ARM64Operand *Op3 = static_cast<ARM64Operand *>(Operands[3]);
      ARM64Operand *Op4 = static_cast<ARM64Operand *>(Operands[4]);

      if (Op1->isReg() && Op3->isImm() && Op4->isImm()) {
        const MCConstantExpr *Op3CE = dyn_cast<MCConstantExpr>(Op3->getImm());
        const MCConstantExpr *Op4CE = dyn_cast<MCConstantExpr>(Op4->getImm());

        if (Op3CE && Op4CE) {
          uint64_t Op3Val = Op3CE->getValue();
          uint64_t Op4Val = Op4CE->getValue();

          uint64_t RegWidth = 0;
          if (ARM64MCRegisterClasses[ARM64::GPR64allRegClassID].contains(
              Op1->getReg()))
            RegWidth = 64;
          else
            RegWidth = 32;

          if (Op3Val >= RegWidth)
            return Error(Op3->getStartLoc(),
                         "expected integer in range [0, 31]");
          if (Op4Val < 1 || Op4Val > RegWidth)
            return Error(Op4->getStartLoc(),
                         "expected integer in range [1, 32]");

          uint64_t NewOp4Val = Op3Val + Op4Val - 1;

          if (NewOp4Val >= RegWidth || NewOp4Val < Op3Val)
            return Error(Op4->getStartLoc(),
                         "requested extract overflows register");

          const MCExpr *NewOp4 =
              MCConstantExpr::Create(NewOp4Val, getContext());
          Operands[4] = ARM64Operand::CreateImm(
              NewOp4, Op4->getStartLoc(), Op4->getEndLoc(), getContext());
          if (Tok == "bfxil")
            Operands[0] = ARM64Operand::CreateToken(
                "bfm", false, Op->getStartLoc(), getContext());
          else if (Tok == "sbfx")
            Operands[0] = ARM64Operand::CreateToken(
                "sbfm", false, Op->getStartLoc(), getContext());
          else if (Tok == "ubfx")
            Operands[0] = ARM64Operand::CreateToken(
                "ubfm", false, Op->getStartLoc(), getContext());
          else
            llvm_unreachable("No valid mnemonic for alias?");

          delete Op;
          delete Op4;
        }
      }
    }
  }
  // FIXME: Horrible hack for sxtw and uxtw with Wn src and Xd dst operands.
  //        InstAlias can't quite handle this since the reg classes aren't
  //        subclasses.
  if (NumOperands == 3 && (Tok == "sxtw" || Tok == "uxtw")) {
    // The source register can be Wn here, but the matcher expects a
    // GPR64. Twiddle it here if necessary.
    ARM64Operand *Op = static_cast<ARM64Operand *>(Operands[2]);
    if (Op->isReg()) {
      unsigned Reg = getXRegFromWReg(Op->getReg());
      Operands[2] = ARM64Operand::CreateReg(Reg, false, Op->getStartLoc(),
                                            Op->getEndLoc(), getContext());
      delete Op;
    }
  }
  // FIXME: Likewise for sxt[bh] with a Xd dst operand
  else if (NumOperands == 3 && (Tok == "sxtb" || Tok == "sxth")) {
    ARM64Operand *Op = static_cast<ARM64Operand *>(Operands[1]);
    if (Op->isReg() &&
        ARM64MCRegisterClasses[ARM64::GPR64allRegClassID].contains(
            Op->getReg())) {
      // The source register can be Wn here, but the matcher expects a
      // GPR64. Twiddle it here if necessary.
      ARM64Operand *Op = static_cast<ARM64Operand *>(Operands[2]);
      if (Op->isReg()) {
        unsigned Reg = getXRegFromWReg(Op->getReg());
        Operands[2] = ARM64Operand::CreateReg(Reg, false, Op->getStartLoc(),
                                              Op->getEndLoc(), getContext());
        delete Op;
      }
    }
  }
  // FIXME: Likewise for uxt[bh] with a Xd dst operand
  else if (NumOperands == 3 && (Tok == "uxtb" || Tok == "uxth")) {
    ARM64Operand *Op = static_cast<ARM64Operand *>(Operands[1]);
    if (Op->isReg() &&
        ARM64MCRegisterClasses[ARM64::GPR64allRegClassID].contains(
            Op->getReg())) {
      // The source register can be Wn here, but the matcher expects a
      // GPR32. Twiddle it here if necessary.
      ARM64Operand *Op = static_cast<ARM64Operand *>(Operands[1]);
      if (Op->isReg()) {
        unsigned Reg = getWRegFromXReg(Op->getReg());
        Operands[1] = ARM64Operand::CreateReg(Reg, false, Op->getStartLoc(),
                                              Op->getEndLoc(), getContext());
        delete Op;
      }
    }
  }

  // Yet another horrible hack to handle FMOV Rd, #0.0 using [WX]ZR.
  if (NumOperands == 3 && Tok == "fmov") {
    ARM64Operand *RegOp = static_cast<ARM64Operand *>(Operands[1]);
    ARM64Operand *ImmOp = static_cast<ARM64Operand *>(Operands[2]);
    if (RegOp->isReg() && ImmOp->isFPImm() &&
        ImmOp->getFPImm() == (unsigned)-1) {
      unsigned zreg = ARM64MCRegisterClasses[ARM64::FPR32RegClassID].contains(
                          RegOp->getReg())
                          ? ARM64::WZR
                          : ARM64::XZR;
      Operands[2] = ARM64Operand::CreateReg(zreg, false, Op->getStartLoc(),
                                            Op->getEndLoc(), getContext());
      delete ImmOp;
    }
  }

  MCInst Inst;
  // First try to match against the secondary set of tables containing the
  // short-form NEON instructions (e.g. "fadd.2s v0, v1, v2").
  unsigned MatchResult =
      MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm, 1);

  // If that fails, try against the alternate table containing long-form NEON:
  // "fadd v0.2s, v1.2s, v2.2s"
  if (MatchResult != Match_Success)
    MatchResult =
        MatchInstructionImpl(Operands, Inst, ErrorInfo, MatchingInlineAsm, 0);

  switch (MatchResult) {
  case Match_Success: {
    // Perform range checking and other semantic validations
    SmallVector<SMLoc, 8> OperandLocs;
    NumOperands = Operands.size();
    for (unsigned i = 1; i < NumOperands; ++i)
      OperandLocs.push_back(Operands[i]->getStartLoc());
    if (validateInstruction(Inst, OperandLocs))
      return true;

    Inst.setLoc(IDLoc);
    Out.EmitInstruction(Inst, STI);
    return false;
  }
  case Match_MissingFeature: {
    assert(ErrorInfo && "Unknown missing feature!");
    // Special case the error message for the very common case where only
    // a single subtarget feature is missing (neon, e.g.).
    std::string Msg = "instruction requires:";
    unsigned Mask = 1;
    for (unsigned i = 0; i < (sizeof(ErrorInfo)*8-1); ++i) {
      if (ErrorInfo & Mask) {
        Msg += " ";
        Msg += getSubtargetFeatureName(ErrorInfo & Mask);
      }
      Mask <<= 1;
    }
    return Error(IDLoc, Msg);
  }
  case Match_MnemonicFail:
    return showMatchError(IDLoc, MatchResult);
  case Match_InvalidOperand: {
    SMLoc ErrorLoc = IDLoc;
    if (ErrorInfo != ~0U) {
      if (ErrorInfo >= Operands.size())
        return Error(IDLoc, "too few operands for instruction");

      ErrorLoc = ((ARM64Operand *)Operands[ErrorInfo])->getStartLoc();
      if (ErrorLoc == SMLoc())
        ErrorLoc = IDLoc;
    }
    // If the match failed on a suffix token operand, tweak the diagnostic
    // accordingly.
    if (((ARM64Operand *)Operands[ErrorInfo])->isToken() &&
        ((ARM64Operand *)Operands[ErrorInfo])->isTokenSuffix())
      MatchResult = Match_InvalidSuffix;

    return showMatchError(ErrorLoc, MatchResult);
  }
  case Match_InvalidMemoryIndexed1:
  case Match_InvalidMemoryIndexed2:
  case Match_InvalidMemoryIndexed4:
  case Match_InvalidMemoryIndexed8:
  case Match_InvalidMemoryIndexed16:
  case Match_InvalidCondCode:
  case Match_AddSubRegExtendSmall:
  case Match_AddSubRegExtendLarge:
  case Match_AddSubSecondSource:
  case Match_LogicalSecondSource:
  case Match_AddSubRegShift32:
  case Match_AddSubRegShift64:
  case Match_InvalidMovImm32Shift:
  case Match_InvalidMovImm64Shift:
  case Match_InvalidFPImm:
  case Match_InvalidMemoryWExtend8:
  case Match_InvalidMemoryWExtend16:
  case Match_InvalidMemoryWExtend32:
  case Match_InvalidMemoryWExtend64:
  case Match_InvalidMemoryWExtend128:
  case Match_InvalidMemoryXExtend8:
  case Match_InvalidMemoryXExtend16:
  case Match_InvalidMemoryXExtend32:
  case Match_InvalidMemoryXExtend64:
  case Match_InvalidMemoryXExtend128:
  case Match_InvalidMemoryIndexed4SImm7:
  case Match_InvalidMemoryIndexed8SImm7:
  case Match_InvalidMemoryIndexed16SImm7:
  case Match_InvalidMemoryIndexedSImm9:
  case Match_InvalidImm0_7:
  case Match_InvalidImm0_15:
  case Match_InvalidImm0_31:
  case Match_InvalidImm0_63:
  case Match_InvalidImm0_127:
  case Match_InvalidImm0_65535:
  case Match_InvalidImm1_8:
  case Match_InvalidImm1_16:
  case Match_InvalidImm1_32:
  case Match_InvalidImm1_64:
  case Match_InvalidIndex1:
  case Match_InvalidIndexB:
  case Match_InvalidIndexH:
  case Match_InvalidIndexS:
  case Match_InvalidIndexD:
  case Match_InvalidLabel:
  case Match_MSR:
  case Match_MRS: {
    // Any time we get here, there's nothing fancy to do. Just get the
    // operand SMLoc and display the diagnostic.
    SMLoc ErrorLoc = ((ARM64Operand *)Operands[ErrorInfo])->getStartLoc();
    if (ErrorLoc == SMLoc())
      ErrorLoc = IDLoc;
    return showMatchError(ErrorLoc, MatchResult);
  }
  }

  llvm_unreachable("Implement any new match types added!");
  return true;
}

/// ParseDirective parses the arm specific directives
bool ARM64AsmParser::ParseDirective(AsmToken DirectiveID) {
  StringRef IDVal = DirectiveID.getIdentifier();
  SMLoc Loc = DirectiveID.getLoc();
  if (IDVal == ".hword")
    return parseDirectiveWord(2, Loc);
  if (IDVal == ".word")
    return parseDirectiveWord(4, Loc);
  if (IDVal == ".xword")
    return parseDirectiveWord(8, Loc);
  if (IDVal == ".tlsdesccall")
    return parseDirectiveTLSDescCall(Loc);

  return parseDirectiveLOH(IDVal, Loc);
}

/// parseDirectiveWord
///  ::= .word [ expression (, expression)* ]
bool ARM64AsmParser::parseDirectiveWord(unsigned Size, SMLoc L) {
  if (getLexer().isNot(AsmToken::EndOfStatement)) {
    for (;;) {
      const MCExpr *Value;
      if (getParser().parseExpression(Value))
        return true;

      getParser().getStreamer().EmitValue(Value, Size);

      if (getLexer().is(AsmToken::EndOfStatement))
        break;

      // FIXME: Improve diagnostic.
      if (getLexer().isNot(AsmToken::Comma))
        return Error(L, "unexpected token in directive");
      Parser.Lex();
    }
  }

  Parser.Lex();
  return false;
}

// parseDirectiveTLSDescCall:
//   ::= .tlsdesccall symbol
bool ARM64AsmParser::parseDirectiveTLSDescCall(SMLoc L) {
  StringRef Name;
  if (getParser().parseIdentifier(Name))
    return Error(L, "expected symbol after directive");

  MCSymbol *Sym = getContext().GetOrCreateSymbol(Name);
  const MCExpr *Expr = MCSymbolRefExpr::Create(Sym, getContext());
  Expr = ARM64MCExpr::Create(Expr, ARM64MCExpr::VK_TLSDESC, getContext());

  MCInst Inst;
  Inst.setOpcode(ARM64::TLSDESCCALL);
  Inst.addOperand(MCOperand::CreateExpr(Expr));

  getParser().getStreamer().EmitInstruction(Inst, STI);
  return false;
}

/// ::= .loh <lohName | lohId> label1, ..., labelN
/// The number of arguments depends on the loh identifier.
bool ARM64AsmParser::parseDirectiveLOH(StringRef IDVal, SMLoc Loc) {
  if (IDVal != MCLOHDirectiveName())
    return true;
  MCLOHType Kind;
  if (getParser().getTok().isNot(AsmToken::Identifier)) {
    if (getParser().getTok().isNot(AsmToken::Integer))
      return TokError("expected an identifier or a number in directive");
    // We successfully get a numeric value for the identifier.
    // Check if it is valid.
    int64_t Id = getParser().getTok().getIntVal();
    Kind = (MCLOHType)Id;
    // Check that Id does not overflow MCLOHType.
    if (!isValidMCLOHType(Kind) || Id != Kind)
      return TokError("invalid numeric identifier in directive");
  } else {
    StringRef Name = getTok().getIdentifier();
    // We successfully parse an identifier.
    // Check if it is a recognized one.
    int Id = MCLOHNameToId(Name);

    if (Id == -1)
      return TokError("invalid identifier in directive");
    Kind = (MCLOHType)Id;
  }
  // Consume the identifier.
  Lex();
  // Get the number of arguments of this LOH.
  int NbArgs = MCLOHIdToNbArgs(Kind);

  assert(NbArgs != -1 && "Invalid number of arguments");

  SmallVector<MCSymbol *, 3> Args;
  for (int Idx = 0; Idx < NbArgs; ++Idx) {
    StringRef Name;
    if (getParser().parseIdentifier(Name))
      return TokError("expected identifier in directive");
    Args.push_back(getContext().GetOrCreateSymbol(Name));

    if (Idx + 1 == NbArgs)
      break;
    if (getLexer().isNot(AsmToken::Comma))
      return TokError("unexpected token in '" + Twine(IDVal) + "' directive");
    Lex();
  }
  if (getLexer().isNot(AsmToken::EndOfStatement))
    return TokError("unexpected token in '" + Twine(IDVal) + "' directive");

  getStreamer().EmitLOHDirective((MCLOHType)Kind, Args);
  return false;
}

bool
ARM64AsmParser::classifySymbolRef(const MCExpr *Expr,
                                  ARM64MCExpr::VariantKind &ELFRefKind,
                                  MCSymbolRefExpr::VariantKind &DarwinRefKind,
                                  int64_t &Addend) {
  ELFRefKind = ARM64MCExpr::VK_INVALID;
  DarwinRefKind = MCSymbolRefExpr::VK_None;
  Addend = 0;

  if (const ARM64MCExpr *AE = dyn_cast<ARM64MCExpr>(Expr)) {
    ELFRefKind = AE->getKind();
    Expr = AE->getSubExpr();
  }

  const MCSymbolRefExpr *SE = dyn_cast<MCSymbolRefExpr>(Expr);
  if (SE) {
    // It's a simple symbol reference with no addend.
    DarwinRefKind = SE->getKind();
    return true;
  }

  const MCBinaryExpr *BE = dyn_cast<MCBinaryExpr>(Expr);
  if (!BE)
    return false;

  SE = dyn_cast<MCSymbolRefExpr>(BE->getLHS());
  if (!SE)
    return false;
  DarwinRefKind = SE->getKind();

  if (BE->getOpcode() != MCBinaryExpr::Add &&
      BE->getOpcode() != MCBinaryExpr::Sub)
    return false;

  // See if the addend is is a constant, otherwise there's more going
  // on here than we can deal with.
  auto AddendExpr = dyn_cast<MCConstantExpr>(BE->getRHS());
  if (!AddendExpr)
    return false;

  Addend = AddendExpr->getValue();
  if (BE->getOpcode() == MCBinaryExpr::Sub)
    Addend = -Addend;

  // It's some symbol reference + a constant addend, but really
  // shouldn't use both Darwin and ELF syntax.
  return ELFRefKind == ARM64MCExpr::VK_INVALID ||
         DarwinRefKind == MCSymbolRefExpr::VK_None;
}

/// Force static initialization.
extern "C" void LLVMInitializeARM64AsmParser() {
  RegisterMCAsmParser<ARM64AsmParser> X(TheARM64leTarget);
  RegisterMCAsmParser<ARM64AsmParser> Y(TheARM64beTarget);
}

#define GET_REGISTER_MATCHER
#define GET_SUBTARGET_FEATURE_NAME
#define GET_MATCHER_IMPLEMENTATION
#include "ARM64GenAsmMatcher.inc"

// Define this matcher function after the auto-generated include so we
// have the match class enum definitions.
unsigned ARM64AsmParser::validateTargetOperandClass(MCParsedAsmOperand *AsmOp,
                                                    unsigned Kind) {
  ARM64Operand *Op = static_cast<ARM64Operand *>(AsmOp);
  // If the kind is a token for a literal immediate, check if our asm
  // operand matches. This is for InstAliases which have a fixed-value
  // immediate in the syntax.
  int64_t ExpectedVal;
  switch (Kind) {
  default:
    return Match_InvalidOperand;
  case MCK__35_0:
    ExpectedVal = 0;
    break;
  case MCK__35_1:
    ExpectedVal = 1;
    break;
  case MCK__35_12:
    ExpectedVal = 12;
    break;
  case MCK__35_16:
    ExpectedVal = 16;
    break;
  case MCK__35_2:
    ExpectedVal = 2;
    break;
  case MCK__35_24:
    ExpectedVal = 24;
    break;
  case MCK__35_3:
    ExpectedVal = 3;
    break;
  case MCK__35_32:
    ExpectedVal = 32;
    break;
  case MCK__35_4:
    ExpectedVal = 4;
    break;
  case MCK__35_48:
    ExpectedVal = 48;
    break;
  case MCK__35_6:
    ExpectedVal = 6;
    break;
  case MCK__35_64:
    ExpectedVal = 64;
    break;
  case MCK__35_8:
    ExpectedVal = 8;
    break;
  }
  if (!Op->isImm())
    return Match_InvalidOperand;
  const MCConstantExpr *CE = dyn_cast<MCConstantExpr>(Op->getImm());
  if (!CE)
    return Match_InvalidOperand;
  if (CE->getValue() == ExpectedVal)
    return Match_Success;
  return Match_InvalidOperand;
}
