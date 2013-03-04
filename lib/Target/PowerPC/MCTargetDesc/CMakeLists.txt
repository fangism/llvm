add_llvm_library(LLVMPowerPCDesc
  PPCAsmBackend.cpp
  PPCMCTargetDesc.cpp
  PPCMCAsmInfo.cpp
  PPCMCCodeEmitter.cpp
  PPCPredicates.cpp
  PPCMachObjectWriter.cpp
  PPCELFObjectWriter.cpp
  )

add_dependencies(LLVMPowerPCDesc PowerPCCommonTableGen)
