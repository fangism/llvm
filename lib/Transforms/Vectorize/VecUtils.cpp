//===- VecUtils.h --- Vectorization Utilities -----------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
#define DEBUG_TYPE "VecUtils"

#include "VecUtils.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/SmallSet.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ScalarEvolutionExpressions.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/Local.h"
#include <algorithm>
#include <map>

using namespace llvm;

namespace llvm {

BoUpSLP::BoUpSLP(BasicBlock *Bb, ScalarEvolution *S, DataLayout *Dl,
                             TargetTransformInfo *Tti, AliasAnalysis *Aa) :
                             BB(Bb), SE(S), DL(Dl), TTI(Tti), AA(Aa) {
  numberInstructions();
}

void BoUpSLP::numberInstructions() {
  int Loc = 0;
  InstrIdx.clear();
  InstrVec.clear();
  // Number the instructions in the block.
  for (BasicBlock::iterator it=BB->begin(), e=BB->end(); it != e; ++it) {
    InstrIdx[it] = Loc++;
    InstrVec.push_back(it);
    assert(InstrVec[InstrIdx[it]] == it && "Invalid allocation");
  }
}

Value *BoUpSLP::getPointerOperand(Value *I) {
  if (LoadInst *LI = dyn_cast<LoadInst>(I)) return LI->getPointerOperand();
  if (StoreInst *SI = dyn_cast<StoreInst>(I)) return SI->getPointerOperand();
  return 0;
}

unsigned BoUpSLP::getAddressSpaceOperand(Value *I) {
  if (LoadInst *L=dyn_cast<LoadInst>(I)) return L->getPointerAddressSpace();
  if (StoreInst *S=dyn_cast<StoreInst>(I)) return S->getPointerAddressSpace();
  return -1;
}

bool BoUpSLP::isConsecutiveAccess(Value *A, Value *B) {
  Value *PtrA = getPointerOperand(A);
  Value *PtrB = getPointerOperand(B);
  unsigned ASA = getAddressSpaceOperand(A);
  unsigned ASB = getAddressSpaceOperand(B);

  // Check that the address spaces match and that the pointers are valid.
  if (!PtrA || !PtrB || (ASA != ASB)) return false;

  // Check that A and B are of the same type.
  if (PtrA->getType() != PtrB->getType()) return false;

  // Calculate the distance.
  const SCEV *PtrSCEVA = SE->getSCEV(PtrA);
  const SCEV *PtrSCEVB = SE->getSCEV(PtrB);
  const SCEV *OffsetSCEV = SE->getMinusSCEV(PtrSCEVA, PtrSCEVB);
  const SCEVConstant *ConstOffSCEV = dyn_cast<SCEVConstant>(OffsetSCEV);

  // Non constant distance.
  if (!ConstOffSCEV) return false;

  unsigned Offset = ConstOffSCEV->getValue()->getSExtValue();
  Type *Ty = cast<PointerType>(PtrA->getType())->getElementType();
  // The Instructions are connsecutive if the size of the first load/store is
  // the same as the offset.
  unsigned Sz = (DL ? DL->getTypeStoreSize(Ty) : Ty->getScalarSizeInBits()/8);
  return ((-Offset) == Sz);
}

bool BoUpSLP::vectorizeStores(StoreList &Stores, int costThreshold) {
  ValueSet Heads, Tails;
  SmallDenseMap<Value*, Value*> ConsecutiveChain;
  bool Changed = false;

  // Do a quadratic search on all of the given stores and find
  // all of the pairs of loads that follow each other.
  for (unsigned i = 0, e = Stores.size(); i < e; ++i)
    for (unsigned j = 0; j < e; ++j) {
      if (i == j) continue;
      if (isConsecutiveAccess(Stores[i], Stores[j])) {
        Tails.insert(Stores[j]);
        Heads.insert(Stores[i]);
        ConsecutiveChain[Stores[i]] = Stores[j];
      }
    }

  // For stores that start but don't end a link in the chain:
  for (ValueSet::iterator it = Heads.begin(), e = Heads.end();it != e; ++it) {
    if (Tails.count(*it)) continue;

    // We found a store instr that starts a chain. Now follow the chain and try
    // to vectorize it.
    ValueList Operands;
    Value *I = *it;
    int MinCost = 0, MinVF = 0;
    while (Tails.count(I) || Heads.count(I)) {
      Operands.push_back(I);
      unsigned VF = Operands.size();
      if (isPowerOf2_32(VF) && VF > 1) {
        int cost = getTreeRollCost(Operands, 0);
        DEBUG(dbgs() << "Found cost=" << cost << " for VF=" << VF << "\n");
        if (cost < MinCost) { MinCost = cost; MinVF = VF; }
      }
      // Move to the next value in the chain.
      I = ConsecutiveChain[I];
    }

    if (MinCost <= costThreshold && MinVF > 1) {
      DEBUG(dbgs() << "Decided to vectorize cost=" << MinCost << "\n");
      vectorizeTree(Operands, MinVF);
      Stores.clear();
      // The current numbering is invalid because we added and removed instrs.
      numberInstructions();
      Changed = true;
    }
  }

  return Changed;
}

int BoUpSLP::getScalarizationCost(Type *Ty) {
  int Cost = 0;
  for (unsigned i = 0, e = cast<VectorType>(Ty)->getNumElements(); i < e; ++i)
    Cost += TTI->getVectorInstrCost(Instruction::InsertElement, Ty, i);
  return Cost;
}

AliasAnalysis::Location BoUpSLP::getLocation(Instruction *I) {
  if (StoreInst *SI = dyn_cast<StoreInst>(I)) return AA->getLocation(SI);
  if (LoadInst *LI = dyn_cast<LoadInst>(I)) return AA->getLocation(LI);
  return AliasAnalysis::Location();
}

Value *BoUpSLP::isUnsafeToSink(Instruction *Src, Instruction *Dst) {
  assert(Src->getParent() == Dst->getParent() && "Not the same BB");
  BasicBlock::iterator I = Src, E = Dst;
  /// Scan all of the instruction from SRC to DST and check if
  /// the source may alias.
  for (++I; I != E; ++I) {
    // Ignore store instructions that are marked as 'ignore'.
    if (MemBarrierIgnoreList.count(I)) continue;
    if (Src->mayWriteToMemory()) /* Write */ {
      if (!I->mayReadOrWriteMemory()) continue;
    } else /* Read */ {
      if (!I->mayWriteToMemory()) continue;
    }
    AliasAnalysis::Location A = getLocation(&*I);
    AliasAnalysis::Location B = getLocation(Src);

    if (!A.Ptr || !B.Ptr || AA->alias(A, B))
      return I;
  }
  return 0;
}

int BoUpSLP::getTreeRollCost(ValueList &VL, unsigned Depth) {
  if (Depth == 6) return max_cost;
  Type *ScalarTy = VL[0]->getType();

  if (StoreInst *SI = dyn_cast<StoreInst>(VL[0]))
    ScalarTy = SI->getValueOperand()->getType();

  /// Don't mess with vectors.
  if (ScalarTy->isVectorTy()) return max_cost;

  VectorType *VecTy = VectorType::get(ScalarTy, VL.size());

  // Check if all of the operands are constants.
  bool AllConst = true;
  bool AllSameScalar = true;
  for (unsigned i = 0, e = VL.size(); i < e; ++i) {
    AllConst &= isa<Constant>(VL[i]);
    AllSameScalar &= (VL[0] == VL[i]);
    // Must have a single use.
    Instruction *I = dyn_cast<Instruction>(VL[i]);
    // Need to scalarize instructions with multiple users or from other BBs.
    if (I && ((I->getNumUses() > 1) || (I->getParent() != BB)))
      return getScalarizationCost(VecTy);
  }

  // Is this a simple vector constant.
  if (AllConst) return 0;

  // If all of the operands are identical we can broadcast them.
  if (AllSameScalar)
    return TTI->getShuffleCost(TargetTransformInfo::SK_Broadcast, VecTy, 0);

  // Scalarize unknown structures.
  Instruction *VL0 = dyn_cast<Instruction>(VL[0]);
  if (!VL0) return getScalarizationCost(VecTy);
  assert(VL0->getParent() == BB && "Wrong BB");

  unsigned Opcode = VL0->getOpcode();
  for (unsigned i = 0, e = VL.size(); i < e; ++i) {
    Instruction *I = dyn_cast<Instruction>(VL[i]);
    // If not all of the instructions are identical then we have to scalarize.
    if (!I || Opcode != I->getOpcode()) return getScalarizationCost(VecTy);
  }

  // Check if it is safe to sink the loads or the stores.
  if (Opcode == Instruction::Load || Opcode == Instruction::Store) {
    int MaxIdx = InstrIdx[VL0];
    for (unsigned i = 1, e = VL.size(); i < e; ++i )
      MaxIdx = std::max(MaxIdx, InstrIdx[VL[i]]);

    Instruction *Last = InstrVec[MaxIdx];
    for (unsigned i = 0, e = VL.size(); i < e; ++i ) {
      if (VL[i] == Last) continue;
      Value *Barrier = isUnsafeToSink(cast<Instruction>(VL[i]), Last);
      if (Barrier) {
        DEBUG(dbgs() << "LR: Can't sink " << *VL[i] << "\n down to " <<
              *Last << "\n because of " << *Barrier << "\n");
        return max_cost;
      }
    }
  }

  switch (Opcode) {
  case Instruction::Add:
  case Instruction::FAdd:
  case Instruction::Sub:
  case Instruction::FSub:
  case Instruction::Mul:
  case Instruction::FMul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::FRem:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    ValueList Operands;
    int Cost = 0;
    // Calculate the cost of all of the operands.
    for (unsigned i = 0, e = VL0->getNumOperands(); i < e; ++i) {
      // Prepare the operand vector.
      for (unsigned j = 0; j < VL.size(); ++j)
        Operands.push_back(cast<Instruction>(VL[j])->getOperand(i));
      Cost += getTreeRollCost(Operands, Depth+1);
      Operands.clear();
    }

    // Calculate the cost of this instruction.
    int ScalarCost = VecTy->getNumElements() *
      TTI->getArithmeticInstrCost(Opcode, ScalarTy);
    int VecCost = TTI->getArithmeticInstrCost(Opcode, VecTy);
    Cost += (VecCost - ScalarCost);
    return Cost;
  }
  case Instruction::Load: {
    // If we are scalarize the loads, add the cost of forming the vector.
    for (unsigned i = 0, e = VL.size()-1; i < e; ++i)
      if (!isConsecutiveAccess(VL[i], VL[i+1]))
        return getScalarizationCost(VecTy);

    // Cost of wide load - cost of scalar loads.
    int ScalarLdCost = VecTy->getNumElements() *
      TTI->getMemoryOpCost(Instruction::Load, ScalarTy, 1, 0);
    int VecLdCost = TTI->getMemoryOpCost(Instruction::Load, ScalarTy, 1, 0);
    return VecLdCost - ScalarLdCost;
  }
  case Instruction::Store: {
    // We know that we can merge the stores. Calculate the cost.
    int ScalarStCost = VecTy->getNumElements() *
      TTI->getMemoryOpCost(Instruction::Store, ScalarTy, 1, 0);
    int VecStCost = TTI->getMemoryOpCost(Instruction::Store, ScalarTy, 1,0);
    int StoreCost = VecStCost - ScalarStCost;

    ValueList Operands;
    for (unsigned j = 0; j < VL.size(); ++j) {
      Operands.push_back(cast<Instruction>(VL[j])->getOperand(0));
      MemBarrierIgnoreList.insert(VL[j]);
    }

    int TotalCost =  StoreCost + getTreeRollCost(Operands, Depth + 1);
    MemBarrierIgnoreList.clear();
    return TotalCost;
  }
  default:
    // Unable to vectorize unknown instructions.
    return getScalarizationCost(VecTy);
  }
}

Instruction *BoUpSLP::GetLastInstr(ValueList &VL, unsigned VF) {
  int MaxIdx = InstrIdx[BB->getFirstNonPHI()];
  for (unsigned i = 0; i < VF; ++i )
    MaxIdx = std::max(MaxIdx, InstrIdx[VL[i]]);
  return InstrVec[MaxIdx + 1];
}

Value *BoUpSLP::Scalarize(ValueList &VL, VectorType *Ty) {
  IRBuilder<> Builder(GetLastInstr(VL, Ty->getNumElements()));
  Value *Vec = UndefValue::get(Ty);
  for (unsigned i=0; i < Ty->getNumElements(); ++i)
    Vec = Builder.CreateInsertElement(Vec, VL[i], Builder.getInt32(i));
  return Vec;
}

Value *BoUpSLP::vectorizeTree(ValueList &VL, int VF) {
  Type *ScalarTy = VL[0]->getType();
  if (StoreInst *SI = dyn_cast<StoreInst>(VL[0]))
    ScalarTy = SI->getValueOperand()->getType();
  VectorType *VecTy = VectorType::get(ScalarTy, VF);

  // Check if all of the operands are constants or identical.
  bool AllConst = true;
  bool AllSameScalar = true;
  for (unsigned i = 0, e = VF; i < e; ++i) {
    AllConst &= !!dyn_cast<Constant>(VL[i]);
    AllSameScalar &= (VL[0] == VL[i]);
    // Must have a single use.
    Instruction *I = dyn_cast<Instruction>(VL[i]);
    if (I && (I->getNumUses() > 1 || I->getParent() != BB))
      return Scalarize(VL, VecTy);
  }

  // Is this a simple vector constant.
  if (AllConst || AllSameScalar) return Scalarize(VL, VecTy);

  // Scalarize unknown structures.
  Instruction *VL0 = dyn_cast<Instruction>(VL[0]);
  if (!VL0) return Scalarize(VL, VecTy);

  unsigned Opcode = VL0->getOpcode();
  for (unsigned i = 0, e = VF; i < e; ++i) {
    Instruction *I = dyn_cast<Instruction>(VL[i]);
    // If not all of the instructions are identical then we have to scalarize.
    if (!I || Opcode != I->getOpcode()) return Scalarize(VL, VecTy);
  }

  switch (Opcode) {
  case Instruction::Add:
  case Instruction::FAdd:
  case Instruction::Sub:
  case Instruction::FSub:
  case Instruction::Mul:
  case Instruction::FMul:
  case Instruction::UDiv:
  case Instruction::SDiv:
  case Instruction::FDiv:
  case Instruction::URem:
  case Instruction::SRem:
  case Instruction::FRem:
  case Instruction::Shl:
  case Instruction::LShr:
  case Instruction::AShr:
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    ValueList LHSVL, RHSVL;
    for (int i = 0; i < VF; ++i) {
      RHSVL.push_back(cast<Instruction>(VL[i])->getOperand(0));
      LHSVL.push_back(cast<Instruction>(VL[i])->getOperand(1));
    }

    Value *RHS = vectorizeTree(RHSVL, VF);
    Value *LHS = vectorizeTree(LHSVL, VF);
    IRBuilder<> Builder(GetLastInstr(VL, VF));
    BinaryOperator *BinOp = dyn_cast<BinaryOperator>(VL0);
    return Builder.CreateBinOp(BinOp->getOpcode(), RHS,LHS);
  }
  case Instruction::Load: {
    LoadInst *LI = dyn_cast<LoadInst>(VL0);
    unsigned Alignment = LI->getAlignment();

    // Check if all of the loads are consecutive.
    for (unsigned i = 1, e = VF; i < e; ++i)
      if (!isConsecutiveAccess(VL[i-1], VL[i]))
        return Scalarize(VL, VecTy);

    IRBuilder<> Builder(GetLastInstr(VL, VF));
    Value *VecPtr = Builder.CreateBitCast(LI->getPointerOperand(),
                                          VecTy->getPointerTo());
    LI = Builder.CreateLoad(VecPtr);
    LI->setAlignment(Alignment);
    return LI;
  }
  case Instruction::Store: {
    StoreInst *SI = dyn_cast<StoreInst>(VL0);
    unsigned Alignment = SI->getAlignment();

    ValueList ValueOp;
    for (int i = 0; i < VF; ++i)
      ValueOp.push_back(cast<StoreInst>(VL[i])->getValueOperand());

    Value *VecValue = vectorizeTree(ValueOp, VF);

    IRBuilder<> Builder(GetLastInstr(VL, VF));
    Value *VecPtr = Builder.CreateBitCast(SI->getPointerOperand(),
                                          VecTy->getPointerTo());
    Builder.CreateStore(VecValue, VecPtr)->setAlignment(Alignment);

    for (int i = 0; i < VF; ++i)
      cast<Instruction>(VL[i])->eraseFromParent();
    return 0;
  }
  default:
    return Scalarize(VL, VecTy);
  }
}

} // end of namespace
