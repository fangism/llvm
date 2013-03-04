add_llvm_library(LLVMMCDisassembler
  Disassembler.cpp
  )

target_link_libraries(LLVMMCDisassembler
  LLVMMC
  LLVMMCParser
  LLVMSupport
  LLVMTarget
  )

foreach(t ${LLVM_TARGETS_TO_BUILD})
  set(td ${LLVM_MAIN_SRC_DIR}/lib/Target/${t})
  if(EXISTS ${td}/TargetInfo/CMakeLists.txt)
    target_link_libraries(LLVMMCDisassembler "LLVM${t}Info")
  endif()
  if(EXISTS ${td}/MCTargetDesc/CMakeLists.txt)
    target_link_libraries(LLVMMCDisassembler "LLVM${t}Desc")
  endif()
  if(EXISTS ${td}/AsmParser/CMakeLists.txt)
    target_link_libraries(LLVMMCDisassembler "LLVM${t}AsmParser")
  endif()
  if(EXISTS ${td}/Disassembler/CMakeLists.txt)
    target_link_libraries(LLVMMCDisassembler "LLVM${t}Disassembler")
  endif()
endforeach(t)

