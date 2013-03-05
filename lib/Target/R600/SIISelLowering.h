//===-- SIISelLowering.h - SI DAG Lowering Interface ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief SI DAG Lowering interface definition
//
//===----------------------------------------------------------------------===//

#ifndef SIISELLOWERING_H
#define SIISELLOWERING_H

#include "AMDGPUISelLowering.h"
#include "SIInstrInfo.h"

namespace llvm {

class SITargetLowering : public AMDGPUTargetLowering {
  const SIInstrInfo * TII;
  const TargetRegisterInfo * TRI;

  void LowerMOV_IMM(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I, unsigned Opocde) const;
  void LowerSI_INTERP(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I, MachineRegisterInfo & MRI) const;
  void LowerSI_WQM(MachineInstr *MI, MachineBasicBlock &BB,
              MachineBasicBlock::iterator I, MachineRegisterInfo & MRI) const;

  SDValue LowerLOAD(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const;
  SDValue LowerBRCOND(SDValue Op, SelectionDAG &DAG) const;

  bool foldImm(SDValue &Operand, int32_t &Immediate,
               bool &ScalarSlotUsed) const;
  bool fitsRegClass(SelectionDAG &DAG, SDValue &Op, unsigned RegClass) const;
  void ensureSRegLimit(SelectionDAG &DAG, SDValue &Operand, 
                       unsigned RegClass, bool &ScalarSlotUsed) const;

public:
  SITargetLowering(TargetMachine &tm);
  virtual MachineBasicBlock * EmitInstrWithCustomInserter(MachineInstr * MI,
                                              MachineBasicBlock * BB) const;
  virtual EVT getSetCCResultType(EVT VT) const;
  virtual SDValue LowerOperation(SDValue Op, SelectionDAG &DAG) const;
  virtual SDValue PerformDAGCombine(SDNode *N, DAGCombinerInfo &DCI) const;
  virtual SDNode *PostISelFolding(MachineSDNode *N, SelectionDAG &DAG) const;

  int32_t analyzeImmediate(const SDNode *N) const;
};

} // End namespace llvm

#endif //SIISELLOWERING_H
