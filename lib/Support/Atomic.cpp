//===-- Atomic.cpp - Atomic Operations --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
//  This header file implements atomic operations.
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/Atomic.h"
#include "llvm/Config/llvm-config.h"

using namespace llvm;

#if defined(_MSC_VER)
#include <windows.h>
#undef MemoryFence
#endif

#if defined(__APPLE__) && 0
#include <libkern/OSAtomic.h>
// __APPLE__ should take precedence over __GNUC__
// sys::cas_flag is int32_t from Support/Atomic.h, so use '32' variants
// prototypes lack the 'volatile' qualifier, so we need to cast them away
template <class T>
static inline
T* vcast(volatile T* ptr) { return const_cast<T*>(ptr); }
#endif

#if defined(__GNUC__) || (defined(__IBMCPP__) && __IBMCPP__ >= 1210)
#define GNU_ATOMICS
#endif

void sys::MemoryFence() {
#if LLVM_HAS_ATOMICS == 0
  return;
/**
#elif defined(__APPLE__)
  OSMemoryBarrier();
**/
#elif defined(GNU_ATOMICS)
  __sync_synchronize();
#elif defined(_MSC_VER)
  MemoryBarrier();
#else
# error No memory fence implementation for your platform!
#endif
}

sys::cas_flag sys::CompareAndSwap(volatile sys::cas_flag* ptr,
                                  sys::cas_flag new_value,
                                  sys::cas_flag old_value) {
#if LLVM_HAS_ATOMICS == 0
  sys::cas_flag result = *ptr;
  if (result == old_value)
    *ptr = new_value;
  return result;
/**
#elif defined(__APPLE__)
  return OSAtomicCompareAndSwap32(old_value, new_value, vcast(ptr));
**/
#elif defined(GNU_ATOMICS)
  return __sync_val_compare_and_swap(ptr, old_value, new_value);
#elif defined(_MSC_VER)
  return InterlockedCompareExchange(ptr, new_value, old_value);
#else
#  error No compare-and-swap implementation for your platform!
#endif
}

sys::cas_flag sys::AtomicIncrement(volatile sys::cas_flag* ptr) {
#if LLVM_HAS_ATOMICS == 0
  ++(*ptr);
  return *ptr;
/**
#elif defined(__APPLE__)
  return OSAtomicIncrement32(vcast(ptr));
**/
#elif defined(GNU_ATOMICS)
  return __sync_add_and_fetch(ptr, 1);
#elif defined(_MSC_VER)
  return InterlockedIncrement(ptr);
#else
#  error No atomic increment implementation for your platform!
#endif
}

sys::cas_flag sys::AtomicDecrement(volatile sys::cas_flag* ptr) {
#if LLVM_HAS_ATOMICS == 0
  --(*ptr);
  return *ptr;
/**
#elif defined(__APPLE__)
  return OSAtomicDecrement32(vcast(ptr));
**/
#elif defined(GNU_ATOMICS)
  return __sync_sub_and_fetch(ptr, 1);
#elif defined(_MSC_VER)
  return InterlockedDecrement(ptr);
#else
#  error No atomic decrement implementation for your platform!
#endif
}

sys::cas_flag sys::AtomicAdd(volatile sys::cas_flag* ptr, sys::cas_flag val) {
#if LLVM_HAS_ATOMICS == 0
  *ptr += val;
  return *ptr;
/**
#elif defined(__APPLE__)
  return OSAtomicAdd32(val, vcast(ptr));
**/
#elif defined(GNU_ATOMICS)
  return __sync_add_and_fetch(ptr, val);
#elif defined(_MSC_VER)
  return InterlockedExchangeAdd(ptr, val) + val;
#else
#  error No atomic add implementation for your platform!
#endif
}

sys::cas_flag sys::AtomicMul(volatile sys::cas_flag* ptr, sys::cas_flag val) {
  sys::cas_flag original, result;
  do {
    original = *ptr;
    result = original * val;
  } while (sys::CompareAndSwap(ptr, result, original) != original);

  return result;
}

sys::cas_flag sys::AtomicDiv(volatile sys::cas_flag* ptr, sys::cas_flag val) {
  sys::cas_flag original, result;
  do {
    original = *ptr;
    result = original / val;
  } while (sys::CompareAndSwap(ptr, result, original) != original);

  return result;
}
