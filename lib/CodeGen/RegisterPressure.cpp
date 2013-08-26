//===-- RegisterPressure.cpp - Dynamic Register Pressure ------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements the RegisterPressure class which can be used to track
// MachineInstr level register pressure.
//
//===----------------------------------------------------------------------===//

#include "llvm/CodeGen/RegisterPressure.h"
#include "llvm/CodeGen/LiveInterval.h"
#include "llvm/CodeGen/LiveIntervalAnalysis.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/RegisterClassInfo.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetMachine.h"

using namespace llvm;

/// Increase pressure for each pressure set provided by TargetRegisterInfo.
static void increaseSetPressure(std::vector<unsigned> &CurrSetPressure,
                                PSetIterator PSetI) {
  unsigned Weight = PSetI.getWeight();
  for (; PSetI.isValid(); ++PSetI)
    CurrSetPressure[*PSetI] += Weight;
}

/// Decrease pressure for each pressure set provided by TargetRegisterInfo.
static void decreaseSetPressure(std::vector<unsigned> &CurrSetPressure,
                                PSetIterator PSetI) {
  unsigned Weight = PSetI.getWeight();
  for (; PSetI.isValid(); ++PSetI) {
    assert(CurrSetPressure[*PSetI] >= Weight && "register pressure underflow");
    CurrSetPressure[*PSetI] -= Weight;
  }
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
void llvm::dumpRegSetPressure(ArrayRef<unsigned> SetPressure,
                              const TargetRegisterInfo *TRI) {
  bool Empty = true;
  for (unsigned i = 0, e = SetPressure.size(); i < e; ++i) {
    if (SetPressure[i] != 0) {
      dbgs() << TRI->getRegPressureSetName(i) << "=" << SetPressure[i] << '\n';
      Empty = false;
    }
  }
  if (Empty)
    dbgs() << "\n";
}

void RegisterPressure::dump(const TargetRegisterInfo *TRI) const {
  dbgs() << "Max Pressure: ";
  dumpRegSetPressure(MaxSetPressure, TRI);
  dbgs() << "Live In: ";
  for (unsigned i = 0, e = LiveInRegs.size(); i < e; ++i)
    dbgs() << PrintReg(LiveInRegs[i], TRI) << " ";
  dbgs() << '\n';
  dbgs() << "Live Out: ";
  for (unsigned i = 0, e = LiveOutRegs.size(); i < e; ++i)
    dbgs() << PrintReg(LiveOutRegs[i], TRI) << " ";
  dbgs() << '\n';
}

void RegPressureTracker::dump() const {
  if (!isTopClosed() || !isBottomClosed()) {
    dbgs() << "Curr Pressure: ";
    dumpRegSetPressure(CurrSetPressure, TRI);
  }
  P.dump(TRI);
}
#endif

/// Increase the current pressure as impacted by these registers and bump
/// the high water mark if needed.
void RegPressureTracker::increaseRegPressure(ArrayRef<unsigned> RegUnits) {
  for (unsigned i = 0, e = RegUnits.size(); i != e; ++i) {
    PSetIterator PSetI = MRI->getPressureSets(RegUnits[i]);
    unsigned Weight = PSetI.getWeight();
    for (; PSetI.isValid(); ++PSetI) {
      CurrSetPressure[*PSetI] += Weight;
      if (CurrSetPressure[*PSetI] > P.MaxSetPressure[*PSetI]) {
        P.MaxSetPressure[*PSetI] = CurrSetPressure[*PSetI];
      }
    }
  }
}

/// Simply decrease the current pressure as impacted by these registers.
void RegPressureTracker::decreaseRegPressure(ArrayRef<unsigned> RegUnits) {
  for (unsigned I = 0, E = RegUnits.size(); I != E; ++I)
    decreaseSetPressure(CurrSetPressure, MRI->getPressureSets(RegUnits[I]));
}

/// Clear the result so it can be used for another round of pressure tracking.
void IntervalPressure::reset() {
  TopIdx = BottomIdx = SlotIndex();
  MaxSetPressure.clear();
  LiveInRegs.clear();
  LiveOutRegs.clear();
}

/// Clear the result so it can be used for another round of pressure tracking.
void RegionPressure::reset() {
  TopPos = BottomPos = MachineBasicBlock::const_iterator();
  MaxSetPressure.clear();
  LiveInRegs.clear();
  LiveOutRegs.clear();
}

/// If the current top is not less than or equal to the next index, open it.
/// We happen to need the SlotIndex for the next top for pressure update.
void IntervalPressure::openTop(SlotIndex NextTop) {
  if (TopIdx <= NextTop)
    return;
  TopIdx = SlotIndex();
  LiveInRegs.clear();
}

/// If the current top is the previous instruction (before receding), open it.
void RegionPressure::openTop(MachineBasicBlock::const_iterator PrevTop) {
  if (TopPos != PrevTop)
    return;
  TopPos = MachineBasicBlock::const_iterator();
  LiveInRegs.clear();
}

/// If the current bottom is not greater than the previous index, open it.
void IntervalPressure::openBottom(SlotIndex PrevBottom) {
  if (BottomIdx > PrevBottom)
    return;
  BottomIdx = SlotIndex();
  LiveInRegs.clear();
}

/// If the current bottom is the previous instr (before advancing), open it.
void RegionPressure::openBottom(MachineBasicBlock::const_iterator PrevBottom) {
  if (BottomPos != PrevBottom)
    return;
  BottomPos = MachineBasicBlock::const_iterator();
  LiveInRegs.clear();
}

const LiveInterval *RegPressureTracker::getInterval(unsigned Reg) const {
  if (TargetRegisterInfo::isVirtualRegister(Reg))
    return &LIS->getInterval(Reg);
  return LIS->getCachedRegUnit(Reg);
}

/// Setup the RegPressureTracker.
///
/// TODO: Add support for pressure without LiveIntervals.
void RegPressureTracker::init(const MachineFunction *mf,
                              const RegisterClassInfo *rci,
                              const LiveIntervals *lis,
                              const MachineBasicBlock *mbb,
                              MachineBasicBlock::const_iterator pos,
                              bool ShouldTrackUntiedDefs)
{
  MF = mf;
  TRI = MF->getTarget().getRegisterInfo();
  RCI = rci;
  MRI = &MF->getRegInfo();
  MBB = mbb;
  TrackUntiedDefs = ShouldTrackUntiedDefs;

  if (RequireIntervals) {
    assert(lis && "IntervalPressure requires LiveIntervals");
    LIS = lis;
  }

  CurrPos = pos;
  CurrSetPressure.assign(TRI->getNumRegPressureSets(), 0);
  LiveThruPressure.clear();

  if (RequireIntervals)
    static_cast<IntervalPressure&>(P).reset();
  else
    static_cast<RegionPressure&>(P).reset();
  P.MaxSetPressure = CurrSetPressure;

  LiveRegs.PhysRegs.clear();
  LiveRegs.PhysRegs.setUniverse(TRI->getNumRegs());
  LiveRegs.VirtRegs.clear();
  LiveRegs.VirtRegs.setUniverse(MRI->getNumVirtRegs());
  UntiedDefs.clear();
  if (TrackUntiedDefs)
    UntiedDefs.setUniverse(MRI->getNumVirtRegs());
}

/// Does this pressure result have a valid top position and live ins.
bool RegPressureTracker::isTopClosed() const {
  if (RequireIntervals)
    return static_cast<IntervalPressure&>(P).TopIdx.isValid();
  return (static_cast<RegionPressure&>(P).TopPos ==
          MachineBasicBlock::const_iterator());
}

/// Does this pressure result have a valid bottom position and live outs.
bool RegPressureTracker::isBottomClosed() const {
  if (RequireIntervals)
    return static_cast<IntervalPressure&>(P).BottomIdx.isValid();
  return (static_cast<RegionPressure&>(P).BottomPos ==
          MachineBasicBlock::const_iterator());
}


SlotIndex RegPressureTracker::getCurrSlot() const {
  MachineBasicBlock::const_iterator IdxPos = CurrPos;
  while (IdxPos != MBB->end() && IdxPos->isDebugValue())
    ++IdxPos;
  if (IdxPos == MBB->end())
    return LIS->getMBBEndIdx(MBB);
  return LIS->getInstructionIndex(IdxPos).getRegSlot();
}

/// Set the boundary for the top of the region and summarize live ins.
void RegPressureTracker::closeTop() {
  if (RequireIntervals)
    static_cast<IntervalPressure&>(P).TopIdx = getCurrSlot();
  else
    static_cast<RegionPressure&>(P).TopPos = CurrPos;

  assert(P.LiveInRegs.empty() && "inconsistent max pressure result");
  P.LiveInRegs.reserve(LiveRegs.PhysRegs.size() + LiveRegs.VirtRegs.size());
  P.LiveInRegs.append(LiveRegs.PhysRegs.begin(), LiveRegs.PhysRegs.end());
  for (SparseSet<unsigned>::const_iterator I =
         LiveRegs.VirtRegs.begin(), E = LiveRegs.VirtRegs.end(); I != E; ++I)
    P.LiveInRegs.push_back(*I);
  std::sort(P.LiveInRegs.begin(), P.LiveInRegs.end());
  P.LiveInRegs.erase(std::unique(P.LiveInRegs.begin(), P.LiveInRegs.end()),
                     P.LiveInRegs.end());
}

/// Set the boundary for the bottom of the region and summarize live outs.
void RegPressureTracker::closeBottom() {
  if (RequireIntervals)
    static_cast<IntervalPressure&>(P).BottomIdx = getCurrSlot();
  else
    static_cast<RegionPressure&>(P).BottomPos = CurrPos;

  assert(P.LiveOutRegs.empty() && "inconsistent max pressure result");
  P.LiveOutRegs.reserve(LiveRegs.PhysRegs.size() + LiveRegs.VirtRegs.size());
  P.LiveOutRegs.append(LiveRegs.PhysRegs.begin(), LiveRegs.PhysRegs.end());
  for (SparseSet<unsigned>::const_iterator I =
         LiveRegs.VirtRegs.begin(), E = LiveRegs.VirtRegs.end(); I != E; ++I)
    P.LiveOutRegs.push_back(*I);
  std::sort(P.LiveOutRegs.begin(), P.LiveOutRegs.end());
  P.LiveOutRegs.erase(std::unique(P.LiveOutRegs.begin(), P.LiveOutRegs.end()),
                      P.LiveOutRegs.end());
}

/// Finalize the region boundaries and record live ins and live outs.
void RegPressureTracker::closeRegion() {
  if (!isTopClosed() && !isBottomClosed()) {
    assert(LiveRegs.PhysRegs.empty() && LiveRegs.VirtRegs.empty() &&
           "no region boundary");
    return;
  }
  if (!isBottomClosed())
    closeBottom();
  else if (!isTopClosed())
    closeTop();
  // If both top and bottom are closed, do nothing.
}

/// The register tracker is unaware of global liveness so ignores normal
/// live-thru ranges. However, two-address or coalesced chains can also lead
/// to live ranges with no holes. Count these to inform heuristics that we
/// can never drop below this pressure.
void RegPressureTracker::initLiveThru(const RegPressureTracker &RPTracker) {
  LiveThruPressure.assign(TRI->getNumRegPressureSets(), 0);
  assert(isBottomClosed() && "need bottom-up tracking to intialize.");
  for (unsigned i = 0, e = P.LiveOutRegs.size(); i < e; ++i) {
    unsigned Reg = P.LiveOutRegs[i];
    if (TargetRegisterInfo::isVirtualRegister(Reg)
        && !RPTracker.hasUntiedDef(Reg)) {
      increaseSetPressure(LiveThruPressure, MRI->getPressureSets(Reg));
    }
  }
}

/// \brief Convenient wrapper for checking membership in RegisterOperands.
/// (std::count() doesn't have an early exit).
static bool containsReg(ArrayRef<unsigned> RegUnits, unsigned RegUnit) {
  return std::find(RegUnits.begin(), RegUnits.end(), RegUnit) != RegUnits.end();
}

/// Collect this instruction's unique uses and defs into SmallVectors for
/// processing defs and uses in order.
class RegisterOperands {
  const TargetRegisterInfo *TRI;
  const MachineRegisterInfo *MRI;

public:
  SmallVector<unsigned, 8> Uses;
  SmallVector<unsigned, 8> Defs;
  SmallVector<unsigned, 8> DeadDefs;

  RegisterOperands(const TargetRegisterInfo *tri,
                   const MachineRegisterInfo *mri): TRI(tri), MRI(mri) {}

  /// Push this operand's register onto the correct vector.
  void collect(const MachineOperand &MO) {
    if (!MO.isReg() || !MO.getReg())
      return;
    if (MO.readsReg())
      pushRegUnits(MO.getReg(), Uses);
    if (MO.isDef()) {
      if (MO.isDead())
        pushRegUnits(MO.getReg(), DeadDefs);
      else
        pushRegUnits(MO.getReg(), Defs);
    }
  }

protected:
  void pushRegUnits(unsigned Reg, SmallVectorImpl<unsigned> &RegUnits) {
    if (TargetRegisterInfo::isVirtualRegister(Reg)) {
      if (containsReg(RegUnits, Reg))
        return;
      RegUnits.push_back(Reg);
    }
    else if (MRI->isAllocatable(Reg)) {
      for (MCRegUnitIterator Units(Reg, TRI); Units.isValid(); ++Units) {
        if (containsReg(RegUnits, *Units))
          continue;
        RegUnits.push_back(*Units);
      }
    }
  }
};

/// Collect physical and virtual register operands.
static void collectOperands(const MachineInstr *MI,
                            RegisterOperands &RegOpers) {
  for (ConstMIBundleOperands OperI(MI); OperI.isValid(); ++OperI)
    RegOpers.collect(*OperI);

  // Remove redundant physreg dead defs.
  SmallVectorImpl<unsigned>::iterator I =
    std::remove_if(RegOpers.DeadDefs.begin(), RegOpers.DeadDefs.end(),
                   std::bind1st(std::ptr_fun(containsReg), RegOpers.Defs));
  RegOpers.DeadDefs.erase(I, RegOpers.DeadDefs.end());
}

/// Force liveness of registers.
void RegPressureTracker::addLiveRegs(ArrayRef<unsigned> Regs) {
  for (unsigned i = 0, e = Regs.size(); i != e; ++i) {
    if (LiveRegs.insert(Regs[i]))
      increaseRegPressure(Regs[i]);
  }
}

/// Add Reg to the live in set and increase max pressure.
void RegPressureTracker::discoverLiveIn(unsigned Reg) {
  assert(!LiveRegs.contains(Reg) && "avoid bumping max pressure twice");
  if (containsReg(P.LiveInRegs, Reg))
    return;

  // At live in discovery, unconditionally increase the high water mark.
  P.LiveInRegs.push_back(Reg);
  increaseSetPressure(P.MaxSetPressure, MRI->getPressureSets(Reg));
}

/// Add Reg to the live out set and increase max pressure.
void RegPressureTracker::discoverLiveOut(unsigned Reg) {
  assert(!LiveRegs.contains(Reg) && "avoid bumping max pressure twice");
  if (containsReg(P.LiveOutRegs, Reg))
    return;

  // At live out discovery, unconditionally increase the high water mark.
  P.LiveOutRegs.push_back(Reg);
  increaseSetPressure(P.MaxSetPressure, MRI->getPressureSets(Reg));
}

/// Recede across the previous instruction.
bool RegPressureTracker::recede() {
  // Check for the top of the analyzable region.
  if (CurrPos == MBB->begin()) {
    closeRegion();
    return false;
  }
  if (!isBottomClosed())
    closeBottom();

  // Open the top of the region using block iterators.
  if (!RequireIntervals && isTopClosed())
    static_cast<RegionPressure&>(P).openTop(CurrPos);

  // Find the previous instruction.
  do
    --CurrPos;
  while (CurrPos != MBB->begin() && CurrPos->isDebugValue());

  if (CurrPos->isDebugValue()) {
    closeRegion();
    return false;
  }
  SlotIndex SlotIdx;
  if (RequireIntervals)
    SlotIdx = LIS->getInstructionIndex(CurrPos).getRegSlot();

  // Open the top of the region using slot indexes.
  if (RequireIntervals && isTopClosed())
    static_cast<IntervalPressure&>(P).openTop(SlotIdx);

  RegisterOperands RegOpers(TRI, MRI);
  collectOperands(CurrPos, RegOpers);

  // Boost pressure for all dead defs together.
  increaseRegPressure(RegOpers.DeadDefs);
  decreaseRegPressure(RegOpers.DeadDefs);

  // Kill liveness at live defs.
  // TODO: consider earlyclobbers?
  for (unsigned i = 0, e = RegOpers.Defs.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Defs[i];
    if (LiveRegs.erase(Reg))
      decreaseRegPressure(Reg);
    else
      discoverLiveOut(Reg);
  }

  // Generate liveness for uses.
  for (unsigned i = 0, e = RegOpers.Uses.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Uses[i];
    if (!LiveRegs.contains(Reg)) {
      // Adjust liveouts if LiveIntervals are available.
      if (RequireIntervals) {
        const LiveInterval *LI = getInterval(Reg);
        if (LI && !LI->killedAt(SlotIdx))
          discoverLiveOut(Reg);
      }
      increaseRegPressure(Reg);
      LiveRegs.insert(Reg);
    }
  }
  if (TrackUntiedDefs) {
    for (unsigned i = 0, e = RegOpers.Defs.size(); i < e; ++i) {
      unsigned Reg = RegOpers.Defs[i];
      if (TargetRegisterInfo::isVirtualRegister(Reg) && !LiveRegs.contains(Reg))
        UntiedDefs.insert(Reg);
    }
  }
  return true;
}

/// Advance across the current instruction.
bool RegPressureTracker::advance() {
  assert(!TrackUntiedDefs && "unsupported mode");

  // Check for the bottom of the analyzable region.
  if (CurrPos == MBB->end()) {
    closeRegion();
    return false;
  }
  if (!isTopClosed())
    closeTop();

  SlotIndex SlotIdx;
  if (RequireIntervals)
    SlotIdx = getCurrSlot();

  // Open the bottom of the region using slot indexes.
  if (isBottomClosed()) {
    if (RequireIntervals)
      static_cast<IntervalPressure&>(P).openBottom(SlotIdx);
    else
      static_cast<RegionPressure&>(P).openBottom(CurrPos);
  }

  RegisterOperands RegOpers(TRI, MRI);
  collectOperands(CurrPos, RegOpers);

  for (unsigned i = 0, e = RegOpers.Uses.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Uses[i];
    // Discover live-ins.
    bool isLive = LiveRegs.contains(Reg);
    if (!isLive)
      discoverLiveIn(Reg);
    // Kill liveness at last uses.
    bool lastUse = false;
    if (RequireIntervals) {
      const LiveInterval *LI = getInterval(Reg);
      lastUse = LI && LI->killedAt(SlotIdx);
    }
    else {
      // Allocatable physregs are always single-use before register rewriting.
      lastUse = !TargetRegisterInfo::isVirtualRegister(Reg);
    }
    if (lastUse && isLive) {
      LiveRegs.erase(Reg);
      decreaseRegPressure(Reg);
    }
    else if (!lastUse && !isLive)
      increaseRegPressure(Reg);
  }

  // Generate liveness for defs.
  for (unsigned i = 0, e = RegOpers.Defs.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Defs[i];
    if (LiveRegs.insert(Reg))
      increaseRegPressure(Reg);
  }

  // Boost pressure for all dead defs together.
  increaseRegPressure(RegOpers.DeadDefs);
  decreaseRegPressure(RegOpers.DeadDefs);

  // Find the next instruction.
  do
    ++CurrPos;
  while (CurrPos != MBB->end() && CurrPos->isDebugValue());
  return true;
}

/// Find the max change in excess pressure across all sets.
static void computeExcessPressureDelta(ArrayRef<unsigned> OldPressureVec,
                                       ArrayRef<unsigned> NewPressureVec,
                                       RegPressureDelta &Delta,
                                       const RegisterClassInfo *RCI,
                                       ArrayRef<unsigned> LiveThruPressureVec) {
  int ExcessUnits = 0;
  unsigned PSetID = ~0U;
  for (unsigned i = 0, e = OldPressureVec.size(); i < e; ++i) {
    unsigned POld = OldPressureVec[i];
    unsigned PNew = NewPressureVec[i];
    int PDiff = (int)PNew - (int)POld;
    if (!PDiff) // No change in this set in the common case.
      continue;
    // Only consider change beyond the limit.
    unsigned Limit = RCI->getRegPressureSetLimit(i);
    if (!LiveThruPressureVec.empty())
      Limit += LiveThruPressureVec[i];

    if (Limit > POld) {
      if (Limit > PNew)
        PDiff = 0;            // Under the limit
      else
        PDiff = PNew - Limit; // Just exceeded limit.
    }
    else if (Limit > PNew)
      PDiff = Limit - POld;   // Just obeyed limit.

    if (PDiff) {
      ExcessUnits = PDiff;
      PSetID = i;
      break;
    }
  }
  Delta.Excess.PSetID = PSetID;
  Delta.Excess.UnitIncrease = ExcessUnits;
}

/// Find the max change in max pressure that either surpasses a critical PSet
/// limit or exceeds the current MaxPressureLimit.
///
/// FIXME: comparing each element of the old and new MaxPressure vectors here is
/// silly. It's done now to demonstrate the concept but will go away with a
/// RegPressureTracker API change to work with pressure differences.
static void computeMaxPressureDelta(ArrayRef<unsigned> OldMaxPressureVec,
                                    ArrayRef<unsigned> NewMaxPressureVec,
                                    ArrayRef<PressureElement> CriticalPSets,
                                    ArrayRef<unsigned> MaxPressureLimit,
                                    RegPressureDelta &Delta) {
  Delta.CriticalMax = PressureElement();
  Delta.CurrentMax = PressureElement();

  unsigned CritIdx = 0, CritEnd = CriticalPSets.size();
  for (unsigned i = 0, e = OldMaxPressureVec.size(); i < e; ++i) {
    unsigned POld = OldMaxPressureVec[i];
    unsigned PNew = NewMaxPressureVec[i];
    if (PNew == POld) // No change in this set in the common case.
      continue;

    if (!Delta.CriticalMax.isValid()) {
      while (CritIdx != CritEnd && CriticalPSets[CritIdx].PSetID < i)
        ++CritIdx;

      if (CritIdx != CritEnd && CriticalPSets[CritIdx].PSetID == i) {
        int PDiff = (int)PNew - (int)CriticalPSets[CritIdx].UnitIncrease;
        if (PDiff > 0) {
          Delta.CriticalMax.PSetID = i;
          Delta.CriticalMax.UnitIncrease = PDiff;
        }
      }
    }
    // Find the first increase above MaxPressureLimit.
    // (Ignores negative MDiff).
    if (!Delta.CurrentMax.isValid()) {
      int MDiff = (int)PNew - (int)MaxPressureLimit[i];
      if (MDiff > 0) {
        Delta.CurrentMax.PSetID = i;
        Delta.CurrentMax.UnitIncrease = MDiff;
        if (CritIdx == CritEnd || Delta.CriticalMax.isValid())
          break;
      }
    }
  }
}

/// Record the upward impact of a single instruction on current register
/// pressure. Unlike the advance/recede pressure tracking interface, this does
/// not discover live in/outs.
///
/// This is intended for speculative queries. It leaves pressure inconsistent
/// with the current position, so must be restored by the caller.
void RegPressureTracker::bumpUpwardPressure(const MachineInstr *MI) {
  assert(!MI->isDebugValue() && "Expect a nondebug instruction.");

  // Account for register pressure similar to RegPressureTracker::recede().
  RegisterOperands RegOpers(TRI, MRI);
  collectOperands(MI, RegOpers);

  // Boost max pressure for all dead defs together.
  // Since CurrSetPressure and MaxSetPressure
  increaseRegPressure(RegOpers.DeadDefs);
  decreaseRegPressure(RegOpers.DeadDefs);

  // Kill liveness at live defs.
  for (unsigned i = 0, e = RegOpers.Defs.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Defs[i];
    if (!containsReg(RegOpers.Uses, Reg))
      decreaseRegPressure(Reg);
  }
  // Generate liveness for uses.
  for (unsigned i = 0, e = RegOpers.Uses.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Uses[i];
    if (!LiveRegs.contains(Reg))
      increaseRegPressure(Reg);
  }
}

/// Consider the pressure increase caused by traversing this instruction
/// bottom-up. Find the pressure set with the most change beyond its pressure
/// limit based on the tracker's current pressure, and return the change in
/// number of register units of that pressure set introduced by this
/// instruction.
///
/// This assumes that the current LiveOut set is sufficient.
///
/// FIXME: This is expensive for an on-the-fly query. We need to cache the
/// result per-SUnit with enough information to adjust for the current
/// scheduling position. But this works as a proof of concept.
void RegPressureTracker::
getMaxUpwardPressureDelta(const MachineInstr *MI, RegPressureDelta &Delta,
                          ArrayRef<PressureElement> CriticalPSets,
                          ArrayRef<unsigned> MaxPressureLimit) {
  // Snapshot Pressure.
  // FIXME: The snapshot heap space should persist. But I'm planning to
  // summarize the pressure effect so we don't need to snapshot at all.
  std::vector<unsigned> SavedPressure = CurrSetPressure;
  std::vector<unsigned> SavedMaxPressure = P.MaxSetPressure;

  bumpUpwardPressure(MI);

  computeExcessPressureDelta(SavedPressure, CurrSetPressure, Delta, RCI,
                             LiveThruPressure);
  computeMaxPressureDelta(SavedMaxPressure, P.MaxSetPressure, CriticalPSets,
                          MaxPressureLimit, Delta);
  assert(Delta.CriticalMax.UnitIncrease >= 0 &&
         Delta.CurrentMax.UnitIncrease >= 0 && "cannot decrease max pressure");

  // Restore the tracker's state.
  P.MaxSetPressure.swap(SavedMaxPressure);
  CurrSetPressure.swap(SavedPressure);
}

/// Helper to find a vreg use between two indices [PriorUseIdx, NextUseIdx).
static bool findUseBetween(unsigned Reg,
                           SlotIndex PriorUseIdx, SlotIndex NextUseIdx,
                           const MachineRegisterInfo *MRI,
                           const LiveIntervals *LIS) {
  for (MachineRegisterInfo::use_nodbg_iterator
         UI = MRI->use_nodbg_begin(Reg), UE = MRI->use_nodbg_end();
         UI != UE; UI.skipInstruction()) {
      const MachineInstr* MI = &*UI;
      if (MI->isDebugValue())
        continue;
      SlotIndex InstSlot = LIS->getInstructionIndex(MI).getRegSlot();
      if (InstSlot >= PriorUseIdx && InstSlot < NextUseIdx)
        return true;
  }
  return false;
}

/// Record the downward impact of a single instruction on current register
/// pressure. Unlike the advance/recede pressure tracking interface, this does
/// not discover live in/outs.
///
/// This is intended for speculative queries. It leaves pressure inconsistent
/// with the current position, so must be restored by the caller.
void RegPressureTracker::bumpDownwardPressure(const MachineInstr *MI) {
  assert(!MI->isDebugValue() && "Expect a nondebug instruction.");

  // Account for register pressure similar to RegPressureTracker::recede().
  RegisterOperands RegOpers(TRI, MRI);
  collectOperands(MI, RegOpers);

  // Kill liveness at last uses. Assume allocatable physregs are single-use
  // rather than checking LiveIntervals.
  SlotIndex SlotIdx;
  if (RequireIntervals)
    SlotIdx = LIS->getInstructionIndex(MI).getRegSlot();

  for (unsigned i = 0, e = RegOpers.Uses.size(); i < e; ++i) {
    unsigned Reg = RegOpers.Uses[i];
    if (RequireIntervals) {
      // FIXME: allow the caller to pass in the list of vreg uses that remain
      // to be bottom-scheduled to avoid searching uses at each query.
      SlotIndex CurrIdx = getCurrSlot();
      const LiveInterval *LI = getInterval(Reg);
      if (LI && LI->killedAt(SlotIdx)
          && !findUseBetween(Reg, CurrIdx, SlotIdx, MRI, LIS)) {
        decreaseRegPressure(Reg);
      }
    }
    else if (!TargetRegisterInfo::isVirtualRegister(Reg)) {
      // Allocatable physregs are always single-use before register rewriting.
      decreaseRegPressure(Reg);
    }
  }

  // Generate liveness for defs.
  increaseRegPressure(RegOpers.Defs);

  // Boost pressure for all dead defs together.
  increaseRegPressure(RegOpers.DeadDefs);
  decreaseRegPressure(RegOpers.DeadDefs);
}

/// Consider the pressure increase caused by traversing this instruction
/// top-down. Find the register class with the most change in its pressure limit
/// based on the tracker's current pressure, and return the number of excess
/// register units of that pressure set introduced by this instruction.
///
/// This assumes that the current LiveIn set is sufficient.
void RegPressureTracker::
getMaxDownwardPressureDelta(const MachineInstr *MI, RegPressureDelta &Delta,
                            ArrayRef<PressureElement> CriticalPSets,
                            ArrayRef<unsigned> MaxPressureLimit) {
  // Snapshot Pressure.
  std::vector<unsigned> SavedPressure = CurrSetPressure;
  std::vector<unsigned> SavedMaxPressure = P.MaxSetPressure;

  bumpDownwardPressure(MI);

  computeExcessPressureDelta(SavedPressure, CurrSetPressure, Delta, RCI,
                             LiveThruPressure);
  computeMaxPressureDelta(SavedMaxPressure, P.MaxSetPressure, CriticalPSets,
                          MaxPressureLimit, Delta);
  assert(Delta.CriticalMax.UnitIncrease >= 0 &&
         Delta.CurrentMax.UnitIncrease >= 0 && "cannot decrease max pressure");

  // Restore the tracker's state.
  P.MaxSetPressure.swap(SavedMaxPressure);
  CurrSetPressure.swap(SavedPressure);
}

/// Get the pressure of each PSet after traversing this instruction bottom-up.
void RegPressureTracker::
getUpwardPressure(const MachineInstr *MI,
                  std::vector<unsigned> &PressureResult,
                  std::vector<unsigned> &MaxPressureResult) {
  // Snapshot pressure.
  PressureResult = CurrSetPressure;
  MaxPressureResult = P.MaxSetPressure;

  bumpUpwardPressure(MI);

  // Current pressure becomes the result. Restore current pressure.
  P.MaxSetPressure.swap(MaxPressureResult);
  CurrSetPressure.swap(PressureResult);
}

/// Get the pressure of each PSet after traversing this instruction top-down.
void RegPressureTracker::
getDownwardPressure(const MachineInstr *MI,
                    std::vector<unsigned> &PressureResult,
                    std::vector<unsigned> &MaxPressureResult) {
  // Snapshot pressure.
  PressureResult = CurrSetPressure;
  MaxPressureResult = P.MaxSetPressure;

  bumpDownwardPressure(MI);

  // Current pressure becomes the result. Restore current pressure.
  P.MaxSetPressure.swap(MaxPressureResult);
  CurrSetPressure.swap(PressureResult);
}
