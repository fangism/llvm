//===-- PPCMCInstLower.cpp - Convert PPC MachineInstr to an MCInst --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains code to lower PPC MachineInstrs to their corresponding
// MCInst records.
//
//===----------------------------------------------------------------------===//

#include "PPC.h"
#include "MCTargetDesc/PPCMCExpr.h"
#include "llvm/ADT/SmallString.h"
#include "llvm/ADT/Twine.h"
#include "llvm/CodeGen/AsmPrinter.h"
#include "llvm/CodeGen/MachineFunction.h"
#include "llvm/CodeGen/MachineModuleInfoImpls.h"
#include "llvm/IR/GlobalValue.h"
#include "llvm/MC/MCAsmInfo.h"
#include "llvm/MC/MCExpr.h"
#include "llvm/MC/MCInst.h"
#include "llvm/Target/Mangler.h"

#define	ENABLE_STACKTRACE		0
#include "llvm/Support/stacktrace.h"
#include "llvm/Support/raw_ostream.h"
using namespace llvm;

static MachineModuleInfoMachO &getMachOMMI(AsmPrinter &AP) {
  return AP.MMI->getObjFileInfo<MachineModuleInfoMachO>();
}


static MCSymbol *GetSymbolFromOperand(const MachineOperand &MO, AsmPrinter &AP){
  STACKTRACE_VERBOSE;
  MCContext &Ctx = AP.OutContext;

  SmallString<128> Name;
  if (!MO.isGlobal()) {
    STACKTRACE_INDENT_PRINT("!MO.isGlobal()" << endl);
    assert(MO.isSymbol() && "Isn't a symbol reference");
    Name += AP.MAI->getGlobalPrefix();
    Name += MO.getSymbolName();
  } else {    
    STACKTRACE_INDENT_PRINT("MO.isGlobal()" << endl);
    const GlobalValue *GV = MO.getGlobal();
    bool isImplicitlyPrivate = false;
    if (MO.getTargetFlags() == PPCII::MO_DARWIN_STUB ||
        (MO.getTargetFlags() & PPCII::MO_NLP_FLAG))
      isImplicitlyPrivate = true;
    
    AP.Mang->getNameWithPrefix(Name, GV, isImplicitlyPrivate);
  }
  STACKTRACE_INDENT_PRINT("base Name: " << Name << endl);
  
  // If the target flags on the operand changes the name of the symbol, do that
  // before we return the symbol.
  if (MO.getTargetFlags() == PPCII::MO_DARWIN_STUB) {
    STACKTRACE_INDENT_PRINT("is PPCII::MO_DARWIN_STUB" << endl);
    Name += "$stub";
    const char *PGP = AP.MAI->getPrivateGlobalPrefix();
    const char *Prefix = "";
    if (!Name.startswith(PGP)) {
      // http://llvm.org/bugs/show_bug.cgi?id=15763
      // all stubs and lazy_ptrs should be local symbols, which need leading 'L'
      Prefix = PGP;
    }
    MCSymbol *Sym = Ctx.GetOrCreateSymbol(Twine(Prefix) + Twine(Name));
    MachineModuleInfoImpl::StubValueTy &StubSym =
      getMachOMMI(AP).getFnStubEntry(Sym);
    if (StubSym.getPointer())
      return Sym;
    
    if (MO.isGlobal()) {
      StubSym =
      MachineModuleInfoImpl::
      StubValueTy(AP.Mang->getSymbol(MO.getGlobal()),
                  !MO.getGlobal()->hasInternalLinkage());
    } else {
      Name.erase(Name.end()-5, Name.end());
      STACKTRACE_INDENT_PRINT("local stub Name: " << Name << endl);
      StubSym =
      MachineModuleInfoImpl::
      StubValueTy(Ctx.GetOrCreateSymbol(Name.str()), false);
    }
    return Sym;
  }

  // If the symbol reference is actually to a non_lazy_ptr, not to the symbol,
  // then add the suffix.
  if (MO.getTargetFlags() & PPCII::MO_NLP_FLAG) {
    Name += "$non_lazy_ptr";
    MCSymbol *Sym = Ctx.GetOrCreateSymbol(Name.str());
  
    MachineModuleInfoMachO &MachO = getMachOMMI(AP);
    
    MachineModuleInfoImpl::StubValueTy &StubSym =
      (MO.getTargetFlags() & PPCII::MO_NLP_HIDDEN_FLAG) ? 
         MachO.getHiddenGVStubEntry(Sym) : MachO.getGVStubEntry(Sym);
    
    if (StubSym.getPointer() == 0) {
      assert(MO.isGlobal() && "Extern symbol not handled yet");
      StubSym = MachineModuleInfoImpl::
                   StubValueTy(AP.Mang->getSymbol(MO.getGlobal()),
                               !MO.getGlobal()->hasInternalLinkage());
    }
    return Sym;
  }
  
  return Ctx.GetOrCreateSymbol(Name.str());
}

static MCOperand GetSymbolRef(const MachineOperand &MO, const MCSymbol *Symbol,
                              AsmPrinter &Printer, bool isDarwin) {
  MCContext &Ctx = Printer.OutContext;
  MCSymbolRefExpr::VariantKind RefKind = MCSymbolRefExpr::VK_None;

  unsigned access = MO.getTargetFlags() & PPCII::MO_ACCESS_MASK;

  if (!isDarwin) {
    switch (access) {
      case PPCII::MO_HA16:
        RefKind = MCSymbolRefExpr::VK_PPC_ADDR16_HA;
        break;
      case PPCII::MO_LO16:
        RefKind = MCSymbolRefExpr::VK_PPC_ADDR16_LO;
        break;
      case PPCII::MO_TPREL16_HA:
        RefKind = MCSymbolRefExpr::VK_PPC_TPREL16_HA;
        break;
      case PPCII::MO_TPREL16_LO:
        RefKind = MCSymbolRefExpr::VK_PPC_TPREL16_LO;
        break;
      case PPCII::MO_DTPREL16_LO:
        RefKind = MCSymbolRefExpr::VK_PPC_DTPREL16_LO;
        break;
      case PPCII::MO_TLSLD16_LO:
        RefKind = MCSymbolRefExpr::VK_PPC_GOT_TLSLD16_LO;
        break;
      case PPCII::MO_TOC16_LO:
        RefKind = MCSymbolRefExpr::VK_PPC_TOC16_LO;
        break;
    }
  }

  const MCExpr *Expr = MCSymbolRefExpr::Create(Symbol, RefKind, Ctx);

  if (!MO.isJTI() && MO.getOffset())
    Expr = MCBinaryExpr::CreateAdd(Expr,
                                   MCConstantExpr::Create(MO.getOffset(), Ctx),
                                   Ctx);

  // Subtract off the PIC base if required.
  if (MO.getTargetFlags() & PPCII::MO_PIC_FLAG) {
    const MachineFunction *MF = MO.getParent()->getParent()->getParent();
    
    const MCExpr *PB = MCSymbolRefExpr::Create(MF->getPICBaseSymbol(), Ctx);
    Expr = MCBinaryExpr::CreateSub(Expr, PB, Ctx);
  }

  // Add Darwin ha16() / lo16() markers if required.
  if (isDarwin) {
    switch (access) {
      case PPCII::MO_HA16:
        Expr = PPCMCExpr::CreateHa16(Expr, Ctx);
        break;
      case PPCII::MO_LO16:
        Expr = PPCMCExpr::CreateLo16(Expr, Ctx);
        break;
    }
  }

  return MCOperand::CreateExpr(Expr);
}

void llvm::LowerPPCMachineInstrToMCInst(const MachineInstr *MI, MCInst &OutMI,
                                        AsmPrinter &AP, bool isDarwin) {
  OutMI.setOpcode(MI->getOpcode());
  
  for (unsigned i = 0, e = MI->getNumOperands(); i != e; ++i) {
    const MachineOperand &MO = MI->getOperand(i);
    
    MCOperand MCOp;
    switch (MO.getType()) {
    default:
      MI->dump();
      llvm_unreachable("unknown operand type");
    case MachineOperand::MO_Register:
      assert(!MO.getSubReg() && "Subregs should be eliminated!");
      MCOp = MCOperand::CreateReg(MO.getReg());
      break;
    case MachineOperand::MO_Immediate:
      MCOp = MCOperand::CreateImm(MO.getImm());
      break;
    case MachineOperand::MO_MachineBasicBlock:
      MCOp = MCOperand::CreateExpr(MCSymbolRefExpr::Create(
                                      MO.getMBB()->getSymbol(), AP.OutContext));
      break;
    case MachineOperand::MO_GlobalAddress:
    case MachineOperand::MO_ExternalSymbol:
      MCOp = GetSymbolRef(MO, GetSymbolFromOperand(MO, AP), AP, isDarwin);
      break;
    case MachineOperand::MO_JumpTableIndex:
      MCOp = GetSymbolRef(MO, AP.GetJTISymbol(MO.getIndex()), AP, isDarwin);
      break;
    case MachineOperand::MO_ConstantPoolIndex:
      MCOp = GetSymbolRef(MO, AP.GetCPISymbol(MO.getIndex()), AP, isDarwin);
      break;
    case MachineOperand::MO_BlockAddress:
      MCOp = GetSymbolRef(MO,AP.GetBlockAddressSymbol(MO.getBlockAddress()),AP,
                          isDarwin);
      break;
    case MachineOperand::MO_RegisterMask:
      continue;
    }
    
    OutMI.addOperand(MCOp);
  }
}
