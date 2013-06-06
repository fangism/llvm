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

// USE_DARWIN_ATOMICS conditionally defined in Atomics.h
#if USE_DARWIN_ATOMICS
#include <libkern/OSAtomic.h>
// __APPLE__ should take precedence over __GNUC__
// sys::cas_flag is int32_t from Support/Atomic.h, so use '32' variants
// prototypes lack the 'volatile' qualifier, so we need to cast them away
template <class T>
static inline
T* vcast(volatile T* ptr) { return const_cast<T*>(ptr); }

// note on weakly-ordered architectures (PPC):
/**
DESCRIPTION
     These functions are thread and multiprocessor safe.  For each function,
     there is a version that does and another that does not incorporate a
     memory barrier.  Barriers strictly order memory access on a weakly-
     ordered architecture such as PPC.  All loads and stores executed in
     sequential program order before the barrier will complete before any load
     or store executed after the barrier.  On a uniprocessor, the barrier
     operation is typically a nop.  On a multiprocessor, the barrier can be
     quite expensive.

     Most code will want to use the barrier functions to insure that memory
     shared between threads is properly synchronized.  For example, if you
     want to initialize a shared data structure and then atomically increment
     a variable to indicate that the initialization is complete, then you MUST
     use OSAtomicIncrement32Barrier() to ensure that the stores to your data
     structure complete before the atomic add.  Likewise, the consumer of that
     data structure MUST use OSAtomicDecrement32Barrier(), in order to ensure
     that their loads of the structure are not executed before the atomic
     decrement.  On the other hand, if you are simply incrementing a global
     counter, then it is safe and potentially much faster to use OSAtomicIn-
     crement32().  If you are unsure which version to use, prefer the barrier
     variants as they are safer.

RETURN VALUES
     The arithmetic and logical operations return the new value, after the
     operation has been performed.  The compare-and-swap operations return
     true if the comparison was equal, ie if the swap occured.  The bit test
     and set/clear operations return the original value of the bit.

	-- man 3 atomic (BSD Library Functions Manual)
**/
#endif

#if defined(__GNUC__) || (defined(__IBMCPP__) && __IBMCPP__ >= 1210)
#if !USE_DARWIN_ATOMICS
#define GNU_ATOMICS
#endif
#endif

void sys::MemoryFence() {
#if LLVM_HAS_ATOMICS == 0
  return;
#elif defined(GNU_ATOMICS)
  __sync_synchronize();
#elif USE_DARWIN_ATOMICS
  OSMemoryBarrier();
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
#elif defined(GNU_ATOMICS)
  return __sync_val_compare_and_swap(ptr, old_value, new_value);
/**
These builtins perform an atomic compare and swap.
That is, if the current value of *ptr is oldval, then write newval into *ptr.
The bool version returns true if the comparison is successful and newval 
was written. The val version returns the contents of *ptr before the operation. 
	-- http://gcc.gnu.org/onlinedocs/gcc-4.1.1/gcc/Atomic-Builtins.html
**/
#elif USE_DARWIN_ATOMICS
  const sys::cas_flag prev = *ptr;
  // returns new value, but we don't want it
  OSAtomicCompareAndSwap32Barrier(old_value, new_value, vcast(ptr));
  return prev;		// return the previous value at *ptr
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
#elif defined(GNU_ATOMICS)
  return __sync_add_and_fetch(ptr, 1);
#elif USE_DARWIN_ATOMICS
  return OSAtomicIncrement32Barrier(vcast(ptr));	// return new value
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
#elif defined(GNU_ATOMICS)
  return __sync_sub_and_fetch(ptr, 1);
#elif USE_DARWIN_ATOMICS
  return OSAtomicDecrement32Barrier(vcast(ptr));	// return new value
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
#elif defined(GNU_ATOMICS)
  return __sync_add_and_fetch(ptr, val);
#elif USE_DARWIN_ATOMICS
  return OSAtomicAdd32Barrier(val, vcast(ptr));		// return new value
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
