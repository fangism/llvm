THIS IS A CLONE OF THE OFFICIAL LLVM PROJECT REPOSITORY (http://llvm.org/).
The master branch of this repository tracks the official git mirror's master,
and new work is done on branches, such as 'powerpc-darwin8' and 'powerpc-darwin8-C++11'.

powerpc-darwin8* branch status: http://www.csl.cornell.edu/~fang/sw/llvm/

================================
Instructions
================================

You are on the powerpc-darwin8-C++11 branch, which requires a c++11-capable host compiler.
Recommended compilers: clang-3.4 (powerpc-darwin8-rel-3.4 branch) or FSF gcc-4.8.
Both of these can be obtained through fink, for example (gcc48, clang34).  
You also need a C++11 standard library such as libc++ (fink: libcxx1-dev).
Whichever compiler you choose, pass it -DCMAKE-CXX_COMPILER=.

Example cmake parameters (using fink's clang-3.4, libc++):
    -DCMAKE_BUILD_TYPE=
    -DBUILD_SHARED_LIBS=ON
    -DCMAKE_CXX_FLAGS:STRING="-O0 -std=c++11 -stdlib=libc++ -cxx-isystem /sw/include/c++/v1 -L/sw/lib/c++ -B/sw/lib/odcctools/bin -no-integrated-as -Qunused-arguments"
    -DCMAKE_C_FLAGS:STRING="-O0 -B/sw/lib/odcctools/bin -no-integrated-as -Qunused-arguments"
    -DCMAKE_EXE_LINKER_FLAGS:STRING=-L/sw/lib/c++
    -DCMAKE_MODULE_LINKER_FLAGS:STRING=-L/sw/lib/c++
    -DCMAKE_SHARED_LINKER_FLAGS:STRING=-L/sw/lib/c++
    -DLLVM_ENABLE_ASSERTIONS:BOOL=ON

================================
Low Level Virtual Machine (LLVM)
================================

This directory and its subdirectories contain source code for LLVM,
a toolkit for the construction of highly optimized compilers,
optimizers, and runtime environments.

LLVM is open source software. You may freely distribute it under the terms of
the license agreement found in LICENSE.txt.

Please see the documentation provided in docs/ for further
assistance with LLVM, and in particular docs/GettingStarted.rst for getting
started with LLVM and docs/README.txt for an overview of LLVM's
documentation setup.

If you're writing a package for LLVM, see docs/Packaging.rst for our
suggestions.
