# patch: this should be built as a bundle/module/plug-in
set(MODULE TRUE)
add_llvm_loadable_module( LLVMHello
  Hello.cpp
  )
