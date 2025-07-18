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
#include <map>
#include <random>

#include "half.hpp"
#include "../../src/defs.h"
#include "../../src/opcodes.h"
#include "../../src/microcode/common_format.h"
#include "../../src/microcode/base_code.h"
#include "../../src/microcode/encoded_shift_code.h"

#define DATAFILE    "DataFile"
#define ADDRFILE    "AddrFile"

using namespace std;

// Format of assembly input:    Inst        [Operands]
// Format of raw traces:        Address     R/W         Data

enum class INSTR : uint {
    WRF,
    NOP, LOOP,
    RLB, WLB, VMV, RMV, GLMV,
    PACK, PERM,
    VFUX,
    EXEC
};

const std::map<std::string, INSTR> STRING2INSTR = {
    { "WRF", INSTR::WRF },
    { "NOP", INSTR::NOP }, { "LOOP", INSTR::LOOP },
    { "RLB", INSTR::RLB }, { "WLB", INSTR::WLB }, { "VMV", INSTR::VMV }, { "RMV", INSTR::RMV }, { "GLMV", INSTR::GLMV },
    { "PACK", INSTR::PACK }, { "PERM", INSTR::PERM },
    { "VFUX", INSTR::VFUX },
    { "EXEC", INSTR::EXEC }
};

enum class VFUX : uint {
    SHIFT, ADD, SUB, MUL
};

const std::map<std::string, VFUX> STRING2VFUX = {
    { "SHIFT", VFUX::SHIFT },
    { "ADD", VFUX::ADD },
    { "SUB", VFUX::SUB },
    { "MUL", VFUX::MUL }
};

const std::map<std::string, RF_SEL> STRING2RFSEL = {
    { "IB", RF_SEL::IB },
    { "SRF", RF_SEL::SRF }, { "MRF", RF_SEL::MRF }, { "CSD", RF_SEL::CSDRF },
    { "VWR", RF_SEL::VWR }, { "LOOP_REG", RF_SEL::LOOP_REG },
    { "CSD_LEN", RF_SEL::CSD_LEN },
};

enum CMD {
    RD, WR,
    DC
};

const std::map<uint8_t, std::string> CMD_STRING = {
    { RD, "RD" }, { WR, "WR" },
    { DC, "DC" },
};

const std::map<std::string, SWSIZE> STRING2SWSIZE = {
    { "LEN_INV",    SWSIZE::INV},
    { "LEN_3",      SWSIZE::B3 },
    { "LEN_4",      SWSIZE::B4 },
    { "LEN_6",      SWSIZE::B6 },
    { "LEN_8",      SWSIZE::B8 },
    { "LEN_12",     SWSIZE::B12 },
    { "LEN_16",     SWSIZE::B16 },
    { "LEN_24",     SWSIZE::B24 },
};

#if SW_TRANS == TWO_LEVELS
const std::map<std::string, SWREPACK> STRING2REPACK = {
    { "CHANGE_INV",     SWREPACK::INV },
    { "LEN_3",          SWREPACK::B3_B3 },
    { "LEN_4",          SWREPACK::B4_B4 },
    { "LEN_6",          SWREPACK::B6_B6 },
    { "LEN_8",          SWREPACK::B8_B8 },
    { "LEN_12",         SWREPACK::B12_B12 },
    { "LEN_16",         SWREPACK::B16_B16 },
    { "LEN_24",         SWREPACK::B24_B24 },
    { "CHANGE_3_3",     SWREPACK::B3_B3 },
    { "CHANGE_4_4",     SWREPACK::B4_B4 },
    { "CHANGE_6_6",     SWREPACK::B6_B6 },
    { "CHANGE_8_8",     SWREPACK::B8_B8 },
    { "CHANGE_12_12",   SWREPACK::B12_B12 },
    { "CHANGE_16_16",   SWREPACK::B16_B16 },
    { "CHANGE_24_24",   SWREPACK::B24_B24 },
    { "CHANGE_3_4",     SWREPACK::B3_B4 },
    { "CHANGE_3_6",     SWREPACK::B3_B6 },
    { "CHANGE_4_6",     SWREPACK::B4_B6 },
    { "CHANGE_4_8",     SWREPACK::B4_B8 },
    { "CHANGE_6_8",     SWREPACK::B6_B8 },
    { "CHANGE_6_12",    SWREPACK::B6_B12 },
    { "CHANGE_8_12",    SWREPACK::B8_B12 },
    { "CHANGE_8_16",    SWREPACK::B8_B16 },
    { "CHANGE_12_16",   SWREPACK::B12_B16 },
    { "CHANGE_12_24",   SWREPACK::B12_B24 },
    { "CHANGE_16_24",   SWREPACK::B16_B24 },
    { "CHANGE_24_16",   SWREPACK::B24_B16 },
    { "CHANGE_24_12",   SWREPACK::B24_B12 },
    { "CHANGE_16_12",   SWREPACK::B16_B12 },
    { "CHANGE_16_8",    SWREPACK::B16_B8 },
    { "CHANGE_12_8",    SWREPACK::B12_B8 },
    { "CHANGE_12_6",    SWREPACK::B12_B6 },
    { "CHANGE_8_6",     SWREPACK::B8_B6 },
    { "CHANGE_8_4",     SWREPACK::B8_B4 },
    { "CHANGE_6_4",     SWREPACK::B6_B4 },
    { "CHANGE_6_3",     SWREPACK::B6_B3 },
    { "CHANGE_4_3",     SWREPACK::B4_B3 },
};
#endif

#if SW_TRANS == ONE_LEVEL
const std::map<std::string, SWREPACK> STRING2REPACK = {
    { "CHANGE_INV",     SWREPACK::INV },
    { "LEN_3",          SWREPACK::B3_B3 },
    { "LEN_4",          SWREPACK::B4_B4 },
    { "LEN_6",          SWREPACK::B6_B6 },
    { "LEN_8",          SWREPACK::B8_B8 },
    { "LEN_12",         SWREPACK::B12_B12 },
    { "LEN_16",         SWREPACK::B16_B16 },
    { "LEN_24",         SWREPACK::B24_B24 },
    { "CHANGE_3_3",     SWREPACK::B3_B3 },
    { "CHANGE_4_4",     SWREPACK::B4_B4 },
    { "CHANGE_6_6",     SWREPACK::B6_B6 },
    { "CHANGE_8_8",     SWREPACK::B8_B8 },
    { "CHANGE_12_12",   SWREPACK::B12_B12 },
    { "CHANGE_16_16",   SWREPACK::B16_B16 },
    { "CHANGE_24_24",   SWREPACK::B24_B24 },
    { "CHANGE_3_4",     SWREPACK::B3_B4 },
    { "CHANGE_4_6",     SWREPACK::B4_B6 },
    { "CHANGE_6_8",     SWREPACK::B6_B8 },
    { "CHANGE_8_12",    SWREPACK::B8_B12 },
    { "CHANGE_12_16",   SWREPACK::B12_B16 },
    { "CHANGE_16_24",   SWREPACK::B16_B24 },
    { "CHANGE_24_16",   SWREPACK::B24_B16 },
    { "CHANGE_16_12",   SWREPACK::B16_B12 },
    { "CHANGE_12_8",    SWREPACK::B12_B8 },
    { "CHANGE_8_6",     SWREPACK::B8_B6 },
    { "CHANGE_6_4",     SWREPACK::B6_B4 },
    { "CHANGE_4_3",     SWREPACK::B4_B3 },
};
#endif

const std::map<std::string, TS_MODE> STRING2TSMODE = {
    { "MODE_NOP",       TS_MODE::NOP },
    { "MODE_SHIFT",     TS_MODE::SHIFT },
    { "MODE_BROADCAST", TS_MODE::BROADCAST },
    { "MODE_REPEAT",    TS_MODE::REPEAT }
};

const std::map<std::string, OPC_STORAGE> STRING2VFUXDST = {
    { "OUT_R3",     OPC_STORAGE::R3 },
    { "OUT_R1",     OPC_STORAGE::R1 },
    { "OUT_R2",     OPC_STORAGE::R2 },
    { "OUT_R1R2",   OPC_STORAGE::R1R2 },
};

class nmcInst {
  public:
    MACRO_IDX ittIdx;
    uint64_t imm;
    OPC_STORAGE dst;
    OPC_STORAGE src;
    OPC_STORAGE src0;
    uint64_t dstN;
    uint64_t srcN;
    uint64_t src0N;
    SWSIZE swLen;
    uint shiftSA;
    uint64_t rfN;
    SWREPACK repack;
    uint packSta;
    uint shiftPM;
    bool dstAddrFile;
    bool srcAddrFile;
    bool relu;
    bool dataFile;
    deque<uint64_t> data;
    nmcInst()
        : ittIdx(MACRO_IDX::SAFE_STATE), imm(0), dst(OPC_STORAGE::IB), src(OPC_STORAGE::IB), src0(OPC_STORAGE::IB),
          dstN(0), srcN(0), src0N(0), swLen(SWSIZE::INV), shiftSA(0), rfN(0), repack(SWREPACK::INV), packSta(0),
          shiftPM(0), dstAddrFile(false), srcAddrFile(false), relu(false), dataFile(false)
    {}
};

// Definition of the address mapping
enum class Level : int {Channel, Rank, BankGroup, Bank, Row, Column, MAX};
string level_str[int(Level::MAX)] = {"Ch", "Ra", "Bg", "Ba", "Ro", "Co"};
int addr_bits[int(Level::MAX)] = {CHANNEL_BITS, RANK_BITS, BG_BITS, BANK_BITS, ROW_BITS, COL_BITS};
Level addr_map[int(Level::MAX)] = {Level::Channel, Level::Column, Level::Rank,
                        Level::BankGroup, Level::Bank, Level::Row};
int global_offset = GLOBAL_OFFSET;

// Function for building the address using the indices of the different levels
uint64_t build_addr(vector<uint64_t> addr_vec, bool rf_write);

// Function for spliting the store type and the index
bool splitStoreIndex(string* storeTypeString, uint64_t *idx);

// Function for spliting the vwr sel and the index
bool splitVwrIndeces(string vwrString, OPC_STORAGE *vwr_sel, uint64_t *vwr_idx, bool idxPresent);

// Function to interpret the DRAM address
bool interpretAddr(string addrString, uint64_t *addr);

// Function to generate the mask for the DRAM access from the string
uint interpretMask(string maskString);

// Function to generate the mode and start index for tile shuffling
uint interpretShuffle(string modeString, uint start_idx);

// Function for getting the bank index from an address
uint8_t get_bank(uint64_t addr);

// Function for getting the row index from an address
uint16_t get_row(uint64_t addr);

// Function for getting the column index from an address
uint16_t get_col(uint64_t addr);

// Function to translate assembly instruction and arguments into index for the Index Translation Table
MACRO_IDX inst2ittIdx(INSTR instr, uint imm, string vfuxType, string dst, string src);

// Function from getting the data for the NMC instruction, either from
// the assembly file, or signal to be read when the loop is executed
bool getInstData(istringstream &aistream, nmcInst *currInstr);

// Function to get data from file at execution time, when the loop is executed
bool getDataFromFile (ifstream &dataFile, nmcInst *currInstr);

// Function to get address from file at execution time, when the loop is executed
bool getAddrFromFile (ifstream &addrFile, uint64_t *addr);