#!/bin/bash

# Architecture configuration for SideDRAM-384B
export NUM_PU=2
export IB_ENTRIES=64
export VWR_BITS=8192
export WORD_BITS=1536

cd $SIDEDRAM_HOME/inputs

echo "NUM_PU,VWR_BITS,WORD_BITS,IB_ENTRIES,M,N,Q,SW_BW,CYCLES" > ../stats/base_exploration.csv

for SW_BW in 3 4 6 8 12 16 24
do
    for Q in 64 256 1024 4096
    do
        M=1
        N=64

        echo "Running PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} Q=${Q} M=${M} N=${N} SW_BW=${SW_BW}..."
        echo "----------------------------------------"

        bin/gen_gemm_assembly --m=$M --n=${N} --q=${Q} --sw_bw=${SW_BW} --csd_len=8 --dram_intvl=5 --output=gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
        ./assembly2sc.sh gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
        CYCLES=$( tail -n 1 $INPUTS_DIR/SystemC/gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0 | awk '{print $1}')
        echo "${NUM_PU},${VWR_BITS},${WORD_BITS},${IB_ENTRIES},${M},${N},${Q},${SW_BW},${CYCLES}" >> ../stats/base_exploration.csv

    done
done

echo "Base exploration completed. Results saved in ../stats/base_exploration.csv"