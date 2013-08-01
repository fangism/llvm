//===-- SystemZLongBranch.cpp - Branch lengthening for SystemZ ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass does three things:
// (1) try to remove compares if CC already contains the required information
// (2) fuse compares and branches into COMPARE AND BRANCH instructions
// (3) make sure that all branches are in range.
//
// We do (1) here rather than earlier because some transformations can
// change the set of available CC values and we generally want those
// transformations to have priority over (1).  This is especially true in
// the commonest case where the CC value is used by a single in-range branch
// instruction, since (2) will then be able to fuse the compare and the
// branch instead.
//
// For example, two-address NILF can sometimes be converted into
// three-address RISBLG.  NILF produces a CC value that indicates whether
// the low word is zero, but RISBLG does not modify CC at all.  On the
// other hand, 64-bit ANDs like NILL can sometimes be converted to RISBG.
// The CC value produced by NILL isn't useful for our purposes, but the
// value produced by RISBG can be used for any comparison with zero
// (not just equality).  So there are some transformations that lose
// CC values (while still being worthwhile) and others that happen to make
// the CC result more useful than it was originally.
//
// We do (2) here rather than earlier because the fused form prevents
// predication.  It also has to happen after (1).
//
// Doing (2) so late makes it more likely that a register will be reused
// between the compare and the branch, but it isn't clear whether preventing
// that would be a win or not.
//
// There are several ways in which (3) could be done.  One aggressive
// approach is to assume that all branches are in range and successively
// replace those that turn out not to be in range with a longer form
// (branch relaxation).  A simple implementation is to continually walk
// through the function relaxing branches until no more changes are
// needed and a fixed point is reached.  However, in the pathological
// worst case, this implementation is quadratic in the number of blocks;
// relaxing branch N can make branch N-1 go out of range, which in turn
// can make branch N-2 go out of range, and so on.
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
// (1) Work out the address that each block would have if no branches
//     need relaxing.  Exit the pass early if all branches are in range
//     according to this assumption.
//
// (2) Work out the address that each block would have if all branches
//     need relaxing.
//
// (3) Walk through the block calculating the final address of each instruction
//     and relaxing those that need to be relaxed.  For backward branches,
//     this check uses the final address of the target block, as calculated
//     earlier in the walk.  For forward branches, this check uses the
//     address of the target block that was calculated in (2).  Both checks
//     give a conservatively-correct range.
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
    // The address that we currently assume the block has.
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

    // The address that we currently assume the terminator has.
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
    // The address that we assume this position has.
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
      : MachineFunctionPass(ID), TII(0) {}

    virtual const char *getPassName() const {
      return "SystemZ Long Branch";
    }

    bool runOnMachineFunction(MachineFunction &F);

  private:
    void skipNonTerminators(BlockPosition &Position, MBBInfo &Block);
    void skipTerminator(BlockPosition &Position, TerminatorInfo &Terminator,
                        bool AssumeRelaxed);
    TerminatorInfo describeTerminator(MachineInstr *MI);
    bool optimizeCompareZero(MachineInstr *PrevCCSetter, MachineInstr *Compare);
    bool fuseCompareAndBranch(MachineInstr *Compare);
    uint64_t initMBBInfo();
    bool mustRelaxBranch(const TerminatorInfo &Terminator, uint64_t Address);
    bool mustRelaxABranch();
    void setWorstCaseAddresses();
    void splitCompareBranch(MachineInstr *MI, unsigned CompareOpcode);
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
    switch (MI->getOpcode()) {
    case SystemZ::J:
      // Relaxes to JG, which is 2 bytes longer.
      Terminator.ExtraRelaxSize = 2;
      break;
    case SystemZ::BRC:
      // Relaxes to BRCL, which is 2 bytes longer.
      Terminator.ExtraRelaxSize = 2;
      break;
    case SystemZ::CRJ:
      // Relaxes to a CR/BRCL sequence, which is 2 bytes longer.
      Terminator.ExtraRelaxSize = 2;
      break;
    case SystemZ::CGRJ:
      // Relaxes to a CGR/BRCL sequence, which is 4 bytes longer.
      Terminator.ExtraRelaxSize = 4;
      break;
    case SystemZ::CIJ:
    case SystemZ::CGIJ:
      // Relaxes to a C(G)HI/BRCL sequence, which is 4 bytes longer.
      Terminator.ExtraRelaxSize = 4;
      break;
    default:
      llvm_unreachable("Unrecognized branch instruction");
    }
    Terminator.Branch = MI;
    Terminator.TargetBlock =
      TII->getBranchInfo(MI).Target->getMBB()->getNumber();
  }
  return Terminator;
}

// Return true if CC is live out of MBB.
static bool isCCLiveOut(MachineBasicBlock *MBB) {
  for (MachineBasicBlock::succ_iterator SI = MBB->succ_begin(),
         SE = MBB->succ_end(); SI != SE; ++SI)
    if ((*SI)->isLiveIn(SystemZ::CC))
      return true;
  return false;
}

// Return true if CC is live after MBBI.
static bool isCCLiveAfter(MachineBasicBlock::iterator MBBI,
                          const TargetRegisterInfo *TRI) {
  if (MBBI->killsRegister(SystemZ::CC, TRI))
    return false;

  MachineBasicBlock *MBB = MBBI->getParent();
  MachineBasicBlock::iterator MBBE = MBB->end();
  for (++MBBI; MBBI != MBBE; ++MBBI) {
    if (MBBI->readsRegister(SystemZ::CC, TRI))
      return true;
    if (MBBI->definesRegister(SystemZ::CC, TRI))
      return false;
  }

  return isCCLiveOut(MBB);
}

// Return true if all uses of the CC value produced by MBBI could make do
// with the CC values in ReusableCCMask.  When returning true, point AlterMasks
// to the "CC valid" and "CC mask" operands for each condition.
static bool canRestrictCCMask(MachineBasicBlock::iterator MBBI,
                              unsigned ReusableCCMask,
                              SmallVectorImpl<MachineOperand *> &AlterMasks,
                              const TargetRegisterInfo *TRI) {
  MachineBasicBlock *MBB = MBBI->getParent();
  MachineBasicBlock::iterator MBBE = MBB->end();
  for (++MBBI; MBBI != MBBE; ++MBBI) {
    if (MBBI->readsRegister(SystemZ::CC, TRI)) {
      // Fail if this isn't a use of CC that we understand.
      unsigned MBBIFlags = MBBI->getDesc().TSFlags;
      unsigned FirstOpNum;
      if (MBBIFlags & SystemZII::CCMaskFirst)
        FirstOpNum = 0;
      else if (MBBIFlags & SystemZII::CCMaskLast)
        FirstOpNum = MBBI->getNumExplicitOperands() - 2;
      else
        return false;

      // Check whether the instruction predicate treats all CC values
      // outside of ReusableCCMask in the same way.  In that case it
      // doesn't matter what those CC values mean.
      unsigned CCValid = MBBI->getOperand(FirstOpNum).getImm();
      unsigned CCMask = MBBI->getOperand(FirstOpNum + 1).getImm();
      unsigned OutValid = ~ReusableCCMask & CCValid;
      unsigned OutMask = ~ReusableCCMask & CCMask;
      if (OutMask != 0 && OutMask != OutValid)
        return false;

      AlterMasks.push_back(&MBBI->getOperand(FirstOpNum));
      AlterMasks.push_back(&MBBI->getOperand(FirstOpNum + 1));

      // Succeed if this was the final use of the CC value.
      if (MBBI->killsRegister(SystemZ::CC, TRI))
        return true;
    }
    // Succeed if the instruction redefines CC.
    if (MBBI->definesRegister(SystemZ::CC, TRI))
      return true;
  }
  // Fail if there are other uses of CC that we didn't see.
  return !isCCLiveOut(MBB);
}

// Try to make Compare redundant with PrevCCSetter, the previous setter of CC,
// by looking for cases where Compare compares the result of PrevCCSetter
// against zero.  Return true on success and if Compare can therefore
// be deleted.
bool SystemZLongBranch::optimizeCompareZero(MachineInstr *PrevCCSetter,
                                            MachineInstr *Compare) {
  if (MF->getTarget().getOptLevel() == CodeGenOpt::None)
    return false;

  // Check whether this is a comparison against zero.
  if (Compare->getNumExplicitOperands() != 2 ||
      !Compare->getOperand(1).isImm() ||
      Compare->getOperand(1).getImm() != 0)
    return false;

  // See which compare-style condition codes are available after PrevCCSetter.
  unsigned PrevFlags = PrevCCSetter->getDesc().TSFlags;
  unsigned ReusableCCMask = 0;
  if (PrevFlags & SystemZII::CCHasZero)
    ReusableCCMask |= SystemZ::CCMASK_CMP_EQ;

  // For unsigned comparisons with zero, only equality makes sense.
  unsigned CompareFlags = Compare->getDesc().TSFlags;
  if (!(CompareFlags & SystemZII::IsLogical) &&
      (PrevFlags & SystemZII::CCHasOrder))
    ReusableCCMask |= SystemZ::CCMASK_CMP_LT | SystemZ::CCMASK_CMP_GT;

  if (ReusableCCMask == 0)
    return false;

  // Make sure that PrevCCSetter sets the value being compared.
  unsigned SrcReg = Compare->getOperand(0).getReg();
  unsigned SrcSubReg = Compare->getOperand(0).getSubReg();
  if (!PrevCCSetter->getOperand(0).isReg() ||
      !PrevCCSetter->getOperand(0).isDef() ||
      PrevCCSetter->getOperand(0).getReg() != SrcReg ||
      PrevCCSetter->getOperand(0).getSubReg() != SrcSubReg)
    return false;

  // Make sure that SrcReg survives until Compare.
  MachineBasicBlock::iterator MBBI = PrevCCSetter, MBBE = Compare;
  const TargetRegisterInfo *TRI = &TII->getRegisterInfo();
  for (++MBBI; MBBI != MBBE; ++MBBI)
    if (MBBI->modifiesRegister(SrcReg, TRI))
      return false;

  // See whether all uses of Compare's CC value could make do with
  // the values produced by PrevCCSetter.
  SmallVector<MachineOperand *, 4> AlterMasks;
  if (!canRestrictCCMask(Compare, ReusableCCMask, AlterMasks, TRI))
    return false;

  // Alter the CC masks that canRestrictCCMask says need to be altered.
  unsigned CCValues = SystemZII::getCCValues(PrevFlags);
  assert((ReusableCCMask & ~CCValues) == 0 && "Invalid CCValues");
  for (unsigned I = 0, E = AlterMasks.size(); I != E; I += 2) {
    AlterMasks[I]->setImm(CCValues);
    unsigned CCMask = AlterMasks[I + 1]->getImm();
    if (CCMask & ~ReusableCCMask)
      AlterMasks[I + 1]->setImm((CCMask & ReusableCCMask) |
                                (CCValues & ~ReusableCCMask));
  }

  // CC is now live after PrevCCSetter.
  int CCDef = PrevCCSetter->findRegisterDefOperandIdx(SystemZ::CC, false,
                                                      true, TRI);
  assert(CCDef >= 0 && "Couldn't find CC set");
  PrevCCSetter->getOperand(CCDef).setIsDead(false);

  // Clear any intervening kills of CC.
  MBBI = PrevCCSetter;
  for (++MBBI; MBBI != MBBE; ++MBBI)
    MBBI->clearRegisterKills(SystemZ::CC, TRI);

  return true;
}

// Try to fuse compare instruction Compare into a later branch.  Return
// true on success and if Compare is therefore redundant.
bool SystemZLongBranch::fuseCompareAndBranch(MachineInstr *Compare) {
  if (MF->getTarget().getOptLevel() == CodeGenOpt::None)
    return false;

  unsigned FusedOpcode = TII->getCompareAndBranch(Compare->getOpcode(),
                                                  Compare);
  if (!FusedOpcode)
    return false;

  unsigned SrcReg = Compare->getOperand(0).getReg();
  unsigned SrcReg2 = (Compare->getOperand(1).isReg() ?
                      Compare->getOperand(1).getReg() : 0);
  const TargetRegisterInfo *TRI = &TII->getRegisterInfo();
  MachineBasicBlock *MBB = Compare->getParent();
  MachineBasicBlock::iterator MBBI = Compare, MBBE = MBB->end();
  for (++MBBI; MBBI != MBBE; ++MBBI) {
    if (MBBI->getOpcode() == SystemZ::BRC && !isCCLiveAfter(MBBI, TRI)) {
      // Read the branch mask and target.
      MachineOperand CCMask(MBBI->getOperand(1));
      MachineOperand Target(MBBI->getOperand(2));
      assert((CCMask.getImm() & ~SystemZ::CCMASK_ICMP) == 0 &&
             "Invalid condition-code mask for integer comparison");

      // Clear out all current operands.
      int CCUse = MBBI->findRegisterUseOperandIdx(SystemZ::CC, false, TRI);
      assert(CCUse >= 0 && "BRC must use CC");
      MBBI->RemoveOperand(CCUse);
      MBBI->RemoveOperand(2);
      MBBI->RemoveOperand(1);
      MBBI->RemoveOperand(0);

      // Rebuild MBBI as a fused compare and branch.
      MBBI->setDesc(TII->get(FusedOpcode));
      MachineInstrBuilder(*MBB->getParent(), MBBI)
        .addOperand(Compare->getOperand(0))
        .addOperand(Compare->getOperand(1))
        .addOperand(CCMask)
        .addOperand(Target);

      // Clear any intervening kills of SrcReg and SrcReg2.
      MBBI = Compare;
      for (++MBBI; MBBI != MBBE; ++MBBI) {
        MBBI->clearRegisterKills(SrcReg, TRI);
        if (SrcReg2)
          MBBI->clearRegisterKills(SrcReg2, TRI);
      }
      return true;
    }

    // Stop if we find another reference to CC before a branch.
    if (MBBI->readsRegister(SystemZ::CC, TRI) ||
        MBBI->modifiesRegister(SystemZ::CC, TRI))
      return false;

    // Stop if we find another assignment to the registers before the branch.
    if (MBBI->modifiesRegister(SrcReg, TRI) ||
        (SrcReg2 && MBBI->modifiesRegister(SrcReg2, TRI)))
      return false;
  }
  return false;
}

// Fill MBBs and Terminators, setting the addresses on the assumption
// that no branches need relaxation.  Return the size of the function under
// this assumption.
uint64_t SystemZLongBranch::initMBBInfo() {
  const TargetRegisterInfo *TRI = &TII->getRegisterInfo();

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
    MachineInstr *PrevCCSetter = 0;
    while (MI != End && !MI->isTerminator()) {
      MachineInstr *Current = MI;
      ++MI;
      if (Current->isCompare()) {
        if ((PrevCCSetter && optimizeCompareZero(PrevCCSetter, Current)) ||
            fuseCompareAndBranch(Current)) {
          Current->removeFromParent();
          continue;
        }
      }
      if (Current->modifiesRegister(SystemZ::CC, TRI))
        PrevCCSetter = Current;
      Block.Size += TII->getInstSizeInBytes(Current);
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

// Return true if, under current assumptions, Terminator would need to be
// relaxed if it were placed at address Address.
bool SystemZLongBranch::mustRelaxBranch(const TerminatorInfo &Terminator,
                                        uint64_t Address) {
  if (!Terminator.Branch)
    return false;

  const MBBInfo &Target = MBBs[Terminator.TargetBlock];
  if (Address >= Target.Address) {
    if (Address - Target.Address <= MaxBackwardRange)
      return false;
  } else {
    if (Target.Address - Address <= MaxForwardRange)
      return false;
  }

  return true;
}

// Return true if, under current assumptions, any terminator needs
// to be relaxed.
bool SystemZLongBranch::mustRelaxABranch() {
  for (SmallVectorImpl<TerminatorInfo>::iterator TI = Terminators.begin(),
         TE = Terminators.end(); TI != TE; ++TI)
    if (mustRelaxBranch(*TI, TI->Address))
      return true;
  return false;
}

// Set the address of each block on the assumption that all branches
// must be long.
void SystemZLongBranch::setWorstCaseAddresses() {
  SmallVector<TerminatorInfo, 16>::iterator TI = Terminators.begin();
  BlockPosition Position(MF->getAlignment());
  for (SmallVectorImpl<MBBInfo>::iterator BI = MBBs.begin(), BE = MBBs.end();
       BI != BE; ++BI) {
    skipNonTerminators(Position, *BI);
    for (unsigned BTI = 0, BTE = BI->NumTerminators; BTI != BTE; ++BTI) {
      skipTerminator(Position, *TI, true);
      ++TI;
    }
  }
}

// Split MI into the comparison given by CompareOpcode followed
// a BRCL on the result.
void SystemZLongBranch::splitCompareBranch(MachineInstr *MI,
                                           unsigned CompareOpcode) {
  MachineBasicBlock *MBB = MI->getParent();
  DebugLoc DL = MI->getDebugLoc();
  BuildMI(*MBB, MI, DL, TII->get(CompareOpcode))
    .addOperand(MI->getOperand(0))
    .addOperand(MI->getOperand(1));
  MachineInstr *BRCL = BuildMI(*MBB, MI, DL, TII->get(SystemZ::BRCL))
    .addImm(SystemZ::CCMASK_ICMP)
    .addOperand(MI->getOperand(2))
    .addOperand(MI->getOperand(3));
  // The implicit use of CC is a killing use.
  BRCL->addRegisterKilled(SystemZ::CC, &TII->getRegisterInfo());
  MI->eraseFromParent();
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
  case SystemZ::CRJ:
    splitCompareBranch(Branch, SystemZ::CR);
    break;
  case SystemZ::CGRJ:
    splitCompareBranch(Branch, SystemZ::CGR);
    break;
  case SystemZ::CIJ:
    splitCompareBranch(Branch, SystemZ::CHI);
    break;
  case SystemZ::CGIJ:
    splitCompareBranch(Branch, SystemZ::CGHI);
    break;
  default:
    llvm_unreachable("Unrecognized branch");
  }

  Terminator.Size += Terminator.ExtraRelaxSize;
  Terminator.ExtraRelaxSize = 0;
  Terminator.Branch = 0;

  ++LongBranches;
}

// Run a shortening pass and relax any branches that need to be relaxed.
void SystemZLongBranch::relaxBranches() {
  SmallVector<TerminatorInfo, 16>::iterator TI = Terminators.begin();
  BlockPosition Position(MF->getAlignment());
  for (SmallVectorImpl<MBBInfo>::iterator BI = MBBs.begin(), BE = MBBs.end();
       BI != BE; ++BI) {
    skipNonTerminators(Position, *BI);
    for (unsigned BTI = 0, BTE = BI->NumTerminators; BTI != BTE; ++BTI) {
      assert(Position.Address <= TI->Address &&
             "Addresses shouldn't go forwards");
      if (mustRelaxBranch(*TI, Position.Address))
        relaxBranch(*TI);
      skipTerminator(Position, *TI, false);
      ++TI;
    }
  }
}

bool SystemZLongBranch::runOnMachineFunction(MachineFunction &F) {
  TII = static_cast<const SystemZInstrInfo *>(F.getTarget().getInstrInfo());
  MF = &F;
  uint64_t Size = initMBBInfo();
  if (Size <= MaxForwardRange || !mustRelaxABranch())
    return false;

  setWorstCaseAddresses();
  relaxBranches();
  return true;
}
