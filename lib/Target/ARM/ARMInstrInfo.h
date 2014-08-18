//===-- ARMInstrInfo.h - ARM Instruction Information ------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the ARM implementation of the TargetInstrInfo class.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_ARM_ARMINSTRINFO_H
#define LLVM_LIB_TARGET_ARM_ARMINSTRINFO_H

#include "ARMBaseInstrInfo.h"
#include "ARMRegisterInfo.h"

namespace llvm {
  class ARMSubtarget;

class ARMInstrInfo : public ARMBaseInstrInfo {
  ARMRegisterInfo RI;
public:
  explicit ARMInstrInfo(const ARMSubtarget &STI);

  /// getNoopForMachoTarget - Return the noop instruction to use for a noop.
  void getNoopForMachoTarget(MCInst &NopInst) const override;

  // Return the non-pre/post incrementing version of 'Opc'. Return 0
  // if there is not such an opcode.
  unsigned getUnindexedOpcode(unsigned Opc) const override;

  /// getRegisterInfo - TargetInstrInfo is a superset of MRegister info.  As
  /// such, whenever a client has an instance of instruction info, it should
  /// always be able to get register info as well (through this method).
  ///
  const ARMRegisterInfo &getRegisterInfo() const override { return RI; }

  /// Build the equivalent inputs of a REG_SEQUENCE for the given \p MI
  /// and \p DefIdx.
  /// \p [out] InputRegs of the equivalent REG_SEQUENCE. Each element of
  /// the list is modeled as <Reg:SubReg, SubIdx>.
  /// E.g., REG_SEQUENCE vreg1:sub1, sub0, vreg2, sub1 would produce
  /// two elements:
  /// - vreg1:sub1, sub0
  /// - vreg2<:0>, sub1
  ///
  /// \returns true if it is possible to build such an input sequence
  /// with the pair \p MI, \p DefIdx. False otherwise.
  ///
  /// \pre MI.isRegSequenceLike().
  bool getRegSequenceLikeInputs(
      const MachineInstr &MI, unsigned DefIdx,
      SmallVectorImpl<RegSubRegPairAndIdx> &InputRegs) const override;

private:
  void expandLoadStackGuard(MachineBasicBlock::iterator MI,
                            Reloc::Model RM) const override;
};

}

#endif
