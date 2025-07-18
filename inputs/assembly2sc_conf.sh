#!/bin/bash

bin/nmc_assembler_PU${2}_IB${3}_VWR${4}_WORD${5} $INPUTS_DIR/assembly-input/$1.asm $INPUTS_DIR/raw/$1.seq $INPUTS_DIR/data-input/$1.data $INPUTS_DIR/address-input/$1.addr

rm $INPUTS_DIR/assembly-input/$1.asm $INPUTS_DIR/address-input/$1.addr $INPUTS_DIR/data-input/$1.data

bin/raw2ramulator_PU${2}_IB${3}_VWR${4}_WORD${5} $INPUTS_DIR/raw/$1.seq $INPUTS_DIR/ramulator-in/$1.trace

$RAMULATOR_DIR/ramulator $RAMULATOR_DIR/configs/HBM2_AB-config.cfg --mode=dram $INPUTS_DIR/ramulator-in/$1.trace > $INPUTS_DIR/ramulator-out/$1.cmd

rm $INPUTS_DIR/ramulator-in/$1.trace

bin/ramulator2sc_PU${2}_IB${3}_VWR${4}_WORD${5} $INPUTS_DIR/raw/$1.seq $INPUTS_DIR/ramulator-out/$1.cmd $INPUTS_DIR/SystemC/$1.sci 1

rm $INPUTS_DIR/raw/$1.seq 
rm $INPUTS_DIR/ramulator-out/$1.cmd

cd ..
Debug/pim-cores_PU${2}_IB${3}_VWR${4}_WORD${5} $1
cd inputs

# ./decode_results results/$1.results
