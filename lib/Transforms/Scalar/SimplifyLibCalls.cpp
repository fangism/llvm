//===- SimplifyLibCalls.cpp - Optimize specific well-known library calls --===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file implements a simple pass that applies a variety of small
// optimizations for calls to specific well-known function calls (e.g. runtime
// library functions).   Any optimization that takes the very simple form
// "replace call to library function with simpler code that provides the same
// result" belongs in this file.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "simplify-libcalls"
#include "llvm/Transforms/Scalar.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Config/config.h"            // FIXME: Shouldn't depend on host!
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetLibraryInfo.h"
#include "llvm/Transforms/Utils/BuildLibCalls.h"
using namespace llvm;

STATISTIC(NumAnnotated, "Number of attributes added to library functions");

//===----------------------------------------------------------------------===//
// Optimizer Base Class
//===----------------------------------------------------------------------===//

/// This class is the abstract base class for the set of optimizations that
/// corresponds to one library call.
namespace {
class LibCallOptimization {
protected:
  Function *Caller;
  const DataLayout *TD;
  const TargetLibraryInfo *TLI;
  LLVMContext* Context;
public:
  LibCallOptimization() { }
  virtual ~LibCallOptimization() {}

  /// CallOptimizer - This pure virtual method is implemented by base classes to
  /// do various optimizations.  If this returns null then no transformation was
  /// performed.  If it returns CI, then it transformed the call and CI is to be
  /// deleted.  If it returns something else, replace CI with the new value and
  /// delete CI.
  virtual Value *CallOptimizer(Function *Callee, CallInst *CI, IRBuilder<> &B)
    =0;

  Value *OptimizeCall(CallInst *CI, const DataLayout *TD,
                      const TargetLibraryInfo *TLI, IRBuilder<> &B) {
    Caller = CI->getParent()->getParent();
    this->TD = TD;
    this->TLI = TLI;
    if (CI->getCalledFunction())
      Context = &CI->getCalledFunction()->getContext();

    // We never change the calling convention.
    if (CI->getCallingConv() != llvm::CallingConv::C)
      return NULL;

    return CallOptimizer(CI->getCalledFunction(), CI, B);
  }
};
} // End anonymous namespace.


//===----------------------------------------------------------------------===//
// SimplifyLibCalls Pass Implementation
//===----------------------------------------------------------------------===//

namespace {
  /// This pass optimizes well known library functions from libc and libm.
  ///
  class SimplifyLibCalls : public FunctionPass {
    TargetLibraryInfo *TLI;

    StringMap<LibCallOptimization*> Optimizations;

    bool Modified;  // This is only used by doInitialization.
  public:
    static char ID; // Pass identification
    SimplifyLibCalls() : FunctionPass(ID) {
      initializeSimplifyLibCallsPass(*PassRegistry::getPassRegistry());
    }
    void AddOpt(LibFunc::Func F, LibCallOptimization* Opt);
    void AddOpt(LibFunc::Func F1, LibFunc::Func F2, LibCallOptimization* Opt);

    void InitOptimizations();
    bool runOnFunction(Function &F);

    void setDoesNotAccessMemory(Function &F);
    void setOnlyReadsMemory(Function &F);
    void setDoesNotThrow(Function &F);
    void setDoesNotCapture(Function &F, unsigned n);
    void setDoesNotAlias(Function &F, unsigned n);
    bool doInitialization(Module &M);

    void inferPrototypeAttributes(Function &F);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.addRequired<TargetLibraryInfo>();
    }
  };
} // end anonymous namespace.

char SimplifyLibCalls::ID = 0;

INITIALIZE_PASS_BEGIN(SimplifyLibCalls, "simplify-libcalls",
                      "Simplify well-known library calls", false, false)
INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfo)
INITIALIZE_PASS_END(SimplifyLibCalls, "simplify-libcalls",
                    "Simplify well-known library calls", false, false)

// Public interface to the Simplify LibCalls pass.
FunctionPass *llvm::createSimplifyLibCallsPass() {
  return new SimplifyLibCalls();
}

void SimplifyLibCalls::AddOpt(LibFunc::Func F, LibCallOptimization* Opt) {
  if (TLI->has(F))
    Optimizations[TLI->getName(F)] = Opt;
}

void SimplifyLibCalls::AddOpt(LibFunc::Func F1, LibFunc::Func F2,
                              LibCallOptimization* Opt) {
  if (TLI->has(F1) && TLI->has(F2))
    Optimizations[TLI->getName(F1)] = Opt;
}

/// Optimizations - Populate the Optimizations map with all the optimizations
/// we know.
void SimplifyLibCalls::InitOptimizations() {
}


/// runOnFunction - Top level algorithm.
///
bool SimplifyLibCalls::runOnFunction(Function &F) {
  TLI = &getAnalysis<TargetLibraryInfo>();

  if (Optimizations.empty())
    InitOptimizations();

  const DataLayout *TD = getAnalysisIfAvailable<DataLayout>();

  IRBuilder<> Builder(F.getContext());

  bool Changed = false;
  for (Function::iterator BB = F.begin(), E = F.end(); BB != E; ++BB) {
    for (BasicBlock::iterator I = BB->begin(), E = BB->end(); I != E; ) {
      // Ignore non-calls.
      CallInst *CI = dyn_cast<CallInst>(I++);
      if (!CI || CI->hasFnAttr(Attribute::NoBuiltin)) continue;

      // Ignore indirect calls and calls to non-external functions.
      Function *Callee = CI->getCalledFunction();
      if (Callee == 0 || !Callee->isDeclaration() ||
          !(Callee->hasExternalLinkage() || Callee->hasDLLImportLinkage()))
        continue;

      // Ignore unknown calls.
      LibCallOptimization *LCO = Optimizations.lookup(Callee->getName());
      if (!LCO) continue;

      // Set the builder to the instruction after the call.
      Builder.SetInsertPoint(BB, I);

      // Use debug location of CI for all new instructions.
      Builder.SetCurrentDebugLocation(CI->getDebugLoc());

      // Try to optimize this call.
      Value *Result = LCO->OptimizeCall(CI, TD, TLI, Builder);
      if (Result == 0) continue;

      DEBUG(dbgs() << "SimplifyLibCalls simplified: " << *CI;
            dbgs() << "  into: " << *Result << "\n");

      // Something changed!
      Changed = true;

      // Inspect the instruction after the call (which was potentially just
      // added) next.
      I = CI; ++I;

      if (CI != Result && !CI->use_empty()) {
        CI->replaceAllUsesWith(Result);
        if (!Result->hasName())
          Result->takeName(CI);
      }
      CI->eraseFromParent();
    }
  }
  return Changed;
}

// Utility methods for doInitialization.

void SimplifyLibCalls::setDoesNotAccessMemory(Function &F) {
  if (!F.doesNotAccessMemory()) {
    F.setDoesNotAccessMemory();
    ++NumAnnotated;
    Modified = true;
  }
}
void SimplifyLibCalls::setOnlyReadsMemory(Function &F) {
  if (!F.onlyReadsMemory()) {
    F.setOnlyReadsMemory();
    ++NumAnnotated;
    Modified = true;
  }
}
void SimplifyLibCalls::setDoesNotThrow(Function &F) {
  if (!F.doesNotThrow()) {
    F.setDoesNotThrow();
    ++NumAnnotated;
    Modified = true;
  }
}
void SimplifyLibCalls::setDoesNotCapture(Function &F, unsigned n) {
  if (!F.doesNotCapture(n)) {
    F.setDoesNotCapture(n);
    ++NumAnnotated;
    Modified = true;
  }
}
void SimplifyLibCalls::setDoesNotAlias(Function &F, unsigned n) {
  if (!F.doesNotAlias(n)) {
    F.setDoesNotAlias(n);
    ++NumAnnotated;
    Modified = true;
  }
}


void SimplifyLibCalls::inferPrototypeAttributes(Function &F) {
  FunctionType *FTy = F.getFunctionType();

  StringRef Name = F.getName();
  switch (Name[0]) {
  case 's':
    if (Name == "strlen") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setOnlyReadsMemory(F);
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "strchr" ||
               Name == "strrchr") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isIntegerTy())
        return;
      setOnlyReadsMemory(F);
      setDoesNotThrow(F);
    } else if (Name == "strcpy" ||
               Name == "stpcpy" ||
               Name == "strcat" ||
               Name == "strtol" ||
               Name == "strtod" ||
               Name == "strtof" ||
               Name == "strtoul" ||
               Name == "strtoll" ||
               Name == "strtold" ||
               Name == "strncat" ||
               Name == "strncpy" ||
               Name == "stpncpy" ||
               Name == "strtoull") {
      if (FTy->getNumParams() < 2 ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "strxfrm") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "strcmp" ||
               Name == "strspn" ||
               Name == "strncmp" ||
               Name == "strcspn" ||
               Name == "strcoll" ||
               Name == "strcasecmp" ||
               Name == "strncasecmp") {
      if (FTy->getNumParams() < 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setOnlyReadsMemory(F);
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "strstr" ||
               Name == "strpbrk") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setOnlyReadsMemory(F);
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "strtok" ||
               Name == "strtok_r") {
      if (FTy->getNumParams() < 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "scanf" ||
               Name == "setbuf" ||
               Name == "setvbuf") {
      if (FTy->getNumParams() < 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "strdup" ||
               Name == "strndup") {
      if (FTy->getNumParams() < 1 || !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
    } else if (Name == "stat" ||
               Name == "sscanf" ||
               Name == "sprintf" ||
               Name == "statvfs") {
      if (FTy->getNumParams() < 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "snprintf") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(2)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 3);
    } else if (Name == "setitimer") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(1)->isPointerTy() ||
          !FTy->getParamType(2)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
      setDoesNotCapture(F, 3);
    } else if (Name == "system") {
      if (FTy->getNumParams() != 1 ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      // May throw; "system" is a valid pthread cancellation point.
      setDoesNotCapture(F, 1);
    }
    break;
  case 'm':
    if (Name == "malloc") {
      if (FTy->getNumParams() != 1 ||
          !FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
    } else if (Name == "memcmp") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setOnlyReadsMemory(F);
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "memchr" ||
               Name == "memrchr") {
      if (FTy->getNumParams() != 3)
        return;
      setOnlyReadsMemory(F);
      setDoesNotThrow(F);
    } else if (Name == "modf" ||
               Name == "modff" ||
               Name == "modfl" ||
               Name == "memcpy" ||
               Name == "memccpy" ||
               Name == "memmove") {
      if (FTy->getNumParams() < 2 ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "memalign") {
      if (!FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotAlias(F, 0);
    } else if (Name == "mkdir" ||
               Name == "mktime") {
      if (FTy->getNumParams() == 0 ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'r':
    if (Name == "realloc") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
    } else if (Name == "read") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      // May throw; "read" is a valid pthread cancellation point.
      setDoesNotCapture(F, 2);
    } else if (Name == "rmdir" ||
               Name == "rewind" ||
               Name == "remove" ||
               Name == "realpath") {
      if (FTy->getNumParams() < 1 ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "rename" ||
               Name == "readlink") {
      if (FTy->getNumParams() < 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    }
    break;
  case 'w':
    if (Name == "write") {
      if (FTy->getNumParams() != 3 || !FTy->getParamType(1)->isPointerTy())
        return;
      // May throw; "write" is a valid pthread cancellation point.
      setDoesNotCapture(F, 2);
    }
    break;
  case 'b':
    if (Name == "bcopy") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "bcmp") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setOnlyReadsMemory(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "bzero") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'c':
    if (Name == "calloc") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
    } else if (Name == "chmod" ||
               Name == "chown" ||
               Name == "ctermid" ||
               Name == "clearerr" ||
               Name == "closedir") {
      if (FTy->getNumParams() == 0 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'a':
    if (Name == "atoi" ||
        Name == "atol" ||
        Name == "atof" ||
        Name == "atoll") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setOnlyReadsMemory(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "access") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'f':
    if (Name == "fopen") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "fdopen") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 2);
    } else if (Name == "feof" ||
               Name == "free" ||
               Name == "fseek" ||
               Name == "ftell" ||
               Name == "fgetc" ||
               Name == "fseeko" ||
               Name == "ftello" ||
               Name == "fileno" ||
               Name == "fflush" ||
               Name == "fclose" ||
               Name == "fsetpos" ||
               Name == "flockfile" ||
               Name == "funlockfile" ||
               Name == "ftrylockfile") {
      if (FTy->getNumParams() == 0 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "ferror") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setOnlyReadsMemory(F);
    } else if (Name == "fputc" ||
               Name == "fstat" ||
               Name == "frexp" ||
               Name == "frexpf" ||
               Name == "frexpl" ||
               Name == "fstatvfs") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "fgets") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(2)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 3);
    } else if (Name == "fread" ||
               Name == "fwrite") {
      if (FTy->getNumParams() != 4 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(3)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 4);
    } else if (Name == "fputs" ||
               Name == "fscanf" ||
               Name == "fprintf" ||
               Name == "fgetpos") {
      if (FTy->getNumParams() < 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    }
    break;
  case 'g':
    if (Name == "getc" ||
        Name == "getlogin_r" ||
        Name == "getc_unlocked") {
      if (FTy->getNumParams() == 0 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "getenv") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setOnlyReadsMemory(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "gets" ||
               Name == "getchar") {
      setDoesNotThrow(F);
    } else if (Name == "getitimer") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "getpwnam") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'u':
    if (Name == "ungetc") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "uname" ||
               Name == "unlink" ||
               Name == "unsetenv") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "utime" ||
               Name == "utimes") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    }
    break;
  case 'p':
    if (Name == "putc") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "puts" ||
               Name == "printf" ||
               Name == "perror") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "pread" ||
               Name == "pwrite") {
      if (FTy->getNumParams() != 4 || !FTy->getParamType(1)->isPointerTy())
        return;
      // May throw; these are valid pthread cancellation points.
      setDoesNotCapture(F, 2);
    } else if (Name == "putchar") {
      setDoesNotThrow(F);
    } else if (Name == "popen") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "pclose") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'v':
    if (Name == "vscanf") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "vsscanf" ||
               Name == "vfscanf") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(1)->isPointerTy() ||
          !FTy->getParamType(2)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "valloc") {
      if (!FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
    } else if (Name == "vprintf") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "vfprintf" ||
               Name == "vsprintf") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "vsnprintf") {
      if (FTy->getNumParams() != 4 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(2)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 3);
    }
    break;
  case 'o':
    if (Name == "open") {
      if (FTy->getNumParams() < 2 || !FTy->getParamType(0)->isPointerTy())
        return;
      // May throw; "open" is a valid pthread cancellation point.
      setDoesNotCapture(F, 1);
    } else if (Name == "opendir") {
      if (FTy->getNumParams() != 1 ||
          !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
    }
    break;
  case 't':
    if (Name == "tmpfile") {
      if (!FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
    } else if (Name == "times") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'h':
    if (Name == "htonl" ||
        Name == "htons") {
      setDoesNotThrow(F);
      setDoesNotAccessMemory(F);
    }
    break;
  case 'n':
    if (Name == "ntohl" ||
        Name == "ntohs") {
      setDoesNotThrow(F);
      setDoesNotAccessMemory(F);
    }
    break;
  case 'l':
    if (Name == "lstat") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "lchown") {
      if (FTy->getNumParams() != 3 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    }
    break;
  case 'q':
    if (Name == "qsort") {
      if (FTy->getNumParams() != 4 || !FTy->getParamType(3)->isPointerTy())
        return;
      // May throw; places call through function pointer.
      setDoesNotCapture(F, 4);
    }
    break;
  case '_':
    if (Name == "__strdup" ||
        Name == "__strndup") {
      if (FTy->getNumParams() < 1 ||
          !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
    } else if (Name == "__strtok_r") {
      if (FTy->getNumParams() != 3 ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "_IO_getc") {
      if (FTy->getNumParams() != 1 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "_IO_putc") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    }
    break;
  case 1:
    if (Name == "\1__isoc99_scanf") {
      if (FTy->getNumParams() < 1 ||
          !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "\1stat64" ||
               Name == "\1lstat64" ||
               Name == "\1statvfs64" ||
               Name == "\1__isoc99_sscanf") {
      if (FTy->getNumParams() < 1 ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "\1fopen64") {
      if (FTy->getNumParams() != 2 ||
          !FTy->getReturnType()->isPointerTy() ||
          !FTy->getParamType(0)->isPointerTy() ||
          !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
      setDoesNotCapture(F, 1);
      setDoesNotCapture(F, 2);
    } else if (Name == "\1fseeko64" ||
               Name == "\1ftello64") {
      if (FTy->getNumParams() == 0 || !FTy->getParamType(0)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 1);
    } else if (Name == "\1tmpfile64") {
      if (!FTy->getReturnType()->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotAlias(F, 0);
    } else if (Name == "\1fstat64" ||
               Name == "\1fstatvfs64") {
      if (FTy->getNumParams() != 2 || !FTy->getParamType(1)->isPointerTy())
        return;
      setDoesNotThrow(F);
      setDoesNotCapture(F, 2);
    } else if (Name == "\1open64") {
      if (FTy->getNumParams() < 2 || !FTy->getParamType(0)->isPointerTy())
        return;
      // May throw; "open" is a valid pthread cancellation point.
      setDoesNotCapture(F, 1);
    }
    break;
  }
}

/// doInitialization - Add attributes to well-known functions.
///
bool SimplifyLibCalls::doInitialization(Module &M) {
  Modified = false;
  for (Module::iterator I = M.begin(), E = M.end(); I != E; ++I) {
    Function &F = *I;
    if (F.isDeclaration() && F.hasName())
      inferPrototypeAttributes(F);
  }
  return Modified;
}

// TODO:
//   Additional cases that we need to add to this file:
//
// cbrt:
//   * cbrt(expN(X))  -> expN(x/3)
//   * cbrt(sqrt(x))  -> pow(x,1/6)
//   * cbrt(sqrt(x))  -> pow(x,1/9)
//
// exp, expf, expl:
//   * exp(log(x))  -> x
//
// log, logf, logl:
//   * log(exp(x))   -> x
//   * log(x**y)     -> y*log(x)
//   * log(exp(y))   -> y*log(e)
//   * log(exp2(y))  -> y*log(2)
//   * log(exp10(y)) -> y*log(10)
//   * log(sqrt(x))  -> 0.5*log(x)
//   * log(pow(x,y)) -> y*log(x)
//
// lround, lroundf, lroundl:
//   * lround(cnst) -> cnst'
//
// pow, powf, powl:
//   * pow(exp(x),y)  -> exp(x*y)
//   * pow(sqrt(x),y) -> pow(x,y*0.5)
//   * pow(pow(x,y),z)-> pow(x,y*z)
//
// round, roundf, roundl:
//   * round(cnst) -> cnst'
//
// signbit:
//   * signbit(cnst) -> cnst'
//   * signbit(nncst) -> 0 (if pstv is a non-negative constant)
//
// sqrt, sqrtf, sqrtl:
//   * sqrt(expN(x))  -> expN(x*0.5)
//   * sqrt(Nroot(x)) -> pow(x,1/(2*N))
//   * sqrt(pow(x,y)) -> pow(|x|,y*0.5)
//
// strchr:
//   * strchr(p, 0) -> strlen(p)
// tan, tanf, tanl:
//   * tan(atan(x)) -> x
//
// trunc, truncf, truncl:
//   * trunc(cnst) -> cnst'
//
//
