#include "../cnm_base.h"

#if MIXED_SIM == 0	// Testbench for SystemC simulation

#include "pch_driver.h"

#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <deque>
#include <assert.h>

using namespace std;

void pch_driver::driver_thread() {

    int i, j, curCycle;
    sc_uint<64> bank2out;
    bool lastCmd, bankRead, bankWrite;

    sc_uint<ADDR_TOTAL_BITS> addrAux;

    // Values for reading from input
    string line;
    int readCycle = 0;
    unsigned long int readAddr;
    dq_type data2DQ;
    uint64_t dataAux, data2bankAux;
    sc_biguint<DRAM_BITS> bankAux, data2bank;
    sc_lv<DRAM_BITS> allzs(SC_LOGIC_Z);
    string readCmd;
    deque<uint64_t> readData;
    deque<sc_biguint<DRAM_BITS> > data2bankBuffer;


    // Initial reset
    curCycle = 0;
    lastCmd = false;
    bankRead = false;
    bankWrite = false;
    rst->write(false);
    RD->write(false);
    WR->write(false);
    ACT->write(false);
    AB_mode->write(false);
    pim_mode->write(false);
    bank_addr->write(0);
    row_addr->write(0);
    col_addr->write(0);
    DQ->write(0);
    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
        even_buses[i]->write(allzs);
        odd_buses[i]->write(allzs);
#else
        dram_buses[i]->write(allzs);
#endif
    }

    wait(CLK_PERIOD / 2, RESOLUTION);
    rst->write(1);
    wait(0, RESOLUTION);

    wait(CLK_PERIOD / 2 + 1, RESOLUTION);
    curCycle++;

    // Open input file
    string fi = "INPUTS_DIR/SystemC/" + filename + ".sci0";		// Input file name, located in pim-cores folder
    ifstream input;
    input.open(fi);
    if (!input.is_open())   {
        cout << "Error when opening input file " << fi << endl;
        sc_stop();
        return;
    }

    // Open output file
    string fo = "INPUTS_DIR/results/" + filename + ".results";	// Output file name, located in pim-cores folder
    ofstream output;
    output.open(fo);
    if (!output.is_open())   {
        cout << "Error when opening output file" << fo << endl;
        sc_stop();
        return;
    }

    // Read first line
    if (getline(input, line)) {

        // Read elements from a line in the input file
        istringstream iss(line);
        readData.clear();
        if (!(iss >> dec >> readCycle >> hex >> readAddr >> readCmd)) {
            cout << "Error when reading input" << endl;
            sc_stop();
            return;
        }
        while (iss >> hex >> dataAux) {
            readData.push_back(dataAux);
        }

    } else {
        cout << "No lines in the input file" << endl;
        sc_stop();
        return;
    }

    // cout << "MACRO_DST_BITS = " << MACRO_DST_BITS << endl;
    // cout << "MACRO_SRC_BITS = " << MACRO_SRC_BITS << endl;
    // cout << "MACRO_SW_LEN_BITS = " << MACRO_SW_LEN_BITS << endl;
    // cout << "MACRO_SHIFTSA_BITS = " << MACRO_SHIFTSA_BITS << endl;
    // cout << "MACRO_SRC0_N_BITS = " << MACRO_SRC0_N_BITS << endl;
    // cout << "MACRO_REPACK_BITS = " << MACRO_REPACK_BITS << endl;
    // cout << "MACRO_PACK_STA_BITS = " << MACRO_PACK_STA_BITS << endl;
    // cout << "MACRO_DST_N_BITS = " << MACRO_DST_N_BITS << endl;
    // cout << "TS_STA_BITS = " << TS_STA_BITS << endl;
    // cout << "TS_MODE_BITS = " << TS_MODE_BITS << endl;
    // cout << "TS_BITS = " << TS_BITS << endl;
    // cout << "MACRO_IMM_BITS = " << MACRO_IMM_BITS << endl;
    // cout << "MACRO_AUX_BITS = " << MACRO_AUX_BITS << endl;
    // cout << "MACRO_BITS = " << MACRO_BITS << endl;
    // cout << "SHIFT_ITT_IDX = " << SHIFT_ITT_IDX << endl;
    // cout << "MOV_IMM_BITS = " << MOV_IMM_BITS << endl;
    // cout << "MOV_FIELDS_BITS = " << MOV_FIELDS_BITS << endl;
    // cout << "SA_SW_LEN_BITS = " << SA_SW_LEN_BITS << endl;
    // cout << "SA_SHIFT_BITS = " << SA_SHIFT_BITS << endl;
    // cout << "SA_FIELDS_BITS = " << SA_FIELDS_BITS << endl;
    // cout << "PM_REPACK_BITS = " << PM_REPACK_BITS << endl;
    // cout << "PM_PACK_STA_BITS = " << PM_PACK_STA_BITS << endl;
    // cout << "PM_SHIFT_BITS = " << PM_SHIFT_BITS << endl;
    // cout << "PM_DST_N_BITS = " << PM_DST_N_BITS << endl;
    // cout << "PM_FIELDS_BITS = " << PM_FIELDS_BITS << endl;

    // Simulation loop
    while (1) {

//    	cout << "Cycle " << dec << curCycle << endl;

        // Default values
        RD->write(false);
        WR->write(false);
        ACT->write(false);
        AB_mode->write(true);
        pim_mode->write(true);
        DQ->write(0);
        for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
            even_buses[i]->write(allzs);
            odd_buses[i]->write(allzs);
#else
            dram_buses[i]->write(allzs);
#endif
        }

        // Fill the banks' sense amplifiers with the corresponding data
        if (bankRead) {
#if DUAL_BANK_INTERFACE
            if (addrAux.range(BA_END, BA_END)) {
                for (i = 0; i < CORES_PER_PCH; i++) {
                    odd_buses[i]->write(data2bankBuffer.front());
                    data2bankBuffer.pop_front();
                }
            } else {
                for (i = 0; i < CORES_PER_PCH; i++) {
                    even_buses[i]->write(data2bankBuffer.front());
                    data2bankBuffer.pop_front();
                }
            }
#else
            for (i = 0; i < CORES_PER_PCH; i++) {
                dram_buses[i]->write(data2bankBuffer.front());
                data2bankBuffer.pop_front();
            }
#endif
            bankRead = false;
        }

        // Execute necessary command at the right time and read next line
        // if current cycle caught up with the previous read one
        if (!lastCmd && curCycle >= readCycle) {

            // Execute command at the corresponding cycle, assuming PIM mode

            // Check first the MSB of the row address to see which mode we're in
            // (writing to RFs or PIM execution)
            addrAux = readAddr;
            if (addrAux.range(RO_STA, RO_STA)) {
                // Writing to the RFs

                // Check command
                if (!readCmd.compare("RD")) {

                    // If writing to RFs and RD, do nothing
                    cout << "Warning: RF writing mode but saw a RD command"
                            << endl;

                } else {    // NOTE Only works if <= 64 bits, otherwise need to implement DQcycle count

                    // If writing to RFs and WR, format data to DQ
                        assert(readData.size() == 1);   // Check it is only one piece of data
                        data2DQ = readData.front();
                        readData.pop_front();
                        WR->write(true);
                        bank_addr->write(addrAux.range(BA_STA, BA_END));
                        row_addr->write(addrAux.range(RO_STA, RO_END));
                        col_addr->write(addrAux.range(CO_STA, CO_END));
                        DQ->write(data2DQ);
                }

            } else {
                // PIM execution

                // Check command
                if (!readCmd.compare("RD")) {

                    RD->write(true);
                    bank_addr->write(addrAux.range(BA_STA, BA_END));
                    row_addr->write(addrAux.range(RO_STA, RO_END));
                    col_addr->write(addrAux.range(CO_STA, CO_END));

                    // If PIM execution and RD with input data, send to the corresponding bank buses in the next cycle
                    if (!readData.empty()){
                        assert(readData.size() == VWR_64B*CORES_PER_PCH);// Check if there are enough pieces of data

                        for (i = 0; i < CORES_PER_PCH; i++) {
                            for (j = 0; j < VWR_64B; j++){
                                data2bankAux = readData.front();
                                readData.pop_front();
                                if (64*(j+1)-1 < VWR_BITS) {
                                    data2bank.range(64*(j+1)-1,64*j) = data2bankAux;
                                } else {
                                    data2bank.range(VWR_BITS-1,64*j) = data2bankAux;
                                }
                            }
                            data2bankBuffer.push_back(data2bank);
                        }

                        bankRead = true;
                    }

                } else {

                    // If PIM execution and WR, we record bank buses next cycle
                    WR->write(true);
                    bank_addr->write(addrAux.range(BA_STA, BA_END));
                    row_addr->write(addrAux.range(RO_STA, RO_END));
                    col_addr->write(addrAux.range(CO_STA, CO_END));
                    bankWrite = true;

                    // Write address here because it will be overwritten later with the next cmd
                    output << showbase << dec << curCycle << "\t" << hex << readAddr << "\t";

                }
            }

            // Read next line
            if (getline(input, line)) {

                // Read elements from a line in the input file
                istringstream iss(line);
                readData.clear();
                if (!(iss >> dec >> readCycle >> hex >> readAddr >> readCmd)) {
                    cout << "Error when reading input" << endl;
                    break;
                }
                while (iss >> hex >> dataAux) {
                    readData.push_back(dataAux);
                }

            } else {// Wait for enough time for the last instruction to be completed
                input.close();
                lastCmd = true;
                readCycle += (5);
            }

        } else if (lastCmd && curCycle >= readCycle) {
            // End of simulation, last command was already read
            output.close();
            break;
        }

        if (bankWrite) {
            for (i = 0; i < 20; i++)    // More than one deltas are needed
                wait(0, RESOLUTION);    // We need to wait for a delta to solve the bank buses
#if DUAL_BANK_INTERFACE
            if (addrAux.range(BA_END, BA_END)) {
                for (i = 0; i < CORES_PER_PCH; i++) {
                    bankAux = odd_buses[i]->read();
                    for (j = 0; j < VWR_64B; j++) {
                        if (64*(j+1)-1 < VWR_BITS) {
                            bank2out = bankAux.range(64*(j+1)-1,64*j);
                        } else {
                            bank2out = bankAux.range(VWR_BITS-1,64*j);
                        }
                        output << showbase << hex << bank2out << "\t";
                    }
                }
            } else {
                for (i = 0; i < CORES_PER_PCH; i++) {
                    bankAux = even_buses[i]->read();
                    for (j = 0; j < VWR_64B; j++) {
                        if (64*(j+1)-1 < VWR_BITS) {
                            bank2out = bankAux.range(64*(j+1)-1,64*j);
                        } else {
                            bank2out = bankAux.range(VWR_BITS-1,64*j);
                        }
                        output << showbase << hex << bank2out << "\t";
                    }
                }
            }
#else
            for (i = 0; i < CORES_PER_PCH; i++) {
                bankAux = dram_buses[i]->read();
                for (j = 0; j < VWR_64B; j++) {
                    if (64*(j+1)-1 < VWR_BITS) {
                        bank2out = bankAux.range(64*(j+1)-1,64*j);
                    } else {
                        bank2out = bankAux.range(VWR_BITS-1,64*j);
                    }
                    output << showbase << hex << bank2out << "\t";
                }
            }
#endif
            output << endl;
            bankWrite = false;
        }

        wait(CLK_PERIOD, RESOLUTION);
        curCycle++;
    }

    cout << "Simulation finished at cycle " << dec << curCycle << endl;

    // Stop simulation
    sc_stop();

}

#endif
