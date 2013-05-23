//===-- R600MachineScheduler.cpp - R600 Scheduler Interface -*- C++ -*-----===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief R600 Machine Scheduler interface
// TODO: Scheduling is optimised for VLIW4 arch, modify it to support TRANS slot
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "misched"

#include "R600MachineScheduler.h"
#include "llvm/CodeGen/LiveIntervalAnalysis.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Pass.h"
#include "llvm/PassManager.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

void R600SchedStrategy::initialize(ScheduleDAGMI *dag) {

  DAG = dag;
  TII = static_cast<const R600InstrInfo*>(DAG->TII);
  TRI = static_cast<const R600RegisterInfo*>(DAG->TRI);
  MRI = &DAG->MRI;
  CurInstKind = IDOther;
  CurEmitted = 0;
  OccupedSlotsMask = 15;
  InstKindLimit[IDAlu] = TII->getMaxAlusPerClause();
  InstKindLimit[IDOther] = 32;

  const AMDGPUSubtarget &ST = DAG->TM.getSubtarget<AMDGPUSubtarget>();
  InstKindLimit[IDFetch] = ST.getTexVTXClauseSize();
}

void R600SchedStrategy::MoveUnits(std::vector<SUnit *> &QSrc,
                                  std::vector<SUnit *> &QDst)
{
  QDst.insert(QDst.end(), QSrc.begin(), QSrc.end());
  QSrc.clear();
}

SUnit* R600SchedStrategy::pickNode(bool &IsTopNode) {
  SUnit *SU = 0;
  NextInstKind = IDOther;

  IsTopNode = false;

  // check if we might want to switch current clause type
  bool AllowSwitchToAlu = (CurEmitted >= InstKindLimit[CurInstKind]) ||
      (Available[CurInstKind].empty());
  bool AllowSwitchFromAlu = (CurEmitted >= InstKindLimit[CurInstKind]) &&
      (!Available[IDFetch].empty() || !Available[IDOther].empty());

  if ((AllowSwitchToAlu && CurInstKind != IDAlu) ||
      (!AllowSwitchFromAlu && CurInstKind == IDAlu)) {
    // try to pick ALU
    SU = pickAlu();
    if (SU) {
      if (CurEmitted >= InstKindLimit[IDAlu])
        CurEmitted = 0;
      NextInstKind = IDAlu;
    }
  }

  if (!SU) {
    // try to pick FETCH
    SU = pickOther(IDFetch);
    if (SU)
      NextInstKind = IDFetch;
  }

  // try to pick other
  if (!SU) {
    SU = pickOther(IDOther);
    if (SU)
      NextInstKind = IDOther;
  }

  DEBUG(
      if (SU) {
        dbgs() << " ** Pick node **\n";
        SU->dump(DAG);
      } else {
        dbgs() << "NO NODE \n";
        for (unsigned i = 0; i < DAG->SUnits.size(); i++) {
          const SUnit &S = DAG->SUnits[i];
          if (!S.isScheduled)
            S.dump(DAG);
        }
      }
  );

  return SU;
}

void R600SchedStrategy::schedNode(SUnit *SU, bool IsTopNode) {

  if (NextInstKind != CurInstKind) {
    DEBUG(dbgs() << "Instruction Type Switch\n");
    if (NextInstKind != IDAlu)
      OccupedSlotsMask = 15;
    CurEmitted = 0;
    CurInstKind = NextInstKind;
  }

  if (CurInstKind == IDAlu) {
    switch (getAluKind(SU)) {
    case AluT_XYZW:
      CurEmitted += 4;
      break;
    case AluDiscarded:
      break;
    default: {
      ++CurEmitted;
      for (MachineInstr::mop_iterator It = SU->getInstr()->operands_begin(),
          E = SU->getInstr()->operands_end(); It != E; ++It) {
        MachineOperand &MO = *It;
        if (MO.isReg() && MO.getReg() == AMDGPU::ALU_LITERAL_X)
          ++CurEmitted;
      }
    }
    }
  } else {
    ++CurEmitted;
  }


  DEBUG(dbgs() << CurEmitted << " Instructions Emitted in this clause\n");

  if (CurInstKind != IDFetch) {
    MoveUnits(Pending[IDFetch], Available[IDFetch]);
  }
}

void R600SchedStrategy::releaseTopNode(SUnit *SU) {
  DEBUG(dbgs() << "Top Releasing ";SU->dump(DAG););

}

void R600SchedStrategy::releaseBottomNode(SUnit *SU) {
  DEBUG(dbgs() << "Bottom Releasing ";SU->dump(DAG););

  int IK = getInstKind(SU);
  // There is no export clause, we can schedule one as soon as its ready
  if (IK == IDOther)
    Available[IDOther].push_back(SU);
  else
    Pending[IK].push_back(SU);

}

bool R600SchedStrategy::regBelongsToClass(unsigned Reg,
                                          const TargetRegisterClass *RC) const {
  if (!TargetRegisterInfo::isVirtualRegister(Reg)) {
    return RC->contains(Reg);
  } else {
    return MRI->getRegClass(Reg) == RC;
  }
}

R600SchedStrategy::AluKind R600SchedStrategy::getAluKind(SUnit *SU) const {
  MachineInstr *MI = SU->getInstr();

    switch (MI->getOpcode()) {
    case AMDGPU::PRED_X:
      return AluPredX;
    case AMDGPU::INTERP_PAIR_XY:
    case AMDGPU::INTERP_PAIR_ZW:
    case AMDGPU::INTERP_VEC_LOAD:
    case AMDGPU::DOT_4:
      return AluT_XYZW;
    case AMDGPU::COPY:
      if (MI->getOperand(1).isUndef()) {
        // MI will become a KILL, don't considers it in scheduling
        return AluDiscarded;
      }
    default:
      break;
    }

    // Does the instruction take a whole IG ?
    if(TII->isVector(*MI) ||
        TII->isCubeOp(MI->getOpcode()) ||
        TII->isReductionOp(MI->getOpcode()))
      return AluT_XYZW;

    // Is the result already assigned to a channel ?
    unsigned DestSubReg = MI->getOperand(0).getSubReg();
    switch (DestSubReg) {
    case AMDGPU::sub0:
      return AluT_X;
    case AMDGPU::sub1:
      return AluT_Y;
    case AMDGPU::sub2:
      return AluT_Z;
    case AMDGPU::sub3:
      return AluT_W;
    default:
      break;
    }

    // Is the result already member of a X/Y/Z/W class ?
    unsigned DestReg = MI->getOperand(0).getReg();
    if (regBelongsToClass(DestReg, &AMDGPU::R600_TReg32_XRegClass) ||
        regBelongsToClass(DestReg, &AMDGPU::R600_AddrRegClass))
      return AluT_X;
    if (regBelongsToClass(DestReg, &AMDGPU::R600_TReg32_YRegClass))
      return AluT_Y;
    if (regBelongsToClass(DestReg, &AMDGPU::R600_TReg32_ZRegClass))
      return AluT_Z;
    if (regBelongsToClass(DestReg, &AMDGPU::R600_TReg32_WRegClass))
      return AluT_W;
    if (regBelongsToClass(DestReg, &AMDGPU::R600_Reg128RegClass))
      return AluT_XYZW;

    return AluAny;

}

int R600SchedStrategy::getInstKind(SUnit* SU) {
  int Opcode = SU->getInstr()->getOpcode();

  if (TII->usesTextureCache(Opcode) || TII->usesVertexCache(Opcode))
    return IDFetch;

  if (TII->isALUInstr(Opcode)) {
    return IDAlu;
  }

  switch (Opcode) {
  case AMDGPU::PRED_X:
  case AMDGPU::COPY:
  case AMDGPU::CONST_COPY:
  case AMDGPU::INTERP_PAIR_XY:
  case AMDGPU::INTERP_PAIR_ZW:
  case AMDGPU::INTERP_VEC_LOAD:
  case AMDGPU::DOT_4:
    return IDAlu;
  default:
    return IDOther;
  }
}

SUnit *R600SchedStrategy::PopInst(std::vector<SUnit *> &Q) {
  if (Q.empty())
    return NULL;
  for (std::vector<SUnit *>::reverse_iterator It = Q.rbegin(), E = Q.rend();
      It != E; ++It) {
    SUnit *SU = *It;
    InstructionsGroupCandidate.push_back(SU->getInstr());
    if (TII->canBundle(InstructionsGroupCandidate)) {
      InstructionsGroupCandidate.pop_back();
      Q.erase((It + 1).base());
      return SU;
    } else {
      InstructionsGroupCandidate.pop_back();
    }
  }
  return NULL;
}

void R600SchedStrategy::LoadAlu() {
  std::vector<SUnit *> &QSrc = Pending[IDAlu];
  for (unsigned i = 0, e = QSrc.size(); i < e; ++i) {
    AluKind AK = getAluKind(QSrc[i]);
    AvailableAlus[AK].push_back(QSrc[i]);
  }
  QSrc.clear();
}

void R600SchedStrategy::PrepareNextSlot() {
  DEBUG(dbgs() << "New Slot\n");
  assert (OccupedSlotsMask && "Slot wasn't filled");
  OccupedSlotsMask = 0;
  InstructionsGroupCandidate.clear();
  LoadAlu();
}

void R600SchedStrategy::AssignSlot(MachineInstr* MI, unsigned Slot) {
  unsigned DestReg = MI->getOperand(0).getReg();
  // PressureRegister crashes if an operand is def and used in the same inst
  // and we try to constraint its regclass
  for (MachineInstr::mop_iterator It = MI->operands_begin(),
      E = MI->operands_end(); It != E; ++It) {
    MachineOperand &MO = *It;
    if (MO.isReg() && !MO.isDef() &&
        MO.getReg() == MI->getOperand(0).getReg())
      return;
  }
  // Constrains the regclass of DestReg to assign it to Slot
  switch (Slot) {
  case 0:
    MRI->constrainRegClass(DestReg, &AMDGPU::R600_TReg32_XRegClass);
    break;
  case 1:
    MRI->constrainRegClass(DestReg, &AMDGPU::R600_TReg32_YRegClass);
    break;
  case 2:
    MRI->constrainRegClass(DestReg, &AMDGPU::R600_TReg32_ZRegClass);
    break;
  case 3:
    MRI->constrainRegClass(DestReg, &AMDGPU::R600_TReg32_WRegClass);
    break;
  }
}

SUnit *R600SchedStrategy::AttemptFillSlot(unsigned Slot) {
  static const AluKind IndexToID[] = {AluT_X, AluT_Y, AluT_Z, AluT_W};
  SUnit *SlotedSU = PopInst(AvailableAlus[IndexToID[Slot]]);
  if (SlotedSU)
    return SlotedSU;
  SUnit *UnslotedSU = PopInst(AvailableAlus[AluAny]);
  if (UnslotedSU)
    AssignSlot(UnslotedSU->getInstr(), Slot);
  return UnslotedSU;
}

bool R600SchedStrategy::isAvailablesAluEmpty() const {
  return Pending[IDAlu].empty() && AvailableAlus[AluAny].empty() &&
      AvailableAlus[AluT_XYZW].empty() && AvailableAlus[AluT_X].empty() &&
      AvailableAlus[AluT_Y].empty() && AvailableAlus[AluT_Z].empty() &&
      AvailableAlus[AluT_W].empty() && AvailableAlus[AluDiscarded].empty() &&
      AvailableAlus[AluPredX].empty();
}

SUnit* R600SchedStrategy::pickAlu() {
  while (!isAvailablesAluEmpty()) {
    if (!OccupedSlotsMask) {
      // Bottom up scheduling : predX must comes first
      if (!AvailableAlus[AluPredX].empty()) {
        OccupedSlotsMask = 15;
        return PopInst(AvailableAlus[AluPredX]);
      }
      // Flush physical reg copies (RA will discard them)
      if (!AvailableAlus[AluDiscarded].empty()) {
        OccupedSlotsMask = 15;
        return PopInst(AvailableAlus[AluDiscarded]);
      }
      // If there is a T_XYZW alu available, use it
      if (!AvailableAlus[AluT_XYZW].empty()) {
        OccupedSlotsMask = 15;
        return PopInst(AvailableAlus[AluT_XYZW]);
      }
    }
    for (int Chan = 3; Chan > -1; --Chan) {
      bool isOccupied = OccupedSlotsMask & (1 << Chan);
      if (!isOccupied) {
        SUnit *SU = AttemptFillSlot(Chan);
        if (SU) {
          OccupedSlotsMask |= (1 << Chan);
          InstructionsGroupCandidate.push_back(SU->getInstr());
          return SU;
        }
      }
    }
    PrepareNextSlot();
  }
  return NULL;
}

SUnit* R600SchedStrategy::pickOther(int QID) {
  SUnit *SU = 0;
  std::vector<SUnit *> &AQ = Available[QID];

  if (AQ.empty()) {
    MoveUnits(Pending[QID], AQ);
  }
  if (!AQ.empty()) {
    SU = AQ.back();
    AQ.resize(AQ.size() - 1);
  }
  return SU;
}

