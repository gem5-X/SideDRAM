#!/bin/bash

# g++ -std=c++17 src/build_addr.cpp ../src/defs.h -o bin/build_addr
g++ -std=c++17 src/decode_results.cpp src/half.hpp ../src/defs.h -o bin/decode_results
# g++ -std=c++17 src/map_kernel.cpp src/map_kernel.h ../src/defs.h ../src/opcodes.h ../src/opcodes.cpp -o bin/map_kernel
g++ -std=c++17 -DSC_ALLOW_DEPRECATED_IEEE_API src/nmc_assembler.cpp src/nmc_assembler.h ../src/defs.h ../src/opcodes.h ../src/opcodes.cpp \
    ../src/microcode/common_format.h ../src/microcode/base_format.h ../src/microcode/base_format.cpp ../src/microcode/base_code.h \
    ../src/microcode/encoded_shift_format.h ../src/microcode/encoded_shift_format.cpp ../src/microcode/encoded_shift_code.h -o bin/nmc_assembler
g++ -std=c++17 -DSC_ALLOW_DEPRECATED_IEEE_API src/gen_gemm_assembly.cpp src/gen_gemm_assembly.h ../src/defs.h ../src/opcodes.h ../src/opcodes.cpp \
    ../src/microcode/common_format.h ../src/microcode/base_format.h ../src/microcode/base_format.cpp ../src/microcode/base_code.h \
    ../src/microcode/encoded_shift_format.h ../src/microcode/encoded_shift_format.cpp ../src/microcode/encoded_shift_code.h -o bin/gen_gemm_assembly -g
g++ -std=c++17 src/ramulator2sc.cpp ../src/defs.h -o bin/ramulator2sc
g++ -std=c++17 src/raw2ramulator.cpp -o bin/raw2ramulator
g++ -std=c++17 src/raw_seq_gen.cpp ../src/defs.h -o bin/raw_seq_gen