//===- verify-uselistorder.cpp - The LLVM Modular Optimizer ---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Verify that use-list order can be serialized correctly.  After reading the
// provided IR, this tool shuffles the use-lists and then writes and reads to a
// separate Module whose use-list orders are compared to the original.
//
// The shuffles are deterministic and somewhat naive.  On a given shuffle, some
// use-lists will not change at all.  The algorithm per iteration is as follows:
//
//  1. Seed the random number generator.  The seed is different for each
//     shuffle.  Shuffle 0 uses default+0, shuffle 1 uses default+1, and so on.
//
//  2. Visit every Value in a deterministic order.
//
//  3. Assign a random number to each Use in the Value's use-list in order.
//
//  4. Sort the use-list using Value::sortUseList(), which is a stable sort.
//
// Shuffling a larger number of times provides a better statistical guarantee
// that each use-list has changed at least once.
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/DenseMap.h"
#include "llvm/AsmParser/Parser.h"
#include "llvm/Bitcode/ReaderWriter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/UseListOrder.h"
#include "llvm/IRReader/IRReader.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/FileUtilities.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/PrettyStackTrace.h"
#include "llvm/Support/Signals.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/Support/SystemUtils.h"

using namespace llvm;

#define DEBUG_TYPE "use-list-order"

static cl::opt<std::string> InputFilename(cl::Positional,
                                          cl::desc("<input bitcode file>"),
                                          cl::init("-"),
                                          cl::value_desc("filename"));

static cl::opt<bool> SaveTemps("save-temps", cl::desc("Save temp files"),
                               cl::init(false));

static cl::opt<unsigned>
    NumShuffles("num-shuffles",
                cl::desc("Number of times to shuffle and verify use-lists"),
                cl::init(5));

namespace {

struct TempFile {
  std::string Filename;
  FileRemover Remover;
  bool init(const std::string &Ext);
  bool writeBitcode(const Module &M) const;
  bool writeAssembly(const Module &M) const;
  std::unique_ptr<Module> readBitcode(LLVMContext &Context) const;
  std::unique_ptr<Module> readAssembly(LLVMContext &Context) const;
};

struct ValueMapping {
  DenseMap<const Value *, unsigned> IDs;
  std::vector<const Value *> Values;

  /// \brief Construct a value mapping for module.
  ///
  /// Creates mapping from every value in \c M to an ID.  This mapping includes
  /// un-referencable values.
  ///
  /// Every \a Value that gets serialized in some way should be represented
  /// here.  The order needs to be deterministic, but it's unnecessary to match
  /// the value-ids in the bitcode writer.
  ///
  /// All constants that are referenced by other values are included in the
  /// mapping, but others -- which wouldn't be serialized -- are not.
  ValueMapping(const Module &M);

  /// \brief Map a value.
  ///
  /// Maps a value.  If it's a constant, maps all of its operands first.
  void map(const Value *V);
  unsigned lookup(const Value *V) const { return IDs.lookup(V); }
};

} // end namespace

bool TempFile::init(const std::string &Ext) {
  SmallVector<char, 64> Vector;
  DEBUG(dbgs() << " - create-temp-file\n");
  if (auto EC = sys::fs::createTemporaryFile("use-list-order", Ext, Vector)) {
    (void)EC;
    DEBUG(dbgs() << "error: " << EC.message() << "\n");
    return true;
  }
  assert(!Vector.empty());

  Filename.assign(Vector.data(), Vector.data() + Vector.size());
  Remover.setFile(Filename, !SaveTemps);
  DEBUG(dbgs() << " - filename = " << Filename << "\n");
  return false;
}

bool TempFile::writeBitcode(const Module &M) const {
  DEBUG(dbgs() << " - write bitcode\n");
  std::string ErrorInfo;
  raw_fd_ostream OS(Filename.c_str(), ErrorInfo, sys::fs::F_None);
  if (!ErrorInfo.empty()) {
    DEBUG(dbgs() << "error: " << ErrorInfo << "\n");
    return true;
  }

  WriteBitcodeToFile(&M, OS);
  return false;
}

bool TempFile::writeAssembly(const Module &M) const {
  DEBUG(dbgs() << " - write assembly\n");
  std::string ErrorInfo;
  raw_fd_ostream OS(Filename.c_str(), ErrorInfo, sys::fs::F_Text);
  if (!ErrorInfo.empty()) {
    DEBUG(dbgs() << "error: " << ErrorInfo << "\n");
    return true;
  }

  OS << M;
  return false;
}

std::unique_ptr<Module> TempFile::readBitcode(LLVMContext &Context) const {
  DEBUG(dbgs() << " - read bitcode\n");
  ErrorOr<std::unique_ptr<MemoryBuffer>> BufferOr =
      MemoryBuffer::getFile(Filename);
  if (!BufferOr) {
    DEBUG(dbgs() << "error: " << BufferOr.getError().message() << "\n");
    return nullptr;
  }

  std::unique_ptr<MemoryBuffer> Buffer = std::move(BufferOr.get());
  ErrorOr<Module *> ModuleOr = parseBitcodeFile(Buffer.get(), Context);
  if (!ModuleOr) {
    DEBUG(dbgs() << "error: " << ModuleOr.getError().message() << "\n");
    return nullptr;
  }
  return std::unique_ptr<Module>(ModuleOr.get());
}

std::unique_ptr<Module> TempFile::readAssembly(LLVMContext &Context) const {
  DEBUG(dbgs() << " - read assembly\n");
  SMDiagnostic Err;
  std::unique_ptr<Module> M(ParseAssemblyFile(Filename, Err, Context));
  if (!M.get())
    DEBUG(dbgs() << "error: "; Err.print("verify-use-list-order", dbgs()));
  return M;
}

ValueMapping::ValueMapping(const Module &M) {
  // Every value should be mapped, including things like void instructions and
  // basic blocks that are kept out of the ValueEnumerator.
  //
  // The current mapping order makes it easier to debug the tables.  It happens
  // to be similar to the ID mapping when writing ValueEnumerator, but they
  // aren't (and needn't be) in sync.

  // Globals.
  for (const GlobalVariable &G : M.globals())
    map(&G);
  for (const GlobalAlias &A : M.aliases())
    map(&A);
  for (const Function &F : M)
    map(&F);

  // Constants used by globals.
  for (const GlobalVariable &G : M.globals())
    if (G.hasInitializer())
      map(G.getInitializer());
  for (const GlobalAlias &A : M.aliases())
    map(A.getAliasee());
  for (const Function &F : M)
    if (F.hasPrefixData())
      map(F.getPrefixData());

  // Function bodies.
  for (const Function &F : M) {
    for (const Argument &A : F.args())
      map(&A);
    for (const BasicBlock &BB : F)
      map(&BB);
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB)
        map(&I);

    // Constants used by instructions.
    for (const BasicBlock &BB : F)
      for (const Instruction &I : BB)
        for (const Value *Op : I.operands())
          if ((isa<Constant>(Op) && !isa<GlobalValue>(*Op)) ||
              isa<InlineAsm>(Op))
            map(Op);
  }
}

void ValueMapping::map(const Value *V) {
  if (IDs.lookup(V))
    return;

  if (auto *C = dyn_cast<Constant>(V))
    if (!isa<GlobalValue>(C))
      for (const Value *Op : C->operands())
        map(Op);

  Values.push_back(V);
  IDs[V] = Values.size();
}

#ifndef NDEBUG
static void dumpMapping(const ValueMapping &VM) {
  dbgs() << "value-mapping (size = " << VM.Values.size() << "):\n";
  for (unsigned I = 0, E = VM.Values.size(); I != E; ++I) {
    dbgs() << " - id = " << I << ", value = ";
    VM.Values[I]->dump();
  }
}

static void debugValue(const ValueMapping &M, unsigned I, StringRef Desc) {
  const Value *V = M.Values[I];
  dbgs() << " - " << Desc << " value = ";
  V->dump();
  for (const Use &U : V->uses()) {
    dbgs() << "   => use: op = " << U.getOperandNo()
           << ", user-id = " << M.IDs.lookup(U.getUser()) << ", user = ";
    U.getUser()->dump();
  }
}

static void debugUserMismatch(const ValueMapping &L, const ValueMapping &R,
                              unsigned I) {
  dbgs() << " - fail: user mismatch: ID = " << I << "\n";
  debugValue(L, I, "LHS");
  debugValue(R, I, "RHS");

  dbgs() << "\nlhs-";
  dumpMapping(L);
  dbgs() << "\nrhs-";
  dumpMapping(R);
}

static void debugSizeMismatch(const ValueMapping &L, const ValueMapping &R) {
  dbgs() << " - fail: map size: " << L.Values.size()
         << " != " << R.Values.size() << "\n";
  dbgs() << "\nlhs-";
  dumpMapping(L);
  dbgs() << "\nrhs-";
  dumpMapping(R);
}
#endif

static bool matches(const ValueMapping &LM, const ValueMapping &RM) {
  DEBUG(dbgs() << "compare value maps\n");
  if (LM.Values.size() != RM.Values.size()) {
    DEBUG(debugSizeMismatch(LM, RM));
    return false;
  }

  // This mapping doesn't include dangling constant users, since those don't
  // get serialized.  However, checking if users are constant and calling
  // isConstantUsed() on every one is very expensive.  Instead, just check if
  // the user is mapped.
  auto skipUnmappedUsers =
      [&](Value::const_use_iterator &U, Value::const_use_iterator E,
          const ValueMapping &M) {
    while (U != E && !M.lookup(U->getUser()))
      ++U;
  };

  // Iterate through all values, and check that both mappings have the same
  // users.
  for (unsigned I = 0, E = LM.Values.size(); I != E; ++I) {
    const Value *L = LM.Values[I];
    const Value *R = RM.Values[I];
    auto LU = L->use_begin(), LE = L->use_end();
    auto RU = R->use_begin(), RE = R->use_end();
    skipUnmappedUsers(LU, LE, LM);
    skipUnmappedUsers(RU, RE, RM);

    while (LU != LE) {
      if (RU == RE) {
        DEBUG(debugUserMismatch(LM, RM, I));
        return false;
      }
      if (LM.lookup(LU->getUser()) != RM.lookup(RU->getUser())) {
        DEBUG(debugUserMismatch(LM, RM, I));
        return false;
      }
      if (LU->getOperandNo() != RU->getOperandNo()) {
        DEBUG(debugUserMismatch(LM, RM, I));
        return false;
      }
      skipUnmappedUsers(++LU, LE, LM);
      skipUnmappedUsers(++RU, RE, RM);
    }
    if (RU != RE) {
      DEBUG(debugUserMismatch(LM, RM, I));
      return false;
    }
  }

  return true;
}

static bool verifyBitcodeUseListOrder(const Module &M) {
  DEBUG(dbgs() << "*** verify-use-list-order: bitcode ***\n");
  TempFile F;
  if (F.init("bc"))
    return false;

  if (F.writeBitcode(M))
    return false;

  LLVMContext Context;
  std::unique_ptr<Module> OtherM = F.readBitcode(Context);
  if (!OtherM)
    return false;

  return matches(ValueMapping(M), ValueMapping(*OtherM));
}

static bool verifyAssemblyUseListOrder(const Module &M) {
  DEBUG(dbgs() << "*** verify-use-list-order: assembly ***\n");
  TempFile F;
  if (F.init("ll"))
    return false;

  if (F.writeAssembly(M))
    return false;

  LLVMContext Context;
  std::unique_ptr<Module> OtherM = F.readAssembly(Context);
  if (!OtherM)
    return false;

  return matches(ValueMapping(M), ValueMapping(*OtherM));
}

int main(int argc, char **argv) {
  sys::PrintStackTraceOnErrorSignal();
  llvm::PrettyStackTraceProgram X(argc, argv);

  // Enable debug stream buffering.
  EnableDebugBuffering = true;

  llvm_shutdown_obj Y; // Call llvm_shutdown() on exit.
  LLVMContext &Context = getGlobalContext();

  cl::ParseCommandLineOptions(argc, argv,
                              "llvm tool to verify use-list order\n");

  SMDiagnostic Err;

  // Load the input module...
  std::unique_ptr<Module> M;
  M.reset(ParseIRFile(InputFilename, Err, Context));

  if (!M.get()) {
    Err.print(argv[0], errs());
    return 1;
  }

  DEBUG(dbgs() << "*** verify-use-list-order ***\n");
  if (!shouldPreserveBitcodeUseListOrder()) {
    // Can't verify if order isn't preserved.
    DEBUG(dbgs() << "warning: cannot verify bitcode; "
                    "try -preserve-bc-use-list-order\n");
    return 0;
  }

  for (unsigned I = 0, E = NumShuffles; I != E; ++I) {
    DEBUG(dbgs() << "*** iteration: " << I << " ***\n");

    // Shuffle with a different seed each time so that use-lists that aren't
    // modified the first time are likely to be modified the next time.
    shuffleUseLists(*M, I);
    if (!verifyBitcodeUseListOrder(*M))
      report_fatal_error("bitcode use-list order changed");

    if (shouldPreserveAssemblyUseListOrder())
      if (!verifyAssemblyUseListOrder(*M))
        report_fatal_error("assembly use-list order changed");
  }

  return 0;
}
