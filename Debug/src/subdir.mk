################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/add_sub.cpp \
../src/control_plane.cpp \
../src/control_unit.cpp \
../src/data_pack.cpp \
../src/data_pack_synth.cpp \
../src/imc_pch.cpp \
../src/instruction_decoder_mov.cpp \
../src/instruction_decoder_pack_mask.cpp \
../src/instruction_decoder_shift_add.cpp \
../src/interface_unit.cpp \
../src/mask_unit.cpp \
../src/mult_sequencer.cpp \
../src/opcodes.cpp \
../src/pack_and_mask.cpp \
../src/pc_unit.cpp \
../src/sc_functions.cpp \
../src/shift_and_add.cpp \
../src/softsimd_pu.cpp \
../src/softsimd_pu_cu_test.cpp \
../src/tile_shuffler.cpp 

CPP_DEPS += \
./src/add_sub.d \
./src/control_plane.d \
./src/control_unit.d \
./src/data_pack.d \
./src/data_pack_synth.d \
./src/imc_pch.d \
./src/instruction_decoder_mov.d \
./src/instruction_decoder_pack_mask.d \
./src/instruction_decoder_shift_add.d \
./src/interface_unit.d \
./src/mask_unit.d \
./src/mult_sequencer.d \
./src/opcodes.d \
./src/pack_and_mask.d \
./src/pc_unit.d \
./src/sc_functions.d \
./src/shift_and_add.d \
./src/softsimd_pu.d \
./src/softsimd_pu_cu_test.d \
./src/tile_shuffler.d 

OBJS += \
./src/add_sub.o \
./src/control_plane.o \
./src/control_unit.o \
./src/data_pack.o \
./src/data_pack_synth.o \
./src/imc_pch.o \
./src/instruction_decoder_mov.o \
./src/instruction_decoder_pack_mask.o \
./src/instruction_decoder_shift_add.o \
./src/interface_unit.o \
./src/mask_unit.o \
./src/mult_sequencer.o \
./src/opcodes.o \
./src/pack_and_mask.o \
./src/pc_unit.o \
./src/sc_functions.o \
./src/shift_and_add.o \
./src/softsimd_pu.o \
./src/softsimd_pu_cu_test.o \
./src/tile_shuffler.o 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.cpp src/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I$(SYSTEMC_HOME)/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++17 -DSC_ALLOW_DEPRECATED_IEEE_API -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src

clean-src:
	-$(RM) ./src/add_sub.d ./src/add_sub.o ./src/control_plane.d ./src/control_plane.o ./src/control_unit.d ./src/control_unit.o ./src/data_pack.d ./src/data_pack.o ./src/data_pack_synth.d ./src/data_pack_synth.o ./src/imc_pch.d ./src/imc_pch.o ./src/instruction_decoder_mov.d ./src/instruction_decoder_mov.o ./src/instruction_decoder_pack_mask.d ./src/instruction_decoder_pack_mask.o ./src/instruction_decoder_shift_add.d ./src/instruction_decoder_shift_add.o ./src/interface_unit.d ./src/interface_unit.o ./src/mask_unit.d ./src/mask_unit.o ./src/mult_sequencer.d ./src/mult_sequencer.o ./src/opcodes.d ./src/opcodes.o ./src/pack_and_mask.d ./src/pack_and_mask.o ./src/pc_unit.d ./src/pc_unit.o ./src/sc_functions.d ./src/sc_functions.o ./src/shift_and_add.d ./src/shift_and_add.o ./src/softsimd_pu.d ./src/softsimd_pu.o ./src/softsimd_pu_cu_test.d ./src/softsimd_pu_cu_test.o ./src/tile_shuffler.d ./src/tile_shuffler.o

.PHONY: clean-src

