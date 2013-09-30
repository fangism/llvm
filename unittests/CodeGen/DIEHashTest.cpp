//===- llvm/unittest/DebugInfo/DWARFFormValueTest.cpp ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "../lib/CodeGen/AsmPrinter/DIE.h"
#include "../lib/CodeGen/AsmPrinter/DIEHash.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Format.h"
#include "gtest/gtest.h"

namespace {

using namespace llvm;
TEST(DIEHashData1Test, DIEHash) {
  DIEHash Hash;
  DIE Die(dwarf::DW_TAG_base_type);
  DIEInteger Size(4);
  Die.addValue(dwarf::DW_AT_byte_size, dwarf::DW_FORM_data1, &Size);
  uint64_t MD5Res = Hash.computeTypeSignature(&Die);
  ASSERT_TRUE(MD5Res == 0x540e9ff30ade3e4aULL);
}
}
