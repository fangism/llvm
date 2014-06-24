//===-- ARMTargetMachine.h - Define TargetMachine for ARM -------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file declares the ARM specific subclass of TargetMachine.
//
//===----------------------------------------------------------------------===//

#ifndef ARMTARGETMACHINE_H
#define ARMTARGETMACHINE_H

#include "ARMFrameLowering.h"
#include "ARMISelLowering.h"
#include "ARMInstrInfo.h"
#include "ARMJITInfo.h"
#include "ARMSelectionDAGInfo.h"
#include "ARMSubtarget.h"
#include "Thumb1FrameLowering.h"
#include "Thumb1InstrInfo.h"
#include "Thumb2InstrInfo.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/MC/MCStreamer.h"
#include "llvm/Target/TargetMachine.h"

namespace llvm {

class ARMBaseTargetMachine : public LLVMTargetMachine {
protected:
  ARMSubtarget        Subtarget;
public:
  ARMBaseTargetMachine(const Target &T, StringRef TT,
                       StringRef CPU, StringRef FS,
                       const TargetOptions &Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL,
                       bool isLittle);

  const ARMSubtarget *getSubtargetImpl() const override { return &Subtarget; }
  const ARMTargetLowering *getTargetLowering() const override {
    // Implemented by derived classes
    llvm_unreachable("getTargetLowering not implemented");
  }
  const InstrItineraryData *getInstrItineraryData() const override {
    return &getSubtargetImpl()->getInstrItineraryData();
  }
  const DataLayout *getDataLayout() const override {
    return getSubtargetImpl()->getDataLayout();
  }
  ARMJITInfo *getJITInfo() override { return Subtarget.getJITInfo(); }

  /// \brief Register ARM analysis passes with a pass manager.
  void addAnalysisPasses(PassManagerBase &PM) override;

  // Pass Pipeline Configuration
  TargetPassConfig *createPassConfig(PassManagerBase &PM) override;

  bool addCodeEmitter(PassManagerBase &PM, JITCodeEmitter &MCE) override;
};

/// ARMTargetMachine - ARM target machine.
///
class ARMTargetMachine : public ARMBaseTargetMachine {
  virtual void anchor();
  ARMInstrInfo        InstrInfo;
  ARMTargetLowering   TLInfo;
  ARMFrameLowering    FrameLowering;
 public:
  ARMTargetMachine(const Target &T, StringRef TT,
                   StringRef CPU, StringRef FS,
                   const TargetOptions &Options,
                   Reloc::Model RM, CodeModel::Model CM,
                   CodeGenOpt::Level OL,
                   bool isLittle);

  const ARMRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo.getRegisterInfo();
  }

  const ARMTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const ARMSelectionDAGInfo *getSelectionDAGInfo() const override {
    return getSubtargetImpl()->getSelectionDAGInfo();
  }
  const ARMFrameLowering *getFrameLowering() const override {
    return &FrameLowering;
  }
  const ARMInstrInfo *getInstrInfo() const override { return &InstrInfo; }
};

/// ARMLETargetMachine - ARM little endian target machine.
///
class ARMLETargetMachine : public ARMTargetMachine {
  void anchor() override;
public:
  ARMLETargetMachine(const Target &T, StringRef TT,
                     StringRef CPU, StringRef FS, const TargetOptions &Options,
                     Reloc::Model RM, CodeModel::Model CM,
                     CodeGenOpt::Level OL);
};

/// ARMBETargetMachine - ARM big endian target machine.
///
class ARMBETargetMachine : public ARMTargetMachine {
  void anchor() override;
public:
  ARMBETargetMachine(const Target &T, StringRef TT,
                     StringRef CPU, StringRef FS, const TargetOptions &Options,
                     Reloc::Model RM, CodeModel::Model CM,
                     CodeGenOpt::Level OL);
};

/// ThumbTargetMachine - Thumb target machine.
/// Due to the way architectures are handled, this represents both
///   Thumb-1 and Thumb-2.
///
class ThumbTargetMachine : public ARMBaseTargetMachine {
  virtual void anchor();
  // Either Thumb1InstrInfo or Thumb2InstrInfo.
  std::unique_ptr<ARMBaseInstrInfo> InstrInfo;
  ARMTargetLowering   TLInfo;
  // Either Thumb1FrameLowering or ARMFrameLowering.
  std::unique_ptr<ARMFrameLowering> FrameLowering;
public:
  ThumbTargetMachine(const Target &T, StringRef TT,
                     StringRef CPU, StringRef FS,
                     const TargetOptions &Options,
                     Reloc::Model RM, CodeModel::Model CM,
                     CodeGenOpt::Level OL,
                     bool isLittle);

  /// returns either Thumb1RegisterInfo or Thumb2RegisterInfo
  const ARMBaseRegisterInfo *getRegisterInfo() const override {
    return &InstrInfo->getRegisterInfo();
  }

  const ARMTargetLowering *getTargetLowering() const override {
    return &TLInfo;
  }

  const ARMSelectionDAGInfo *getSelectionDAGInfo() const override {
    return getSubtargetImpl()->getSelectionDAGInfo();
  }

  /// returns either Thumb1InstrInfo or Thumb2InstrInfo
  const ARMBaseInstrInfo *getInstrInfo() const override {
    return InstrInfo.get();
  }
  /// returns either Thumb1FrameLowering or ARMFrameLowering
  const ARMFrameLowering *getFrameLowering() const override {
    return FrameLowering.get();
  }
};

/// ThumbLETargetMachine - Thumb little endian target machine.
///
class ThumbLETargetMachine : public ThumbTargetMachine {
  void anchor() override;
public:
  ThumbLETargetMachine(const Target &T, StringRef TT,
                     StringRef CPU, StringRef FS, const TargetOptions &Options,
                     Reloc::Model RM, CodeModel::Model CM,
                     CodeGenOpt::Level OL);
};

/// ThumbBETargetMachine - Thumb big endian target machine.
///
class ThumbBETargetMachine : public ThumbTargetMachine {
  void anchor() override;
public:
  ThumbBETargetMachine(const Target &T, StringRef TT, StringRef CPU,
                       StringRef FS, const TargetOptions &Options,
                       Reloc::Model RM, CodeModel::Model CM,
                       CodeGenOpt::Level OL);
};

} // end namespace llvm

#endif
