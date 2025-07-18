
#!/bin/bash

DEFS_FILE=$SIDEDRAM_HOME/src/defs.h

IB_ENTRIES=64

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

# Loop through VWR sizes
for VWR_BITS in 256 4096 8192
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
    for WORD_BITS in 48 96 192 384 768 1536
    do
        # Only do if WORD_BITS is smaller than DRAM_BITS
        if [[ $WORD_BITS -lt $VWR_BITS && !($VWR_BITS -eq 8192 && $WORD_BITS -eq 48) ]] 
        then   
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
            ./compile_all_conf.sh $IB_ENTRIES $VWR_BITS $WORD_BITS &
            make -C $SIDEDRAM_HOME/Debug/ all -j8
            mv $SIDEDRAM_HOME/Debug/pim-cores $SIDEDRAM_HOME/Debug/pim-cores_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
            wait

        fi
    done
done

cd $INPUTS_DIR/

MAX_JOBS=4

wait-n ()
{ StartJobs="$(jobs -p)"
  CurJobs="$(jobs -p)"
  while diff -q  <(echo -e "$StartJobs") <(echo -e "$CurJobs") >/dev/null
  do
    sleep 1
    CurJobs="$(jobs -p)"
  done
}

for VWR_BITS in 256 4096 8192
do 
    for WORD_BITS in 48 96 192 384 768 1536
    do
        if [[ $WORD_BITS -lt $VWR_BITS && !($VWR_BITS -eq 8192 && $WORD_BITS -eq 48) ]] 
        then
            echo "VWR_BITS,WORD_BITS,KERNEL_TYPE,KERNEL_SIZE,SW_BW,CYCLES" > $SIDEDRAM_HOME/stats/cycles_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv
            for KERNEL_SIZE in 128 256 512 1024
            do
                for SW_BW in 3 4 6 8 12 16 24
                do 
                    (
                        echo "Running IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} kernel_size=${KERNEL_SIZE} sw_bw=${SW_BW}..."
                        echo "----------------------------------------"

                        bin/gen_gemm_assembly_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} --m=1 --n=${KERNEL_SIZE} --q=${KERNEL_SIZE} --sw_bw=${SW_BW} --csd_len=8 --dram_intvl=5 --output=gemv${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
                        ./assembly2sc_conf.sh gemv${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} $IB_ENTRIES $VWR_BITS $WORD_BITS 16 # Change number of banks if needed
                        CYCLES=$( tail -n 1 SystemC/gemv${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0 | awk '{print $1}')
                        echo "${VWR_BITS},${WORD_BITS},gemv,${KERNEL_SIZE},${SW_BW},${CYCLES}" >> $SIDEDRAM_HOME/stats/cycles_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv
                        rm SystemC/gemv${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0

                        bin/gen_gemm_assembly_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} --m=${KERNEL_SIZE} --n=${KERNEL_SIZE} --q=${KERNEL_SIZE} --sw_bw=${SW_BW} --csd_len=8 --dram_intvl=5 --output=gemm${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}
                        ./assembly2sc_conf.sh gemm${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS} $IB_ENTRIES $VWR_BITS $WORD_BITS 16 # Change number of banks if needed
                        CYCLES=$( tail -n 1 SystemC/gemm${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0 | awk '{print $1}')
                        echo "${VWR_BITS},${WORD_BITS},gemm,${KERNEL_SIZE},${SW_BW},${CYCLES}" >> $SIDEDRAM_HOME/stats/cycles_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.csv
                        rm SystemC/gemm${KERNEL_SIZE}_SW${SW_BW}_IB${IB_ENTRIES}_VWR${VWR_BITS}_WORD${WORD_BITS}.sci0
                    ) &
                    
                    sleep 10

                    # Allow only MAX_JOBS jobs to run at a time
                    if [[ $(jobs -r -p | wc -l) -ge $MAX_JOBS ]]; then
                        wait -n
                    fi
                done
            done
        fi
    done
done

wait
echo "Done"