#!/bin/bash

# Needs to be used in the root directory of the project with source export_paths.sh
export SIDEDRAM_HOME=$PWD
export SYSTEMC_HOME=$SIDEDRAM_HOME/systemc
export INPUTS_DIR=$SIDEDRAM_HOME/inputs
export RAMULATOR_DIR=$SIDEDRAM_HOME/ramulator
export LD_LIBRARY_PATH=$SYSTEMC_HOME/lib:$LD_LIBRARY_PATH

echo "SIDEDRAM_HOME: $SIDEDRAM_HOME"

INPUTS_SED=$(echo $INPUTS_DIR | sed 's/\//\\\//g')

# Substitute INPUTS_DIR with $INPUTS_DIR in the needed files
sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/inputs/src/gen_gemm_assembly.cpp
sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/inputs/src/map_kernel.cpp
sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/src/control_unit.cpp
sed -i "s/INPUTS_DIR/$INPUTS_SED/g" $SIDEDRAM_HOME/src/tb/pch_driver.cpp