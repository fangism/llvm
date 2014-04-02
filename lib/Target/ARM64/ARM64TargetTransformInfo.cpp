//===-- ARM64TargetTransformInfo.cpp - ARM64 specific TTI pass ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
/// \file
/// This file implements a TargetTransformInfo analysis pass specific to the
/// ARM64 target machine. It uses the target's detailed information to provide
/// more precise answers to certain TTI queries, while letting the target
/// independent and default TTI implementations handle the rest.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "arm64tti"
#include "ARM64.h"
#include "ARM64TargetMachine.h"
#include "MCTargetDesc/ARM64AddressingModes.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Target/CostTable.h"
#include "llvm/Target/TargetLowering.h"
using namespace llvm;

// Declare the pass initialization routine locally as target-specific passes
// don't havve a target-wide initialization entry point, and so we rely on the
// pass constructor initialization.
namespace llvm {
void initializeARM64TTIPass(PassRegistry &);
}

namespace {

class ARM64TTI final : public ImmutablePass, public TargetTransformInfo {
  const ARM64TargetMachine *TM;
  const ARM64Subtarget *ST;
  const ARM64TargetLowering *TLI;

  /// Estimate the overhead of scalarizing an instruction. Insert and Extract
  /// are set if the result needs to be inserted and/or extracted from vectors.
  unsigned getScalarizationOverhead(Type *Ty, bool Insert, bool Extract) const;

public:
  ARM64TTI() : ImmutablePass(ID), TM(0), ST(0), TLI(0) {
    llvm_unreachable("This pass cannot be directly constructed");
  }

  ARM64TTI(const ARM64TargetMachine *TM)
      : ImmutablePass(ID), TM(TM), ST(TM->getSubtargetImpl()),
        TLI(TM->getTargetLowering()) {
    initializeARM64TTIPass(*PassRegistry::getPassRegistry());
  }

  void initializePass() override { pushTTIStack(this); }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    TargetTransformInfo::getAnalysisUsage(AU);
  }

  /// Pass identification.
  static char ID;

  /// Provide necessary pointer adjustments for the two base classes.
  void *getAdjustedAnalysisPointer(const void *ID) override {
    if (ID == &TargetTransformInfo::ID)
      return (TargetTransformInfo *)this;
    return this;
  }

  /// \name Scalar TTI Implementations
  /// @{

  unsigned getIntImmCost(const APInt &Imm, Type *Ty) const override;
  PopcntSupportKind getPopcntSupport(unsigned TyWidth) const override;

  /// @}

  /// \name Vector TTI Implementations
  /// @{

  unsigned getNumberOfRegisters(bool Vector) const override {
    if (Vector)
      return 32;

    return 31;
  }

  unsigned getRegisterBitWidth(bool Vector) const override {
    if (Vector)
      return 128;

    return 64;
  }

  unsigned getMaximumUnrollFactor() const override { return 2; }

  unsigned getCastInstrCost(unsigned Opcode, Type *Dst, Type *Src) const
      override;

  unsigned getVectorInstrCost(unsigned Opcode, Type *Val, unsigned Index) const
      override;

  unsigned getArithmeticInstrCost(unsigned Opcode, Type *Ty,
                                  OperandValueKind Opd1Info = OK_AnyValue,
                                  OperandValueKind Opd2Info = OK_AnyValue) const
      override;

  unsigned getAddressComputationCost(Type *Ty, bool IsComplex) const override;

  unsigned getCmpSelInstrCost(unsigned Opcode, Type *ValTy, Type *CondTy) const
      override;

  unsigned getMemoryOpCost(unsigned Opcode, Type *Src, unsigned Alignment,
                           unsigned AddressSpace) const override;
  /// @}
};

} // end anonymous namespace

INITIALIZE_AG_PASS(ARM64TTI, TargetTransformInfo, "arm64tti",
                   "ARM64 Target Transform Info", true, true, false)
char ARM64TTI::ID = 0;

ImmutablePass *
llvm::createARM64TargetTransformInfoPass(const ARM64TargetMachine *TM) {
  return new ARM64TTI(TM);
}

unsigned ARM64TTI::getIntImmCost(const APInt &Imm, Type *Ty) const {
  assert(Ty->isIntegerTy());

  unsigned BitSize = Ty->getPrimitiveSizeInBits();
  if (BitSize == 0)
    return ~0U;

  int64_t Val = Imm.getSExtValue();
  if (Val == 0 || ARM64_AM::isLogicalImmediate(Val, BitSize))
    return 1;

  if ((int64_t)Val < 0)
    Val = ~Val;
  if (BitSize == 32)
    Val &= (1LL << 32) - 1;

  unsigned LZ = countLeadingZeros((uint64_t)Val);
  unsigned Shift = (63 - LZ) / 16;
  // MOVZ is free so return true for one or fewer MOVK.
  return (Shift == 0) ? 1 : Shift;
}

ARM64TTI::PopcntSupportKind ARM64TTI::getPopcntSupport(unsigned TyWidth) const {
  assert(isPowerOf2_32(TyWidth) && "Ty width must be power of 2");
  if (TyWidth == 32 || TyWidth == 64)
    return PSK_FastHardware;
  // TODO: ARM64TargetLowering::LowerCTPOP() supports 128bit popcount.
  return PSK_Software;
}

unsigned ARM64TTI::getCastInstrCost(unsigned Opcode, Type *Dst,
                                    Type *Src) const {
  int ISD = TLI->InstructionOpcodeToISD(Opcode);
  assert(ISD && "Invalid opcode");

  EVT SrcTy = TLI->getValueType(Src);
  EVT DstTy = TLI->getValueType(Dst);

  if (!SrcTy.isSimple() || !DstTy.isSimple())
    return TargetTransformInfo::getCastInstrCost(Opcode, Dst, Src);

  static const TypeConversionCostTblEntry<MVT> ConversionTbl[] = {
    // LowerVectorINT_TO_FP:
    { ISD::SINT_TO_FP, MVT::v2f32, MVT::v2i32, 1 },
    { ISD::SINT_TO_FP, MVT::v2f64, MVT::v2i8, 1 },
    { ISD::SINT_TO_FP, MVT::v2f64, MVT::v2i16, 1 },
    { ISD::SINT_TO_FP, MVT::v2f64, MVT::v2i32, 1 },
    { ISD::SINT_TO_FP, MVT::v2f64, MVT::v2i64, 1 },
    { ISD::UINT_TO_FP, MVT::v2f32, MVT::v2i32, 1 },
    { ISD::UINT_TO_FP, MVT::v2f64, MVT::v2i8, 1 },
    { ISD::UINT_TO_FP, MVT::v2f64, MVT::v2i16, 1 },
    { ISD::UINT_TO_FP, MVT::v2f64, MVT::v2i32, 1 },
    { ISD::UINT_TO_FP, MVT::v2f64, MVT::v2i64, 1 },
    // LowerVectorFP_TO_INT
    { ISD::FP_TO_SINT, MVT::v4i32, MVT::v4f32, 1 },
    { ISD::FP_TO_SINT, MVT::v2i64, MVT::v2f64, 1 },
    { ISD::FP_TO_UINT, MVT::v4i32, MVT::v4f32, 1 },
    { ISD::FP_TO_UINT, MVT::v2i64, MVT::v2f64, 1 },
    { ISD::FP_TO_UINT, MVT::v2i32, MVT::v2f64, 1 },
    { ISD::FP_TO_SINT, MVT::v2i32, MVT::v2f64, 1 },
    { ISD::FP_TO_UINT, MVT::v2i64, MVT::v2f64, 4 },
    { ISD::FP_TO_SINT, MVT::v2i64, MVT::v2f64, 4 },
  };

  int Idx = ConvertCostTableLookup<MVT>(
      ConversionTbl, array_lengthof(ConversionTbl), ISD, DstTy.getSimpleVT(),
      SrcTy.getSimpleVT());
  if (Idx != -1)
    return ConversionTbl[Idx].Cost;

  return TargetTransformInfo::getCastInstrCost(Opcode, Dst, Src);
}

unsigned ARM64TTI::getVectorInstrCost(unsigned Opcode, Type *Val,
                                      unsigned Index) const {
  assert(Val->isVectorTy() && "This must be a vector type");

  if (Index != -1U) {
    // Legalize the type.
    std::pair<unsigned, MVT> LT = TLI->getTypeLegalizationCost(Val);

    // This type is legalized to a scalar type.
    if (!LT.second.isVector())
      return 0;

    // The type may be split. Normalize the index to the new type.
    unsigned Width = LT.second.getVectorNumElements();
    Index = Index % Width;

    // The element at index zero is already inside the vector.
    if (Index == 0)
      return 0;
  }

  // All other insert/extracts cost this much.
  return 2;
}

unsigned ARM64TTI::getArithmeticInstrCost(unsigned Opcode, Type *Ty,
                                          OperandValueKind Opd1Info,
                                          OperandValueKind Opd2Info) const {
  // Legalize the type.
  std::pair<unsigned, MVT> LT = TLI->getTypeLegalizationCost(Ty);

  int ISD = TLI->InstructionOpcodeToISD(Opcode);

  switch (ISD) {
  default:
    return TargetTransformInfo::getArithmeticInstrCost(Opcode, Ty, Opd1Info,
                                                       Opd2Info);
  case ISD::ADD:
  case ISD::MUL:
  case ISD::XOR:
  case ISD::OR:
  case ISD::AND:
    // These nodes are marked as 'custom' for combining purposes only.
    // We know that they are legal. See LowerAdd in ISelLowering.
    return 1 * LT.first;
  }
}

unsigned ARM64TTI::getAddressComputationCost(Type *Ty, bool IsComplex) const {
  // Address computations in vectorized code with non-consecutive addresses will
  // likely result in more instructions compared to scalar code where the
  // computation can more often be merged into the index mode. The resulting
  // extra micro-ops can significantly decrease throughput.
  unsigned NumVectorInstToHideOverhead = 10;

  if (Ty->isVectorTy() && IsComplex)
    return NumVectorInstToHideOverhead;

  // In many cases the address computation is not merged into the instruction
  // addressing mode.
  return 1;
}

unsigned ARM64TTI::getCmpSelInstrCost(unsigned Opcode, Type *ValTy,
                                      Type *CondTy) const {

  int ISD = TLI->InstructionOpcodeToISD(Opcode);
  // We don't lower vector selects well that are wider than the register width.
  if (ValTy->isVectorTy() && ISD == ISD::SELECT) {
    // We would need this many instructions to hide the scalarization happening.
    unsigned AmortizationCost = 20;
    static const TypeConversionCostTblEntry<MVT::SimpleValueType>
    VectorSelectTbl[] = {
      { ISD::SELECT, MVT::v16i1, MVT::v16i16, 16 * AmortizationCost },
      { ISD::SELECT, MVT::v8i1, MVT::v8i32, 8 * AmortizationCost },
      { ISD::SELECT, MVT::v16i1, MVT::v16i32, 16 * AmortizationCost },
      { ISD::SELECT, MVT::v4i1, MVT::v4i64, 4 * AmortizationCost },
      { ISD::SELECT, MVT::v8i1, MVT::v8i64, 8 * AmortizationCost },
      { ISD::SELECT, MVT::v16i1, MVT::v16i64, 16 * AmortizationCost }
    };

    EVT SelCondTy = TLI->getValueType(CondTy);
    EVT SelValTy = TLI->getValueType(ValTy);
    if (SelCondTy.isSimple() && SelValTy.isSimple()) {
      int Idx =
          ConvertCostTableLookup(VectorSelectTbl, ISD, SelCondTy.getSimpleVT(),
                                 SelValTy.getSimpleVT());
      if (Idx != -1)
        return VectorSelectTbl[Idx].Cost;
    }
  }
  return TargetTransformInfo::getCmpSelInstrCost(Opcode, ValTy, CondTy);
}

unsigned ARM64TTI::getMemoryOpCost(unsigned Opcode, Type *Src,
                                   unsigned Alignment,
                                   unsigned AddressSpace) const {
  std::pair<unsigned, MVT> LT = TLI->getTypeLegalizationCost(Src);

  if (Opcode == Instruction::Store && Src->isVectorTy() && Alignment != 16 &&
      Src->getVectorElementType()->isIntegerTy(64)) {
    // Unaligned stores are extremely inefficient. We don't split
    // unaligned v2i64 stores because the negative impact that has shown in
    // practice on inlined memcpy code.
    // We make v2i64 stores expensive so that we will only vectorize if there
    // are 6 other instructions getting vectorized.
    unsigned AmortizationCost = 6;

    return LT.first * 2 * AmortizationCost;
  }

  if (Src->isVectorTy() && Src->getVectorElementType()->isIntegerTy(8) &&
      Src->getVectorNumElements() < 8) {
    // We scalarize the loads/stores because there is not v.4b register and we
    // have to promote the elements to v.4h.
    unsigned NumVecElts = Src->getVectorNumElements();
    unsigned NumVectorizableInstsToAmortize = NumVecElts * 2;
    // We generate 2 instructions per vector element.
    return NumVectorizableInstsToAmortize * NumVecElts * 2;
  }

  return LT.first;
}
