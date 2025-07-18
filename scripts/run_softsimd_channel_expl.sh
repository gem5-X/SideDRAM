
#!/bin/bash

DEFS_FILE=$SIDEDRAM_HOME/src/defs.h
COMPILE=0

IB_ENTRIES=64

DESIRED_STR="MIXED_SIM   0"
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct MIXED_SIM, not replacing..."
else
    SED_CMD='s/MIXED_SIM   [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change MIXED_SIM"
        exit 1
    fi

    echo $SED_CMD
fi

DESIRED_STR="RECORDING   0"
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct RECORDING, not replacing..."
else
    SED_CMD='s/RECORDING   [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change RECORDING"
        exit 1
    fi

    echo $SED_CMD
fi

DESIRED_STR="EN_MODEL    1"
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct EN_MODEL, not replacing..."
else
    SED_CMD='s/EN_MODEL    [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change EN_MODEL"
        exit 1
    fi

    echo $SED_CMD
fi

DESIRED_STR="VCD_TRACE   0"
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct VCD_TRACE, not replacing..."
else
    SED_CMD='s/VCD_TRACE   [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change VCD_TRACE"
        exit 1
    fi

    echo $SED_CMD
fi

DESIRED_STR="DEBUG       0"
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct DEBUG, not replacing..."
else
    SED_CMD='s/DEBUG       [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change DEBUG"
        exit 1
    fi

    echo $SED_CMD
fi

DESIRED_STR="INSTR_FORMAT    BASE_FORMAT"
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct INSTR_FORMAT, not replacing..."
else
    SED_CMD='s/INSTR_FORMAT    [A-Z_]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change INSTR_FORMAT"
        exit 1
    fi

    echo $SED_CMD
fi

DESIRED_STR="IB_ENTRIES      "$IB_ENTRIES
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct IB_ENTRIES, not replacing..."
else
    SED_CMD='s/IB_ENTRIES      [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change IB_ENTRIES"
        exit 1
    fi

    echo $SED_CMD
fi

CSD_ENTRIES = $((IB_ENTRIES / 2))
DESIRED_STR="CSD_ENTRIES     "$CSD_ENTRIES
if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
    echo "Cached: defs.h already contains the correct CSD_ENTRIES, not replacing..."
else
    SED_CMD='s/CSD_ENTRIES     [0123456789]+/'$DESIRED_STR'/'
    echo "Editing defs.h ..."

    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
        echo "Couldn't edit defs.h to change CSD_ENTRIES"
        exit 1
    fi

    echo $SED_CMD
fi

if [ $COMPILE -eq 1 ]; then
    # Loop through VWR sizes
    for NUM_PU in 2 4 8 16
    do 
        DESIRED_STR="CORES_PER_PCH   $NUM_PU"
        if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
            echo "Cached: defs.h already contains the correct CORES_PER_PCH, not replacing..."
        else
            SED_CMD='s/CORES_PER_PCH   [0123456789]+/'$DESIRED_STR'/'
            echo "Editing defs.h ..."

            if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
                echo "Couldn't edit defs.h to change CORES_PER_PCH"
                exit 1
            fi

            echo $SED_CMD
        fi

        for VWR_BITS in 1024 2048 4096 8192
        do
            DESIRED_STR="VWR_BITS        "$VWR_BITS
            if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
                echo "Cached: defs.h already contains the correct VWR_SIZE, not replacing..."
            else
                SED_CMD='s/VWR_BITS        [0123456789]+/'$DESIRED_STR'/'
                echo "Editing defs.h ..."

                if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
                    echo "Couldn't edit defs.h to change VWR_SIZE"
                    exit 1
                fi

                echo $SED_CMD
            fi

            DESIRED_STR="DRAM_BITS       "$VWR_BITS
            if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
                echo "Cached: defs.h already contains the correct DRAM_BITS, not replacing..."
            else
                SED_CMD='s/DRAM_BITS       [0123456789]+/'$DESIRED_STR'/'
                echo "Editing defs.h ..."

                if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
                    echo "Couldn't edit defs.h to change DRAM_BITS"
                    exit 1
                fi

                echo $SED_CMD
            fi

            # Loop through word sizes
            for WORD_BITS in 192 384 768 1536 3072
            do
                # Skip if VWR_BITS is less than WORD_BITS
                if [ $VWR_BITS -lt $WORD_BITS ]; then
                    continue
                fi

                DESIRED_STR="WORD_BITS       "$WORD_BITS
                if grep -q -E "$DESIRED_STR" $DEFS_FILE; then
                    echo "Cached: defs.h already contains the correct WORD_BITS, not replacing..."
                else
                    SED_CMD='s/WORD_BITS       [0123456789]+/'$DESIRED_STR'/'
                    echo "Editing defs.h ..."

                    if ! sed -i -E "$SED_CMD" $DEFS_FILE ; then
                        echo "Couldn't edit defs.h to change WORD_BITS"
                        exit 1
                    fi

                    echo $SED_CMD
                fi

                # Compile everything
                cd $INPUTS_DIR/
                ./compile_all_conf.sh $NUM_PU $IB_ENTRIES $VWR_BITS $WORD_BITS &
                make -C $SIDEDRAM_HOME/Debug/ all -j32
                mv $SIDEDRAM_HOME/Debug/pim-cores $SIDEDRAM_HOME/Debug/pim-cores_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
                wait

            done
        done
    done
fi

cd $INPUTS_DIR/

MAX_JOBS=60
NICENESS=1

renice -n $NICENESS $$

wait-n ()
{ StartJobs="$(jobs -p)"
  CurJobs="$(jobs -p)"
  while diff -q  <(echo -e "$StartJobs") <(echo -e "$CurJobs") >/dev/null
  do
    sleep 1
    CurJobs="$(jobs -p)"
  done
}

for NUM_PU in 2 4 8 16
do 
    for VWR_BITS in 1024 2048 4096 8192
    do 
        for WORD_BITS in 192 384 768 1536 3072
        do
            # Skip if VWR_BITS is less than WORD_BITS
            if [ $VWR_BITS -lt $WORD_BITS ]; then
                continue
            fi

            for SW_BW in 3 4 6 8 12 16 24
            do 
                # Check if $SIDEDRAM_HOME/stats/cycles_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv exists
                if [ -f $SIDEDRAM_HOME/stats/cycles_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv ]; then
                    echo "Cached: $SIDEDRAM_HOME/stats/cycles_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv already exists, not replacing..."
                else
                    echo "Creating $SIDEDRAM_HOME/stats/cycles_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv..."
                    echo "NUM_PU,VWR_BITS,WORD_BITS,IB_ENTRIES,M,N,Q,SW_BW,CYCLES" > $SIDEDRAM_HOME/stats/cycles_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv
                fi

                for Q in 64 128 256 512 1024 2048 4096 8192 16384 32768 65536
                do
                    (
                        M=1
                        N=64

                        # Skip if the simulation results are already cached
                        if [ ! -f $INPUTS_DIR//recording/gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}_0_cp.csv ]; then # || [ ! -f $INPUTS_DIR//dram-energy/gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv ]; then

                            echo "Running PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} GEMM m=$M n=$N q=$Q sw_bw=${SW_BW}..."
                            echo "----------------------------------------"

                            bin/gen_gemm_assembly_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} --m=$M --n=${N} --q=${Q} --sw_bw=${SW_BW} --csd_len=8 --dram_intvl=5 --output=gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
                            ./assembly2sc_conf.sh gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} $NUM_PU $IB_ENTRIES $VWR_BITS $WORD_BITS 1 # Change number of banks if needed
                            CYCLES=$( tail -n 1 $INPUTS_DIR//SystemC/gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0 | awk '{print $1}')
                            echo "${NUM_PU},${VWR_BITS},${WORD_BITS},${IB_ENTRIES},${M},${N},${Q},${SW_BW},${CYCLES}" >> $SIDEDRAM_HOME/stats/cycles_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv
                            rm $INPUTS_DIR//SystemC/gemm_m${M}_n${N}_q${Q}_SW${SW_BW}_PU${NUM_PU}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0

                        fi
                    ) &
                    
                    sleep 1

                    # Allow only MAX_JOBS jobs to run at a time
                    if [[ $(jobs -r -p | wc -l) -ge $MAX_JOBS ]]; then
                        wait -n
                    fi
                done
            done
        done
    done
done

wait
echo "Done"
