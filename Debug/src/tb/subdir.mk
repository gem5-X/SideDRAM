################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../src/tb/cp_driver.cpp \
../src/tb/ms_driver.cpp \
../src/tb/ms_monitor.cpp \
../src/tb/pch_driver.cpp \
../src/tb/pch_driver_mixed.cpp \
../src/tb/pch_main.cpp \
../src/tb/pch_monitor.cpp \
../src/tb/softs_driver.cpp \
../src/tb/softs_monitor.cpp \
../src/tb/softsimd_pu_driver.cpp \
../src/tb/softsimd_pu_monitor.cpp \
../src/tb/vwr_driver.cpp \
../src/tb/vwr_monitor.cpp 

CPP_DEPS += \
./src/tb/cp_driver.d \
./src/tb/ms_driver.d \
./src/tb/ms_monitor.d \
./src/tb/pch_driver.d \
./src/tb/pch_driver_mixed.d \
./src/tb/pch_main.d \
./src/tb/pch_monitor.d \
./src/tb/softs_driver.d \
./src/tb/softs_monitor.d \
./src/tb/softsimd_pu_driver.d \
./src/tb/softsimd_pu_monitor.d \
./src/tb/vwr_driver.d \
./src/tb/vwr_monitor.d 

OBJS += \
./src/tb/cp_driver.o \
./src/tb/ms_driver.o \
./src/tb/ms_monitor.o \
./src/tb/pch_driver.o \
./src/tb/pch_driver_mixed.o \
./src/tb/pch_main.o \
./src/tb/pch_monitor.o \
./src/tb/softs_driver.o \
./src/tb/softs_monitor.o \
./src/tb/softsimd_pu_driver.o \
./src/tb/softsimd_pu_monitor.o \
./src/tb/vwr_driver.o \
./src/tb/vwr_monitor.o 


# Each subdirectory must supply rules for building sources it contributes
src/tb/%.o: ../src/tb/%.cpp src/tb/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -I$(SYSTEMC_HOME)/include -O0 -g3 -Wall -c -fmessage-length=0 -std=c++17 -DSC_ALLOW_DEPRECATED_IEEE_API -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


clean: clean-src-2f-tb

clean-src-2f-tb:
	-$(RM) ./src/tb/cp_driver.d ./src/tb/cp_driver.o ./src/tb/ms_driver.d ./src/tb/ms_driver.o ./src/tb/ms_monitor.d ./src/tb/ms_monitor.o ./src/tb/pch_driver.d ./src/tb/pch_driver.o ./src/tb/pch_driver_mixed.d ./src/tb/pch_driver_mixed.o ./src/tb/pch_main.d ./src/tb/pch_main.o ./src/tb/pch_monitor.d ./src/tb/pch_monitor.o ./src/tb/softs_driver.d ./src/tb/softs_driver.o ./src/tb/softs_monitor.d ./src/tb/softs_monitor.o ./src/tb/softsimd_pu_driver.d ./src/tb/softsimd_pu_driver.o ./src/tb/softsimd_pu_monitor.d ./src/tb/softsimd_pu_monitor.o ./src/tb/vwr_driver.d ./src/tb/vwr_driver.o ./src/tb/vwr_monitor.d ./src/tb/vwr_monitor.o

.PHONY: clean-src-2f-tb

