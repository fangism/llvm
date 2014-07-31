set(BACKEND_LIBS "")
foreach(t ${LLVM_TARGETS_TO_BUILD})
  set(td ${LLVM_MAIN_SRC_DIR}/lib/Target/${t})
  if(EXISTS ${td}/TargetInfo/CMakeLists.txt)
    append(BACKEND_LIBS "LLVM${t}Info")
  endif()
  if(EXISTS ${td}/MCTargetDesc/CMakeLists.txt)
    append(BACKEND_LIBS "LLVM${t}Desc")
  endif()
  if(EXISTS ${td}/AsmParser/CMakeLists.txt)
    append(BACKEND_LIBS "LLVM${t}AsmParser")
  endif()
  if(EXISTS ${td}/Disassembler/CMakeLists.txt)
    append(BACKEND_LIBS "LLVM${t}Disassembler")
  endif()
endforeach(t)

add_llvm_library(LLVMMCDisassembler
  Disassembler.cpp
  MCRelocationInfo.cpp
  MCExternalSymbolizer.cpp
  MCDisassembler.cpp
  )

target_link_libraries(LLVMMCDisassembler
  LLVMMC
  LLVMMCParser
  LLVMSupport
  LLVMTarget
  ${BACKEND_LIBS}
  )
