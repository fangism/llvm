//===-- SystemZLongBranch.cpp - Branch lengthening for SystemZ ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass makes sure that all branches are in range.  There are several ways
// in which this could be done.  One aggressive approach is to assume that all
// branches are in range and successively replace those that turn out not
// to be in range with a longer form (branch relaxation).  A simple
// implementation is to continually walk through the function relaxing
// branches until no more changes are needed and a fixed point is reached.
// However, in the pathological worst case, this implementation is
// quadratic in the number of blocks; relaxing branch N can make branch N-1
// go out of range, which in turn can make branch N-2 go out of range,
// and so on.
//
// An alternative approach is to assume that all branches must be
// converted to their long forms, then reinstate the short forms of
// branches that, even under this pessimistic assumption, turn out to be
// in range (branch shortening).  This too can be implemented as a function
// walk that is repeated until a fixed point is reached.  In general,
// the result of shortening is not as good as that of relaxation, and
// shortening is also quadratic in the worst case; shortening branch N
// can bring branch N-1 in range of the short form, which in turn can do
// the same for branch N-2, and so on.  The main advantage of shortening
// is that each walk through the function produces valid code, so it is
// possible to stop at any point after the first walk.  The quadraticness
// could therefore be handled with a maximum pass count, although the
// question then becomes: what maximum count should be used?
//
// On SystemZ, long branches are only needed for functions bigger than 64k,
// which are relatively rare to begin with, and the long branch sequences
// are actually relatively cheap.  It therefore doesn't seem worth spending
// much compilation time on the problem.  Instead, the approach we take is:
//
// (1) Check whether all branches can be short (the usual case).  Exit the
//     pass if so.
// (2) If one branch needs to be long, work out the address that each block
//     would have if all branches need to be long, as for shortening above.
// (3) Relax any branch that is out of range according to this pessimistic
//     assumption.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "systemz-long-branch"

#include "SystemZTargetMachine.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Target/TargetInstrInfo.h"
#include "llvm/Target/TargetMachine.h"
#include "llvm/Target/TargetRegisterInfo.h"

using namespace llvm;

STATISTIC(LongBranches, "Number of long branches.");

namespace {
  typedef MachineBasicBlock::iterator Iter;

  // Represents positional information about a basic block.
  struct MBBInfo {
    // The address that we currently assume the block has, relative to
    // the start of the function.  This is designed so that taking the
    // difference between two addresses gives a conservative upper bound
    // on the distance between them.
    uint64_t Address;

    // The size of the block in bytes, excluding terminators.
    // This value never changes.
    uint64_t Size;

    // The minimum alignment of the block, as a log2 value.
    // This value never changes.
    unsigned Alignment;

    // The number of terminators in this block.  This value never changes.
    unsigned NumTerminators;

    MBBInfo()
      : Address(0), Size(0), Alignment(0), NumTerminators(0) {} 
  };

  // Represents the state of a block terminator.
  struct TerminatorInfo {
    // If this terminator is a relaxable branch, this points to the branch
    // instruction, otherwise it is null.
    MachineInstr *Branch;

    // The current address of the terminator, in the same form as
    // for BlockInfo.
    uint64_t Address;

    // The current size of the terminator in bytes.
    uint64_t Size;

    // If Branch is nonnull, this is the number of the target block,
    // otherwise it is unused.
    unsigned TargetBlock;

    // If Branch is nonnull, this is the length of the longest relaxed form,
    // otherwise it is zero.
    unsigned ExtraRelaxSize;

    TerminatorInfo() : Branch(0), Size(0), TargetBlock(0), ExtraRelaxSize(0) {}
  };

  // Used to keep track of the current position while iterating over the blocks.
  struct BlockPosition {
    // The offset from the start of the function, in the same form
    // as BlockInfo.
    uint64_t Address;

    // The number of low bits in Address that are known to be the same
    // as the runtime address.
    unsigned KnownBits;

    BlockPosition(unsigned InitialAlignment)
      : Address(0), KnownBits(InitialAlignment) {}
  };

  class SystemZLongBranch : public MachineFunctionPass {
  public:
    static char ID;
    SystemZLongBranch(const SystemZTargetMachine &tm)
      : MachineFunctionPass(ID),
        TII(static_cast<const SystemZInstrInfo *>(tm.getInstrInfo())) {}

    virtual const char *getPassName() const {
      return "SystemZ Long Branch";
    }

    bool runOnMachineFunction(MachineFunction &F);

  private:
    void skipNonTerminators(BlockPosition &Position, MBBInfo &Block);
    void skipTerminator(BlockPosition &Position, TerminatorInfo &Terminator,
                        bool AssumeRelaxed);
    TerminatorInfo describeTerminator(MachineInstr *MI);
    uint64_t initMBBInfo();
    bool mustRelaxBranch(const TerminatorInfo &Terminator);
    bool mustRelaxABranch();
    void setWorstCaseAddresses();
    void relaxBranch(TerminatorInfo &Terminator);
    void relaxBranches();

    const SystemZInstrInfo *TII;
    MachineFunction *MF;
    SmallVector<MBBInfo, 16> MBBs;
    SmallVector<TerminatorInfo, 16> Terminators;
  };

  char SystemZLongBranch::ID = 0;

  const uint64_t MaxBackwardRange = 0x10000;
  const uint64_t MaxForwardRange = 0xfffe;
} // end of anonymous namespace

FunctionPass *llvm::createSystemZLongBranchPass(SystemZTargetMachine &TM) {
  return new SystemZLongBranch(TM);
}

// Position describes the state immediately before Block.  Update Block
// accordingly and move Position to the end of the block's non-terminator
// instructions.
void SystemZLongBranch::skipNonTerminators(BlockPosition &Position,
                                           MBBInfo &Block) {
  if (Block.Alignment > Position.KnownBits) {
    // When calculating the address of Block, we need to conservatively
    // assume that Block had the worst possible misalignment.
    Position.Address += ((uint64_t(1) << Block.Alignment) -
                         (uint64_t(1) << Position.KnownBits));
    Position.KnownBits = Block.Alignment;
  }

  // Align the addresses.
  uint64_t AlignMask = (uint64_t(1) << Block.Alignment) - 1;
  Position.Address = (Position.Address + AlignMask) & ~AlignMask;

  // Record the block's position.
  Block.Address = Position.Address;

  // Move past the non-terminators in the block.
  Position.Address += Block.Size;
}

// Position describes the state immediately before Terminator.
// Update Terminator accordingly and move Position past it.
// Assume that Terminator will be relaxed if AssumeRelaxed.
void SystemZLongBranch::skipTerminator(BlockPosition &Position,
                                       TerminatorInfo &Terminator,
                                       bool AssumeRelaxed) {
  Terminator.Address = Position.Address;
  Position.Address += Terminator.Size;
  if (AssumeRelaxed)
    Position.Address += Terminator.ExtraRelaxSize;
}

// Return a description of terminator instruction MI.
TerminatorInfo SystemZLongBranch::describeTerminator(MachineInstr *MI) {
  TerminatorInfo Terminator;
  Terminator.Size = TII->getInstSizeInBytes(MI);
  if (MI->isConditionalBranch() || MI->isUnconditionalBranch()) {
    Terminator.Branch = MI;
    switch (MI->getOpcode()) {
    case SystemZ::J:
      // Relaxes to JG, which is 2 bytes longer.
      Terminator.TargetBlock = MI->getOperand(0).getMBB()->getNumber();
      Terminator.ExtraRelaxSize = 2;
      break;
    case SystemZ::BRC:
      // Relaxes to BRCL, which is 2 bytes longer.  Operand 0 is the
      // condition code mask.
      Terminator.TargetBlock = MI->getOperand(1).getMBB()->getNumber();
      Terminator.ExtraRelaxSize = 2;
      break;
    default:
      llvm_unreachable("Unrecognized branch instruction");
    }
  }
  return Terminator;
}

// Fill MBBs and Terminators, setting the addresses on the assumption
// that no branches need relaxation.  Return the size of the function under
// this assumption.
uint64_t SystemZLongBranch::initMBBInfo() {
  MF->RenumberBlocks();
  unsigned NumBlocks = MF->size();

  MBBs.clear();
  MBBs.resize(NumBlocks);

  Terminators.clear();
  Terminators.reserve(NumBlocks);

  BlockPosition Position(MF->getAlignment());
  for (unsigned I = 0; I < NumBlocks; ++I) {
    MachineBasicBlock *MBB = MF->getBlockNumbered(I);
    MBBInfo &Block = MBBs[I];

    // Record the alignment, for quick access.
    Block.Alignment = MBB->getAlignment();

    // Calculate the size of the fixed part of the block.
    MachineBasicBlock::iterator MI = MBB->begin();
    MachineBasicBlock::iterator End = MBB->end();
    while (MI != End && !MI->isTerminator()) {
      Block.Size += TII->getInstSizeInBytes(MI);
      ++MI;
    }
    skipNonTerminators(Position, Block);

    // Add the terminators.
    while (MI != End) {
      if (!MI->isDebugValue()) {
        assert(MI->isTerminator() && "Terminator followed by non-terminator");
        Terminators.push_back(describeTerminator(MI));
        skipTerminator(Position, Terminators.back(), false);
        ++Block.NumTerminators;
      }
      ++MI;
    }
  }

  return Position.Address;
}

// Return true if, under current assumptions, Terminator needs to be relaxed.
bool SystemZLongBranch::mustRelaxBranch(const TerminatorInfo &Terminator) {
  if (!Terminator.Branch)
    return false;

  const MBBInfo &Target = MBBs[Terminator.TargetBlock];
  if (Target.Address < Terminator.Address) {
    if (Terminator.Address - Target.Address <= MaxBackwardRange)
      return false;
  } else {
    if (Target.Address - Terminator.Address <= MaxForwardRange)
      return false;
  }

  return true;
}

// Return true if, under current assumptions, any terminator needs
// to be relaxed.
bool SystemZLongBranch::mustRelaxABranch() {
  for (SmallVector<TerminatorInfo, 16>::iterator TI = Terminators.begin(),
         TE = Terminators.end(); TI != TE; ++TI)
    if (mustRelaxBranch(*TI))
      return true;
  return false;
}

// Set the address of each block on the assumption that all branches
// must be long.
void SystemZLongBranch::setWorstCaseAddresses() {
  SmallVector<TerminatorInfo, 16>::iterator TI = Terminators.begin();
  BlockPosition Position(MF->getAlignment());
  for (SmallVector<MBBInfo, 16>::iterator BI = MBBs.begin(), BE = MBBs.end();
       BI != BE; ++BI) {
    skipNonTerminators(Position, *BI);
    for (unsigned BTI = 0, BTE = BI->NumTerminators; BTI != BTE; ++BTI) {
      skipTerminator(Position, *TI, true);
      ++TI;
    }
  }
}

// Relax the branch described by Terminator.
void SystemZLongBranch::relaxBranch(TerminatorInfo &Terminator) {
  MachineInstr *Branch = Terminator.Branch;
  switch (Branch->getOpcode()) {
    case SystemZ::J:
      Branch->setDesc(TII->get(SystemZ::JG));
      break;
    case SystemZ::BRC:
      Branch->setDesc(TII->get(SystemZ::BRCL));
      break;
    default:
      llvm_unreachable("Unrecognized branch");
    }

  Terminator.Size += Terminator.ExtraRelaxSize;
  Terminator.ExtraRelaxSize = 0;
  Terminator.Branch = 0;

  ++LongBranches;
}

// Relax any branches that need to be relaxed, under current assumptions.
void SystemZLongBranch::relaxBranches() {
  for (SmallVector<TerminatorInfo, 16>::iterator TI = Terminators.begin(),
         TE = Terminators.end(); TI != TE; ++TI)
    if (mustRelaxBranch(*TI))
      relaxBranch(*TI);
}

bool SystemZLongBranch::runOnMachineFunction(MachineFunction &F) {
  MF = &F;
  uint64_t Size = initMBBInfo();
  if (Size <= MaxForwardRange || !mustRelaxABranch())
    return false;

  setWorstCaseAddresses();
  relaxBranches();
  return true;
}
