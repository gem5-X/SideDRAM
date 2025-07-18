################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/microcode/base_format.cpp \
../src/microcode/encoded_shift_format.cpp 

CPP_DEPS += \
./src/microcode/base_format.d \
./src/microcode/encoded_shift_format.d 

OBJS += \
./src/microcode/base_format.o \
./src/microcode/encoded_shift_format.o 


# Each subdirectory must supply rules for building sources it contributes
src/microcode/%.o: ../src/microcode/%.cpp src/microcode/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I/opt/systemc/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++17 -DSC_ALLOW_DEPRECATED_IEEE_API -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-microcode

clean-src-2f-microcode:
	-$(RM) ./src/microcode/base_format.d ./src/microcode/base_format.o ./src/microcode/encoded_shift_format.d ./src/microcode/encoded_shift_format.o

.PHONY: clean-src-2f-microcode

