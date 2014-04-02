//===-- llvm/CodeGen/DebugLocEntry.h - Entry in debug_loc list -*- C++ -*--===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef CODEGEN_ASMPRINTER_DEBUGLOCENTRY_H__
#define CODEGEN_ASMPRINTER_DEBUGLOCENTRY_H__
#include "llvm/IR/Constants.h"
#include "llvm/MC/MachineLocation.h"
#include "llvm/MC/MCSymbol.h"

namespace llvm {
class DwarfCompileUnit;
class MDNode;
/// \brief This struct describes location entries emitted in the .debug_loc
/// section.
class DebugLocEntry {
  // Begin and end symbols for the address range that this location is valid.
  const MCSymbol *Begin;
  const MCSymbol *End;

  // Type of entry that this represents.
  enum EntryType { E_Location, E_Integer, E_ConstantFP, E_ConstantInt };
  enum EntryType EntryKind;

  union {
    int64_t Int;
    const ConstantFP *CFP;
    const ConstantInt *CIP;
  } Constants;

  // The location in the machine frame.
  MachineLocation Loc;

  // The variable to which this location entry corresponds.
  const MDNode *Variable;

  // The compile unit to which this location entry is referenced by.
  const DwarfCompileUnit *Unit;

  bool hasSameValueOrLocation(const DebugLocEntry &Next) {
    if (EntryKind != Next.EntryKind)
      return false;

    bool EqualValues;
    switch (EntryKind) {
    case E_Location:
      EqualValues = Loc == Next.Loc;
      break;
    case E_Integer:
      EqualValues = Constants.Int == Next.Constants.Int;
      break;
    case E_ConstantFP:
      EqualValues = Constants.CFP == Next.Constants.CFP;
      break;
    case E_ConstantInt:
      EqualValues = Constants.CIP == Next.Constants.CIP;
      break;
    }

    return EqualValues;
  }

public:
  DebugLocEntry() : Begin(0), End(0), Variable(0), Unit(0) {
    Constants.Int = 0;
  }
  DebugLocEntry(const MCSymbol *B, const MCSymbol *E, MachineLocation &L,
                const MDNode *V, const DwarfCompileUnit *U)
      : Begin(B), End(E), Loc(L), Variable(V), Unit(U) {
    Constants.Int = 0;
    EntryKind = E_Location;
  }
  DebugLocEntry(const MCSymbol *B, const MCSymbol *E, int64_t i,
                const DwarfCompileUnit *U)
      : Begin(B), End(E), Variable(0), Unit(U) {
    Constants.Int = i;
    EntryKind = E_Integer;
  }
  DebugLocEntry(const MCSymbol *B, const MCSymbol *E, const ConstantFP *FPtr,
                const DwarfCompileUnit *U)
      : Begin(B), End(E), Variable(0), Unit(U) {
    Constants.CFP = FPtr;
    EntryKind = E_ConstantFP;
  }
  DebugLocEntry(const MCSymbol *B, const MCSymbol *E, const ConstantInt *IPtr,
                const DwarfCompileUnit *U)
      : Begin(B), End(E), Variable(0), Unit(U) {
    Constants.CIP = IPtr;
    EntryKind = E_ConstantInt;
  }

  /// \brief Attempt to merge this DebugLocEntry with Next and return
  /// true if the merge was successful. Entries can be merged if they
  /// share the same Loc/Constant and if Next immediately follows this
  /// Entry.
  bool Merge(const DebugLocEntry &Next) {
    if (End == Next.Begin && hasSameValueOrLocation(Next)) {
      End = Next.End;
      return true;
    }
    return false;
  }
  bool isLocation() const { return EntryKind == E_Location; }
  bool isInt() const { return EntryKind == E_Integer; }
  bool isConstantFP() const { return EntryKind == E_ConstantFP; }
  bool isConstantInt() const { return EntryKind == E_ConstantInt; }
  int64_t getInt() const { return Constants.Int; }
  const ConstantFP *getConstantFP() const { return Constants.CFP; }
  const ConstantInt *getConstantInt() const { return Constants.CIP; }
  const MDNode *getVariable() const { return Variable; }
  const MCSymbol *getBeginSym() const { return Begin; }
  const MCSymbol *getEndSym() const { return End; }
  const DwarfCompileUnit *getCU() const { return Unit; }
  MachineLocation getLoc() const { return Loc; }
};

}
#endif
