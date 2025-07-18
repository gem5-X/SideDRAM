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
    // Randomization tools
    std::mt19937 gen(11111);    // Standard mersenne_twister_engine seeded
    std::uniform_int_distribution<> dis_bool(0, 1);
    std::uniform_int_distribution<> dis_channel(0, (1 << addr_bits[int(Level::Channel)])-1);
    std::uniform_int_distribution<> dis_rank(0, (1 << addr_bits[int(Level::Rank)])-1);
    std::uniform_int_distribution<> dis_bg(0, (1 << addr_bits[int(Level::BankGroup)])-1);
    std::uniform_int_distribution<> dis_bank(0, (1 << addr_bits[int(Level::Bank)])-1);
    std::uniform_int_distribution<> dis_row(0, (1 << addr_bits[int(Level::Row)])-1);
    std::uniform_int_distribution<> dis_column(0, (1 << addr_bits[int(Level::Column)])-1);
    std::uniform_int_distribution<> dis_data(0, (1 << 30)-1);

    uint chan, rank, bg, bank, row, col;
    chan = rank = bg = bank = row = col = 0;
    string cmd = "RD";
    
    ofstream output, debug;
    output.open("ex.seq");
    debug.open("addr.debug");

    output << showbase << internal << setfill ('0') << hex;

    for (int i = 0; i < 100; i++){
        bank = dis_bank(gen);
        bg = dis_bg(gen);
        col = dis_column(gen);
        row = dis_row(gen);
        output << build_addr({chan, rank, bg, bank, row, col}) << "\t";
        // output << (dis_bool(gen) ? "WR" : "RD") << "\t";
        // output << dis_data(gen);
        output << "WR" << endl;
        debug << chan << " " << rank << " " << bg << " " << bank << " " << row << " " << col << endl;
    }

    output.close();
    return 0;
}

// Function for building the address using the indices of the different levels
uint64_t build_addr(vector<uint64_t> addr_vec)
{
    uint64_t addr_aux = 0;
    uint64_t offset = 6; // Don't know really why right now, maybe to address groups of 128

    for (int i = 0; i < int(Level::MAX); i++) {
        if (addr_bits[int(map[i])]) {
            addr_aux |= (addr_vec[int(map[i])] << offset);
            offset += addr_bits[int(map[i])];
        }
    }

    return addr_aux;
}