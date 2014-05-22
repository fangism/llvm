//===- MCTargetOptions.h - MC Target Options -------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_MC_MCTARGETOPTIONS_H
#define LLVM_MC_MCTARGETOPTIONS_H

namespace llvm {

class MCTargetOptions {
public:
  enum AsmInstrumentation {
    AsmInstrumentationNone,
    AsmInstrumentationAddress
  };

  /// Enables AddressSanitizer instrumentation at machine level.
  bool SanitizeAddress : 1;

  unsigned MCRelaxAll : 1;
  unsigned MCNoExecStack : 1;
  unsigned MCSaveTempLabels : 1;
  unsigned MCUseDwarfDirectory : 1;
  unsigned ShowMCEncoding : 1;
  unsigned ShowMCInst : 1;
  unsigned AsmVerbose : 1;
  MCTargetOptions();
};

inline bool operator==(const MCTargetOptions &LHS, const MCTargetOptions &RHS) {
#define ARE_EQUAL(X) LHS.X == RHS.X
  return (ARE_EQUAL(SanitizeAddress) &&
          ARE_EQUAL(MCRelaxAll) &&
          ARE_EQUAL(MCNoExecStack) &&
          ARE_EQUAL(MCSaveTempLabels) &&
          ARE_EQUAL(MCUseDwarfDirectory) &&
          ARE_EQUAL(ShowMCEncoding) &&
          ARE_EQUAL(ShowMCInst) &&
          ARE_EQUAL(AsmVerbose));
#undef ARE_EQUAL
}

inline bool operator!=(const MCTargetOptions &LHS, const MCTargetOptions &RHS) {
  return !(LHS == RHS);
}

} // end namespace llvm

#endif
