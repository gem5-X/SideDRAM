#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <cmath>
#include <random>

#include "half.hpp"
#include "../../src/defs.h"
#include "../../src/opcodes.h"

using namespace std;

// Definition of the address mapping
enum class Level : int {Channel, Rank, BankGroup, Bank, Row, Column, MAX};
string level_str[int(Level::MAX)] = {"Ch", "Ra", "Bg", "Ba", "Ro", "Co"};
int addr_bits[int(Level::MAX)] = {CHANNEL_BITS, RANK_BITS, BG_BITS, BANK_BITS, ROW_BITS, COL_BITS};
Level addr_map[int(Level::MAX)] = {Level::Channel, Level::Column, Level::Rank,
                        Level::BankGroup, Level::Bank, Level::Row};
int global_offset = GLOBAL_OFFSET;

#define COLPERROW (1 << addr_bits[int(Level::Column)])

enum KERNEL_VALUES {
    EWARW,  // Element-Wise Addition Row-Wise
    EWACW,  // Element-Wise Addition Column-Wise
    EWMRW,  // Element-Wise Multiplication Row-Wise
    EWMCW,  // Element-Wise Multiplication Column-Wise
    DP,     // Dot Product
    MMS,    // Matrix Multiplication using SRF
    CCWWR,  // Convolution Channel-Wise wieht Weight Reuse
};

const std::map<std::string, uint8_t> KERNEL = {
    { "EWARW", EWARW },
    { "EWACW", EWACW },
    { "EWMRW", EWMRW },
    { "EWMCW", EWMCW },
    { "DP", DP },
    { "MMS", MMS },
    { "CCWWR", CCWWR },
};

// Row-wise mapping of element-wise vector addition, R-limited
void mapEWAdditionRowWiseRLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float **op1, float **op2, int V, int n);

// Row-wise mapping of element-wise vector addition, C-limited
void mapEWAdditionRowWiseCLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float **op1, float **op2, int V, int n);

// Column-wise mapping of element-wise vector addition, R-limited
void mapEWAdditionColWiseRLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float **op1, float **op2, int V, int n);

// Column-wise mapping of element-wise vector addition, C-limited
void mapEWAdditionColWiseCLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float **op1, float **op2, int V, int n);

// Mapping of dot product, R-limited
void mapDotProductRLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float **op1, float **op2, int V, int n);

// Mapping of dot product, C-limited
void mapDotProductCLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float **op1, float **op2, int V, int n);

// Mapping of matrix multriplication using the SRF, R-limited
void mapMatrixMultSrfRLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float *op1, float *op2, int m, int n, int q);

// Mapping of matrix multriplication using the SRF, C-limited
void mapMatrixMultSrfCLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float *op1, float *op2, int m, int n, int q);

// Channel-wise mapping of convolution with weight reuse, R-limited
void mapConvCWWRRLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float *act, float *weight, float *bias,
                                int ci, int wi, int hi, int k, int co, int wo, int ho, int stride);

// Channel-wise mapping of convolution with weight reuse, C-limited
void mapConvCWWRCLim (ofstream &assembly, ofstream &dataFile, ofstream &addrFile,
                                float *act, float *weight, float *bias,
                                int ci, int wi, int hi, int k, int co, int wo, int ho, int stride); 

// Function for building addresses
uint64_t build_addr(vector<uint64_t> addr_vec);

// Function for getting the bank index from an address
uint8_t get_bank(uint64_t addr);

// Function for getting the row index from an address
uint16_t get_row(uint64_t addr);

// Function for getting the column index from an address
uint16_t get_col(uint64_t addr);

// Function for computing partial dot products
float partDotProduct(float *op1, float *op2, int n);

// Function for computing a partial dot product within a matrix multiplication
float partMatMultDP(float *op1, float *op2, int m, int n, int q, int rowOp1, int colOp2, int len);

// Function for computing partial convolution resulting in a pixel
float partPixConv(float *act, float *weight, float *bias, int stride, int k,
                    int ci, int hi, int wi, int ck, int hk, int wk, int co, int ho, int wo);