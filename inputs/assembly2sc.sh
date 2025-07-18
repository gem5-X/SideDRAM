#!/bin/bash

bin/nmc_assembler $INPUTS_DIR/assembly-input/$1.asm $INPUTS_DIR/raw/$1.seq $INPUTS_DIR/data-input/$1.data $INPUTS_DIR/address-input/$1.addr

bin/raw2ramulator $INPUTS_DIR/raw/$1.seq $INPUTS_DIR/ramulator-in/$1.trace

$RAMULATOR_DIR/ramulator $RAMULATOR_DIR/configs/HBM2_AB-config.cfg --mode=dram $INPUTS_DIR/ramulator-in/$1.trace > $INPUTS_DIR/ramulator-out/$1.cmd

bin/ramulator2sc $INPUTS_DIR/raw/$1.seq $INPUTS_DIR/ramulator-out/$1.cmd $INPUTS_DIR/SystemC/$1.sci 1

cd ..
Debug/pim-cores $1
cd inputs

rm assembly-input/$1.asm address-input/$1.addr data-input/$1.data raw/$1.seq ramulator-in/$1.trace ramulator-out/$1.cmd

# ./decode_results results/$1.results
