//===-- llvm/Support/FormattedStream.cpp - Formatted streams ----*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains the implementation of default_format_string.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Format.h"

namespace llvm {

#define	FORMAT_DEC(T, fmt) const char default_format_string<T>::dec[] = fmt;
#define	FORMAT_HEX(T, fmt) const char default_format_string<T>::hex[] = fmt;

FORMAT_DEC(int, "%d")
FORMAT_HEX(int, "0x%x")
FORMAT_DEC(long, "%ld")
FORMAT_HEX(long, "0x%lx")
FORMAT_DEC(long long, "%lld")
FORMAT_HEX(long long, "0x%llx")
FORMAT_DEC(unsigned int, "%u")
FORMAT_HEX(unsigned int, "0x%x")
FORMAT_DEC(unsigned long, "%lu")
FORMAT_HEX(unsigned long, "0x%lx")
FORMAT_DEC(unsigned long long, "%llu")
FORMAT_HEX(unsigned long long, "0x%llx")

}	// end namespace llvm
