#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <random>

#include "../../src/defs.h"

using namespace std;

// Definition of the address mapping
enum class Level : int {Channel, Rank, BankGroup, Bank, Row, Column, MAX};
string level_str[int(Level::MAX)] = {"Ch", "Ra", "Bg", "Ba", "Ro", "Co"};
int addr_bits[int(Level::MAX)] = {CHANNEL_BITS, RANK_BITS, BG_BITS, BANK_BITS, ROW_BITS, COL_BITS};
Level map[int(Level::MAX)] = {Level::Channel, Level::Column, Level::Rank,
                        Level::BankGroup, Level::Bank, Level::Row};
int global_offset = GLOBAL_OFFSET;

long unsigned int build_addr(vector<uint64_t> addr_vec);

int main(int argc, const char *argv[])
{
    if (argc != 7) {
        cout << "Usage: " << argv[0] << " <channel> <rank> <bank group> <bank> <row> <column>" << endl;
        return 0;
    }

    cout << showbase << hex << build_addr({atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), atoi(argv[5]), atoi(argv[6])}) << endl;
    return 0;
}

// Function for building the address using the indices of the different levels
uint64_t build_addr(vector<uint64_t> addr_vec)
{
    uint64_t addr_aux = 0;
    uint64_t offset = 6; // Don't know really why right now, maybe to address groups of 128

    for (int i = 0; i < int(Level::MAX); i++) {
        if (addr_bits[int(map[i])]) {
            if (addr_vec[int(map[i])] >= (1 << addr_bits[int(map[i])]))
                cout << "Warning, " << level_str[int(map[i])] << " index is too large" << endl;
            if (map[i] == Level::Row && addr_vec[int(map[i])] >= (1 << (addr_bits[int(map[i])]-1)))
                cout << "Warning, row MSB is set, which can interfere with PIM computing" << endl;
            addr_aux |= (addr_vec[int(map[i])] << offset);
            offset += addr_bits[int(map[i])];
        }
    }

    return addr_aux;
}