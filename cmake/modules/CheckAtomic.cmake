# atomic builtins are required for threading support.

INCLUDE(CheckCXXSourceCompiles)

check_function_exists(__atomic_fetch_add_4 HAVE___ATOMIC_FETCH_ADD_4)
if( NOT HAVE___ATOMIC_FETCH_ADD_4 )
  check_library_exists(atomic __atomic_fetch_add_4 "" HAVE_LIBATOMIC)
  set(HAVE_LIBATOMIC False)
  if( HAVE_LIBATOMIC )
    list(APPEND CMAKE_REQUIRED_LIBRARIES "atomic")
  endif()
endif()

CHECK_CXX_SOURCE_COMPILES("
#ifdef _MSC_VER
#include <Intrin.h> /* Workaround for PR19898. */
#include <windows.h>
#endif
#define	NEED_DARWIN_ATOMICS (defined(__APPLE__) && defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 2)))
#if NEED_DARWIN_ATOMICS
#include <libkern/OSAtomic.h>
#endif
int main() {
#ifdef _MSC_VER
        volatile LONG val = 1;
        MemoryBarrier();
        InterlockedCompareExchange(&val, 0, 1);
        InterlockedIncrement(&val);
        InterlockedDecrement(&val);
#elif NEED_DARWIN_ATOMICS
	int32_t val = 1;
	OSMemoryBarrier();
	OSAtomicCompareAndSwap32Barrier(1, 0, &val);
	OSAtomicIncrement32(&val);
	OSAtomicDecrement32(&val);
#else
        volatile unsigned long val = 1;
        __sync_synchronize();
        __sync_val_compare_and_swap(&val, 1, 0);
        __sync_add_and_fetch(&val, 1);
        __sync_sub_and_fetch(&val, 1);
#endif
        return 0;
      }
" LLVM_HAS_ATOMICS)

if( NOT LLVM_HAS_ATOMICS )
  message(STATUS "Warning: LLVM will be built thread-unsafe because atomic builtins are missing")
endif()
