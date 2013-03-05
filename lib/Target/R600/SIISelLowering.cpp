//===-- SIISelLowering.cpp - SI DAG Lowering Implementation ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// \brief Custom DAG lowering for SI
//
//===----------------------------------------------------------------------===//

#include "SIISelLowering.h"
#include "AMDIL.h"
#include "AMDILIntrinsicInfo.h"
#include "SIInstrInfo.h"
#include "SIMachineFunctionInfo.h"
#include "SIRegisterInfo.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/CodeGen/SelectionDAG.h"

using namespace llvm;

SITargetLowering::SITargetLowering(TargetMachine &TM) :
    AMDGPUTargetLowering(TM),
    TII(static_cast<const SIInstrInfo*>(TM.getInstrInfo())),
    TRI(TM.getRegisterInfo()) {
  addRegisterClass(MVT::v4f32, &AMDGPU::VReg_128RegClass);
  addRegisterClass(MVT::f32, &AMDGPU::VReg_32RegClass);
  addRegisterClass(MVT::i32, &AMDGPU::VReg_32RegClass);
  addRegisterClass(MVT::i64, &AMDGPU::SReg_64RegClass);
  addRegisterClass(MVT::i1, &AMDGPU::SReg_64RegClass);

  addRegisterClass(MVT::v1i32, &AMDGPU::VReg_32RegClass);
  addRegisterClass(MVT::v2i32, &AMDGPU::VReg_64RegClass);
  addRegisterClass(MVT::v4i32, &AMDGPU::VReg_128RegClass);
  addRegisterClass(MVT::v8i32, &AMDGPU::VReg_256RegClass);
  addRegisterClass(MVT::v16i32, &AMDGPU::VReg_512RegClass);

  computeRegisterProperties();

  setOperationAction(ISD::ADD, MVT::i64, Legal);
  setOperationAction(ISD::ADD, MVT::i32, Legal);

  setOperationAction(ISD::INTRINSIC_WO_CHAIN, MVT::Other, Custom);

  // We need to custom lower loads from the USER_SGPR address space, so we can
  // add the SGPRs as livein registers.
  setOperationAction(ISD::LOAD, MVT::i32, Custom);
  setOperationAction(ISD::LOAD, MVT::i64, Custom);

  setOperationAction(ISD::SELECT_CC, MVT::f32, Custom);
  setOperationAction(ISD::SELECT_CC, MVT::i32, Custom);

  setOperationAction(ISD::SELECT_CC, MVT::Other, Expand);
  setTargetDAGCombine(ISD::SELECT_CC);

  setTargetDAGCombine(ISD::SETCC);
}

MachineBasicBlock * SITargetLowering::EmitInstrWithCustomInserter(
    MachineInstr * MI, MachineBasicBlock * BB) const {
  MachineRegisterInfo & MRI = BB->getParent()->getRegInfo();
  MachineBasicBlock::iterator I = MI;

  switch (MI->getOpcode()) {
  default:
    return AMDGPUTargetLowering::EmitInstrWithCustomInserter(MI, BB);
  case AMDGPU::BRANCH: return BB;
  case AMDGPU::SHADER_TYPE:
    BB->getParent()->getInfo<SIMachineFunctionInfo>()->ShaderType =
                                        MI->getOperand(0).getImm();
    MI->eraseFromParent();
    break;

  case AMDGPU::SI_INTERP:
    LowerSI_INTERP(MI, *BB, I, MRI);
    break;
  case AMDGPU::SI_WQM:
    LowerSI_WQM(MI, *BB, I, MRI);
    break;
  }
  return BB;
}

void SITargetLowering::LowerSI_WQM(MachineInstr *MI, MachineBasicBlock &BB,
    MachineBasicBlock::iterator I, MachineRegisterInfo & MRI) const {
  BuildMI(BB, I, BB.findDebugLoc(I), TII->get(AMDGPU::S_WQM_B64), AMDGPU::EXEC)
          .addReg(AMDGPU::EXEC);

  MI->eraseFromParent();
}

void SITargetLowering::LowerSI_INTERP(MachineInstr *MI, MachineBasicBlock &BB,
    MachineBasicBlock::iterator I, MachineRegisterInfo & MRI) const {
  unsigned tmp = MRI.createVirtualRegister(&AMDGPU::VReg_32RegClass);
  unsigned M0 = MRI.createVirtualRegister(&AMDGPU::M0RegRegClass);
  MachineOperand dst = MI->getOperand(0);
  MachineOperand iReg = MI->getOperand(1);
  MachineOperand jReg = MI->getOperand(2);
  MachineOperand attr_chan = MI->getOperand(3);
  MachineOperand attr = MI->getOperand(4);
  MachineOperand params = MI->getOperand(5);

  BuildMI(BB, I, BB.findDebugLoc(I), TII->get(AMDGPU::S_MOV_B32), M0)
          .addOperand(params);

  BuildMI(BB, I, BB.findDebugLoc(I), TII->get(AMDGPU::V_INTERP_P1_F32), tmp)
          .addOperand(iReg)
          .addOperand(attr_chan)
          .addOperand(attr)
          .addReg(M0);

  BuildMI(BB, I, BB.findDebugLoc(I), TII->get(AMDGPU::V_INTERP_P2_F32))
          .addOperand(dst)
          .addReg(tmp)
          .addOperand(jReg)
          .addOperand(attr_chan)
          .addOperand(attr)
          .addReg(M0);

  MI->eraseFromParent();
}

EVT SITargetLowering::getSetCCResultType(EVT VT) const {
  return MVT::i1;
}

//===----------------------------------------------------------------------===//
// Custom DAG Lowering Operations
//===----------------------------------------------------------------------===//

SDValue SITargetLowering::LowerOperation(SDValue Op, SelectionDAG &DAG) const {
  switch (Op.getOpcode()) {
  default: return AMDGPUTargetLowering::LowerOperation(Op, DAG);
  case ISD::BRCOND: return LowerBRCOND(Op, DAG);
  case ISD::LOAD: return LowerLOAD(Op, DAG);
  case ISD::SELECT_CC: return LowerSELECT_CC(Op, DAG);
  case ISD::INTRINSIC_WO_CHAIN: {
    unsigned IntrinsicID =
                         cast<ConstantSDNode>(Op.getOperand(0))->getZExtValue();
    EVT VT = Op.getValueType();
    switch (IntrinsicID) {
    case AMDGPUIntrinsic::SI_vs_load_buffer_index:
      return CreateLiveInRegister(DAG, &AMDGPU::VReg_32RegClass,
                                  AMDGPU::VGPR0, VT);
    default: return AMDGPUTargetLowering::LowerOperation(Op, DAG);
    }
    break;
  }
  }
  return SDValue();
}

/// \brief Helper function for LowerBRCOND
static SDNode *findUser(SDValue Value, unsigned Opcode) {

  SDNode *Parent = Value.getNode();
  for (SDNode::use_iterator I = Parent->use_begin(), E = Parent->use_end();
       I != E; ++I) {

    if (I.getUse().get() != Value)
      continue;

    if (I->getOpcode() == Opcode)
      return *I;
  }
  return 0;
}

/// This transforms the control flow intrinsics to get the branch destination as
/// last parameter, also switches branch target with BR if the need arise
SDValue SITargetLowering::LowerBRCOND(SDValue BRCOND,
                                      SelectionDAG &DAG) const {

  DebugLoc DL = BRCOND.getDebugLoc();

  SDNode *Intr = BRCOND.getOperand(1).getNode();
  SDValue Target = BRCOND.getOperand(2);
  SDNode *BR = 0;

  if (Intr->getOpcode() == ISD::SETCC) {
    // As long as we negate the condition everything is fine
    SDNode *SetCC = Intr;
    assert(SetCC->getConstantOperandVal(1) == 1);
    assert(cast<CondCodeSDNode>(SetCC->getOperand(2).getNode())->get() ==
           ISD::SETNE);
    Intr = SetCC->getOperand(0).getNode();

  } else {
    // Get the target from BR if we don't negate the condition
    BR = findUser(BRCOND, ISD::BR);
    Target = BR->getOperand(1);
  }

  assert(Intr->getOpcode() == ISD::INTRINSIC_W_CHAIN);

  // Build the result and
  SmallVector<EVT, 4> Res;
  for (unsigned i = 1, e = Intr->getNumValues(); i != e; ++i)
    Res.push_back(Intr->getValueType(i));

  // operands of the new intrinsic call
  SmallVector<SDValue, 4> Ops;
  Ops.push_back(BRCOND.getOperand(0));
  for (unsigned i = 1, e = Intr->getNumOperands(); i != e; ++i)
    Ops.push_back(Intr->getOperand(i));
  Ops.push_back(Target);

  // build the new intrinsic call
  SDNode *Result = DAG.getNode(
    Res.size() > 1 ? ISD::INTRINSIC_W_CHAIN : ISD::INTRINSIC_VOID, DL,
    DAG.getVTList(Res.data(), Res.size()), Ops.data(), Ops.size()).getNode();

  if (BR) {
    // Give the branch instruction our target
    SDValue Ops[] = {
      BR->getOperand(0),
      BRCOND.getOperand(2)
    };
    DAG.MorphNodeTo(BR, ISD::BR, BR->getVTList(), Ops, 2);
  }

  SDValue Chain = SDValue(Result, Result->getNumValues() - 1);

  // Copy the intrinsic results to registers
  for (unsigned i = 1, e = Intr->getNumValues() - 1; i != e; ++i) {
    SDNode *CopyToReg = findUser(SDValue(Intr, i), ISD::CopyToReg);
    if (!CopyToReg)
      continue;

    Chain = DAG.getCopyToReg(
      Chain, DL,
      CopyToReg->getOperand(1),
      SDValue(Result, i - 1),
      SDValue());

    DAG.ReplaceAllUsesWith(SDValue(CopyToReg, 0), CopyToReg->getOperand(0));
  }

  // Remove the old intrinsic from the chain
  DAG.ReplaceAllUsesOfValueWith(
    SDValue(Intr, Intr->getNumValues() - 1),
    Intr->getOperand(0));

  return Chain;
}

SDValue SITargetLowering::LowerLOAD(SDValue Op, SelectionDAG &DAG) const {
  EVT VT = Op.getValueType();
  LoadSDNode *Ptr = dyn_cast<LoadSDNode>(Op);

  assert(Ptr);

  unsigned AddrSpace = Ptr->getPointerInfo().getAddrSpace();

  // We only need to lower USER_SGPR address space loads
  if (AddrSpace != AMDGPUAS::USER_SGPR_ADDRESS) {
    return SDValue();
  }

  // Loads from the USER_SGPR address space can only have constant value
  // pointers.
  ConstantSDNode *BasePtr = dyn_cast<ConstantSDNode>(Ptr->getBasePtr());
  assert(BasePtr);

  unsigned TypeDwordWidth = VT.getSizeInBits() / 32;
  const TargetRegisterClass * dstClass;
  switch (TypeDwordWidth) {
    default:
      assert(!"USER_SGPR value size not implemented");
      return SDValue();
    case 1:
      dstClass = &AMDGPU::SReg_32RegClass;
      break;
    case 2:
      dstClass = &AMDGPU::SReg_64RegClass;
      break;
  }
  uint64_t Index = BasePtr->getZExtValue();
  assert(Index % TypeDwordWidth == 0 && "USER_SGPR not properly aligned");
  unsigned SGPRIndex = Index / TypeDwordWidth;
  unsigned Reg = dstClass->getRegister(SGPRIndex);

  DAG.ReplaceAllUsesOfValueWith(Op, CreateLiveInRegister(DAG, dstClass, Reg,
                                                         VT));
  return SDValue();
}

SDValue SITargetLowering::LowerSELECT_CC(SDValue Op, SelectionDAG &DAG) const {
  SDValue LHS = Op.getOperand(0);
  SDValue RHS = Op.getOperand(1);
  SDValue True = Op.getOperand(2);
  SDValue False = Op.getOperand(3);
  SDValue CC = Op.getOperand(4);
  EVT VT = Op.getValueType();
  DebugLoc DL = Op.getDebugLoc();

  // Possible Min/Max pattern
  SDValue MinMax = LowerMinMax(Op, DAG);
  if (MinMax.getNode()) {
    return MinMax;
  }

  SDValue Cond = DAG.getNode(ISD::SETCC, DL, MVT::i1, LHS, RHS, CC);
  return DAG.getNode(ISD::SELECT, DL, VT, Cond, True, False);
}

//===----------------------------------------------------------------------===//
// Custom DAG optimizations
//===----------------------------------------------------------------------===//

SDValue SITargetLowering::PerformDAGCombine(SDNode *N,
                                            DAGCombinerInfo &DCI) const {
  SelectionDAG &DAG = DCI.DAG;
  DebugLoc DL = N->getDebugLoc();
  EVT VT = N->getValueType(0);

  switch (N->getOpcode()) {
    default: break;
    case ISD::SELECT_CC: {
      N->dump();
      ConstantSDNode *True, *False;
      // i1 selectcc(l, r, -1, 0, cc) -> i1 setcc(l, r, cc)
      if ((True = dyn_cast<ConstantSDNode>(N->getOperand(2)))
          && (False = dyn_cast<ConstantSDNode>(N->getOperand(3)))
          && True->isAllOnesValue()
          && False->isNullValue()
          && VT == MVT::i1) {
        return DAG.getNode(ISD::SETCC, DL, VT, N->getOperand(0),
                           N->getOperand(1), N->getOperand(4));

      }
      break;
    }
    case ISD::SETCC: {
      SDValue Arg0 = N->getOperand(0);
      SDValue Arg1 = N->getOperand(1);
      SDValue CC = N->getOperand(2);
      ConstantSDNode * C = NULL;
      ISD::CondCode CCOp = dyn_cast<CondCodeSDNode>(CC)->get();

      // i1 setcc (sext(i1), 0, setne) -> i1 setcc(i1, 0, setne)
      if (VT == MVT::i1
          && Arg0.getOpcode() == ISD::SIGN_EXTEND
          && Arg0.getOperand(0).getValueType() == MVT::i1
          && (C = dyn_cast<ConstantSDNode>(Arg1))
          && C->isNullValue()
          && CCOp == ISD::SETNE) {
        return SimplifySetCC(VT, Arg0.getOperand(0),
                             DAG.getConstant(0, MVT::i1), CCOp, true, DCI, DL);
      }
      break;
    }
  }
  return SDValue();
}

/// \brief Test if RegClass is one of the VSrc classes 
static bool isVSrc(unsigned RegClass) {
  return AMDGPU::VSrc_32RegClassID == RegClass ||
         AMDGPU::VSrc_64RegClassID == RegClass;
}

/// \brief Test if RegClass is one of the SSrc classes 
static bool isSSrc(unsigned RegClass) {
  return AMDGPU::SSrc_32RegClassID == RegClass ||
         AMDGPU::SSrc_64RegClassID == RegClass;
}

/// \brief Analyze the possible immediate value Op
///
/// Returns -1 if it isn't an immediate, 0 if it's and inline immediate
/// and the immediate value if it's a literal immediate
int32_t SITargetLowering::analyzeImmediate(const SDNode *N) const {

  union {
    int32_t I;
    float F;
  } Imm;

  if (const ConstantSDNode *Node = dyn_cast<ConstantSDNode>(N))
    Imm.I = Node->getSExtValue();
  else if (const ConstantFPSDNode *Node = dyn_cast<ConstantFPSDNode>(N))
    Imm.F = Node->getValueAPF().convertToFloat();
  else
    return -1; // It isn't an immediate

  if ((Imm.I >= -16 && Imm.I <= 64) ||
      Imm.F == 0.5f || Imm.F == -0.5f ||
      Imm.F == 1.0f || Imm.F == -1.0f ||
      Imm.F == 2.0f || Imm.F == -2.0f ||
      Imm.F == 4.0f || Imm.F == -4.0f)
    return 0; // It's an inline immediate

  return Imm.I; // It's a literal immediate
}

/// \brief Try to fold an immediate directly into an instruction
bool SITargetLowering::foldImm(SDValue &Operand, int32_t &Immediate,
                               bool &ScalarSlotUsed) const {

  MachineSDNode *Mov = dyn_cast<MachineSDNode>(Operand);
  if (Mov == 0 || !TII->isMov(Mov->getMachineOpcode()))
    return false;

  const SDValue &Op = Mov->getOperand(0);
  int32_t Value = analyzeImmediate(Op.getNode());
  if (Value == -1) {
    // Not an immediate at all
    return false;

  } else if (Value == 0) {
    // Inline immediates can always be fold
    Operand = Op;
    return true;

  } else if (Value == Immediate) {
    // Already fold literal immediate
    Operand = Op;
    return true;

  } else if (!ScalarSlotUsed && !Immediate) {
    // Fold this literal immediate
    ScalarSlotUsed = true;
    Immediate = Value;
    Operand = Op;
    return true;

  }

  return false;
}

/// \brief Does "Op" fit into register class "RegClass" ?
bool SITargetLowering::fitsRegClass(SelectionDAG &DAG, SDValue &Op,
                                    unsigned RegClass) const {

  MachineRegisterInfo &MRI = DAG.getMachineFunction().getRegInfo(); 
  SDNode *Node = Op.getNode();

  int OpClass;
  if (MachineSDNode *MN = dyn_cast<MachineSDNode>(Node)) {
    const MCInstrDesc &Desc = TII->get(MN->getMachineOpcode());
    OpClass = Desc.OpInfo[Op.getResNo()].RegClass;

  } else if (Node->getOpcode() == ISD::CopyFromReg) {
    RegisterSDNode *Reg = cast<RegisterSDNode>(Node->getOperand(1).getNode());
    OpClass = MRI.getRegClass(Reg->getReg())->getID();

  } else
    return false;

  if (OpClass == -1)
    return false;

  return TRI->getRegClass(RegClass)->hasSubClassEq(TRI->getRegClass(OpClass));
}

/// \brief Make sure that we don't exeed the number of allowed scalars
void SITargetLowering::ensureSRegLimit(SelectionDAG &DAG, SDValue &Operand,
                                       unsigned RegClass,
                                       bool &ScalarSlotUsed) const {

  // First map the operands register class to a destination class
  if (RegClass == AMDGPU::VSrc_32RegClassID)
    RegClass = AMDGPU::VReg_32RegClassID;
  else if (RegClass == AMDGPU::VSrc_64RegClassID)
    RegClass = AMDGPU::VReg_64RegClassID;
  else
    return;

  // Nothing todo if they fit naturaly
  if (fitsRegClass(DAG, Operand, RegClass))
    return;

  // If the scalar slot isn't used yet use it now
  if (!ScalarSlotUsed) {
    ScalarSlotUsed = true;
    return;
  }

  // This is a conservative aproach, it is possible that we can't determine
  // the correct register class and copy too often, but better save than sorry.
  SDValue RC = DAG.getTargetConstant(RegClass, MVT::i32);
  SDNode *Node = DAG.getMachineNode(TargetOpcode::COPY_TO_REGCLASS, DebugLoc(),
                                    Operand.getValueType(), Operand, RC);
  Operand = SDValue(Node, 0);
}

SDNode *SITargetLowering::PostISelFolding(MachineSDNode *Node,
                                          SelectionDAG &DAG) const {

  // Original encoding (either e32 or e64)
  int Opcode = Node->getMachineOpcode();
  const MCInstrDesc *Desc = &TII->get(Opcode);

  unsigned NumDefs = Desc->getNumDefs();
  unsigned NumOps = Desc->getNumOperands();

  // e64 version if available, -1 otherwise
  int OpcodeE64 = AMDGPU::getVOPe64(Opcode);
  const MCInstrDesc *DescE64 = OpcodeE64 == -1 ? 0 : &TII->get(OpcodeE64);

  assert(!DescE64 || DescE64->getNumDefs() == NumDefs);
  assert(!DescE64 || DescE64->getNumOperands() == (NumOps + 4));

  int32_t Immediate = Desc->getSize() == 4 ? 0 : -1;
  bool HaveVSrc = false, HaveSSrc = false;

  // First figure out what we alread have in this instruction
  for (unsigned i = 0, e = Node->getNumOperands(), Op = NumDefs;
       i != e && Op < NumOps; ++i, ++Op) {

    unsigned RegClass = Desc->OpInfo[Op].RegClass;
    if (isVSrc(RegClass))
      HaveVSrc = true;
    else if (isSSrc(RegClass))
      HaveSSrc = true;
    else
      continue;

    int32_t Imm = analyzeImmediate(Node->getOperand(i).getNode());
    if (Imm != -1 && Imm != 0) {
      // Literal immediate
      Immediate = Imm;
    }
  }

  // If we neither have VSrc nor SSrc it makes no sense to continue
  if (!HaveVSrc && !HaveSSrc)
    return Node;

  // No scalar allowed when we have both VSrc and SSrc
  bool ScalarSlotUsed = HaveVSrc && HaveSSrc;

  // Second go over the operands and try to fold them
  std::vector<SDValue> Ops;
  bool Promote2e64 = false;
  for (unsigned i = 0, e = Node->getNumOperands(), Op = NumDefs;
       i != e && Op < NumOps; ++i, ++Op) {

    const SDValue &Operand = Node->getOperand(i);
    Ops.push_back(Operand);

    // Already folded immediate ?
    if (isa<ConstantSDNode>(Operand.getNode()) ||
        isa<ConstantFPSDNode>(Operand.getNode()))
      continue;

    // Is this a VSrc or SSrc operand ?
    unsigned RegClass = Desc->OpInfo[Op].RegClass;
    if (!isVSrc(RegClass) && !isSSrc(RegClass)) {

      if (i == 1 && Desc->isCommutable() &&
          fitsRegClass(DAG, Ops[0], RegClass) &&
          foldImm(Ops[1], Immediate, ScalarSlotUsed)) {

        assert(isVSrc(Desc->OpInfo[NumDefs].RegClass) ||
               isSSrc(Desc->OpInfo[NumDefs].RegClass));

        // Swap commutable operands
        SDValue Tmp = Ops[1];
        Ops[1] = Ops[0];
        Ops[0] = Tmp;

      } else if (DescE64 && !Immediate) {
        // Test if it makes sense to switch to e64 encoding

        RegClass = DescE64->OpInfo[Op].RegClass;
        int32_t TmpImm = -1;
        if ((isVSrc(RegClass) || isSSrc(RegClass)) &&
            foldImm(Ops[i], TmpImm, ScalarSlotUsed)) {

          Immediate = -1;
          Promote2e64 = true;
          Desc = DescE64;
          DescE64 = 0;
        }
      }
      continue;
    }

    // Try to fold the immediates
    if (!foldImm(Ops[i], Immediate, ScalarSlotUsed)) {
      // Folding didn't worked, make sure we don't hit the SReg limit
      ensureSRegLimit(DAG, Ops[i], RegClass, ScalarSlotUsed);
    }
  }

  if (Promote2e64) {
    // Add the modifier flags while promoting
    for (unsigned i = 0; i < 4; ++i)
      Ops.push_back(DAG.getTargetConstant(0, MVT::i32));
  }

  // Add optional chain and glue
  for (unsigned i = NumOps - NumDefs, e = Node->getNumOperands(); i < e; ++i)
    Ops.push_back(Node->getOperand(i));

  // Either create a complete new or update the current instruction
  if (Promote2e64)
    return DAG.getMachineNode(OpcodeE64, Node->getDebugLoc(),
                              Node->getVTList(), Ops.data(), Ops.size());
  else
    return DAG.UpdateNodeOperands(Node, Ops.data(), Ops.size());
}
