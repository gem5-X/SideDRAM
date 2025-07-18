#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <deque>
#include <array>
#include <random>

#include "../../src/defs.h"

using namespace std;

// Format of raw traces:        Address R/W     Data
// Format of ramulator output:  Cmd     Cycle   Channel Rank    BG  Bank    Row Column
// Format of SystemC input:     Cycle   Address R/W     Data

struct rawCmd {
    unsigned long int addr;
    deque<unsigned long long int> data;
};

struct chanSeq {
    deque<rawCmd> readq;
    deque<rawCmd> writeq;
    ofstream output;
};

// Definition of the address mapping
enum class Level : int {Channel, Rank, BankGroup, Bank, Row, Column, MAX};
string level_str[int(Level::MAX)] = {"Ch", "Ra", "Bg", "Ba", "Ro", "Co"};
int addr_bits[int(Level::MAX)] = {CHANNEL_BITS, RANK_BITS, BG_BITS, BANK_BITS, ROW_BITS, COL_BITS};
Level addr_map[int(Level::MAX)] = {Level::Channel, Level::Column, Level::Rank,
                        Level::BankGroup, Level::Bank, Level::Row};
int global_offset = GLOBAL_OFFSET;

unsigned long int build_addr(vector<uint64_t> addr_vec);

unsigned int get_channel(unsigned long int addr);

int main(int argc, const char *argv[])
{   
    if (argc != 5) {
        cout << "Usage: " << argv[0] << " <raw-sequence> <ramulator-output> <output-file> <number-channels>" << endl;
        return 0;
    }

    string rs = argv[1];    // Input raw sequences file name
    string ro = argv[2];    // Input ramulator output file name
    string fo = argv[3];    // Output file name
    unsigned int numChannels = atoi(argv[4]);
    string rsline, roline;
    string finalLine = "Simulation done.";  // Start of final line in ramulator output

    // Open input files
    ifstream rawSeq;
    ifstream ramOut;
    rawSeq.open(rs);
    ramOut.open(ro);

    // Create channels and open output files
    chanSeq channel[numChannels];
    for (int i = 0; i < numChannels; i++) {
        channel[i].output.open(fo + to_string(i));
    }

    // Variables for holding ramulator output data
    string ramCmd;
    uint64_t cycle, ch, ra, bg, ba, row, col;
    char colon;
    unsigned long int ramAddr;

    // Variables for holding raw sequence data
    unsigned int rsCh;
    unsigned long int rsAddr, dataAux;
    deque<unsigned long long int> rsData;
    string rsCmd;

    // Run though the ramulator output
    while (getline(ramOut, roline)) {

        rsData.clear();

        // Check that is not last line in the ramulator output
        if (!roline.compare(0, finalLine.size(), finalLine)) {
            cout << "Final line reached" << endl;
            continue;
        } else {
            
            // Read elements from a line in the ramulator output
            istringstream roiss(roline);
            if (!(roiss >> ramCmd >> cycle >> colon >> ch >> ra >> bg >> ba >> row >> col)) {
                cout << "Error when reading ramulator output" << endl;
                break;
            }

            // If not RD or WR, jump to next iteration
            if (ramCmd.compare("RD") && ramCmd.compare("WR"))
                continue;

            // Build the address
            ramAddr = build_addr({ch, ra, bg, ba, row, col});

            // Check in the correct queue if it's not empty
            bool found = false;
            if (!ramCmd.compare("RD")) {    // Read queue
                auto queuedCmd = channel[ch].readq.begin();
                while (queuedCmd != channel[ch].readq.end() &&
                        (queuedCmd->addr >> global_offset) != (ramAddr >> global_offset)) {
                    ++queuedCmd;
                    
                }
                if (queuedCmd != channel[ch].readq.end()) {
                    rsData = queuedCmd->data;
                    queuedCmd = channel[ch].readq.erase(queuedCmd);
                    found = true;
                }
            } else {                        // Write queue  
                auto queuedCmd = channel[ch].writeq.begin();
                while (queuedCmd != channel[ch].writeq.end() &&
                        (queuedCmd->addr >> global_offset) != (ramAddr >> global_offset)) {
                    ++queuedCmd;
                }
                if (queuedCmd != channel[ch].writeq.end()) {
                    rsData = queuedCmd->data;
                    queuedCmd = channel[ch].writeq.erase(queuedCmd);
                    found = true;
                }
            }

            // If queue empty or not found there, search in the raw sequence
            while (!found) {
                if (getline(rawSeq, rsline)) {
                    istringstream rsiss(rsline);
                    if (!(rsiss >> hex >> rsAddr >> rsCmd)) {
                        cout << "Error when reading raw sequence" << endl;
                        break;
                    }
                    while (rsiss >> dataAux) {
                        rsData.push_back(dataAux);
                    }
                    if ((rsAddr >> global_offset) == (ramAddr >> global_offset) &&
                        !ramCmd.compare(rsCmd)) {
                        found = true;
                    } else {
                        // Extract channel to push to the correct queue
                        rsCh = get_channel(rsAddr);

                        // Push non-matching commands to the correct queue
                        rawCmd cmdToQueue = {rsAddr, rsData};
                        (!rsCmd.compare("RD")) ?
                            channel[rsCh].readq.push_back(cmdToQueue) :
                            channel[rsCh].writeq.push_back(cmdToQueue);
                        rsData.clear();
                    }
                } else {
                    cout << "Error: command not found in raw sequence" << endl;
                    break;
                }
            }

            // If not found anywhere, abort the program
            if (!found)
                break;

            // Write to correct output file
            channel[ch].output << showbase << dec << cycle << "\t" << hex << ramAddr << "\t";
            channel[ch].output << ramCmd << "\t";
            while (!rsData.empty()) {
                channel[ch].output << hex << rsData.front() << "\t";
                rsData.pop_front();
            }
            channel[ch].output << endl;
        }
    }
    
    rawSeq.close();
    ramOut.close();
    for (int i = 0; i < numChannels; i++) {
        channel[i].output.close();
    }

    cout << "Program finished" << endl;

    return 0;
}

// Function for building the address using the indices of the different levels
uint64_t build_addr(vector<uint64_t> addr_vec)
{
    uint64_t addr_aux = 0;
    uint64_t offset = 6; // Don't know really why right now, maybe to address groups of 128

    for (int i = 0; i < int(Level::MAX); i++) {
        if (addr_bits[int(addr_map[i])]) {
            addr_aux |= (addr_vec[int(addr_map[i])] << offset);
            offset += addr_bits[int(addr_map[i])];
        }
    }

    return addr_aux;
}

// Function to get the channel index from an address
unsigned int get_channel(unsigned long int addr) {
    unsigned int channel, channelMask;
    uint offset = 6; // Initial offset

    addr = addr >> offset;  // Remove initial offset
    channelMask = (1 << addr_bits[int(addr_map[0])]) - 1;
    channel = addr & channelMask;

    return channel;
}