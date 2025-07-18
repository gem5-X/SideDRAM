#include "systemc.h"

#include "../cnm_base.h"

#include <random>

#if MIXED_SIM

#define TEST_IDLE               0
#define TEST_WRF_CSDLEN_LOOP    1
#define TEST_WRF_IB             2
#define TEST_WRF_CSD            3
#define TEST_NOP                4
#define TEST_RLB                5
#define TEST_WLB                6
#define TEST_VMV                7
#define TEST_SHIFT              8
#define TEST_ADD                9
#define TEST_MUL                10
#define TEST_PACK               11
#define TEST_EXIT               12

#define TESTED_INSTRS           TEST_EXIT
#define TESTED_CYCLES           100
#define TEST_LOOP_LEN           32
#define TESTED_CSD_LEN          8

#define POSSIBLE_REPACKS        20
const SWREPACK random_repack[POSSIBLE_REPACKS] = {
    SWREPACK::INV,
    SWREPACK::B3_B3,
    SWREPACK::B4_B4,
    SWREPACK::B6_B6,
    SWREPACK::B8_B8,
    SWREPACK::B12_B12,
    SWREPACK::B16_B16,
    SWREPACK::B24_B24,
    SWREPACK::B3_B4,
    SWREPACK::B4_B6,
    SWREPACK::B6_B8,
    SWREPACK::B8_B12,
    SWREPACK::B12_B16,
    SWREPACK::B16_B24,
    SWREPACK::B24_B16,
    SWREPACK::B16_B12,
    SWREPACK::B12_B8,
    SWREPACK::B8_B6,
    SWREPACK::B6_B4,
    SWREPACK::B4_B3,
};

class cp_driver: public sc_module {
public:

    sc_out<sc_logic>            rst;
    sc_out<sc_logic>            RD;         // DRAM read command
    sc_out<sc_logic>            WR;         // DRAM write command
    sc_out<sc_logic>            ACT;        // DRAM activate command
//    sc_out<sc_logic>              RSTB;       //
    sc_out<sc_logic>            AB_mode;    // Signals if the All-Banks mode is enabled
    sc_out<sc_logic>            pim_mode;   // Signals if the PIM mode is enabled
    sc_out<sc_lv<BANK_BITS> >   bank_addr;  // Address of the bank
    sc_out<sc_lv<ROW_BITS> >    row_addr;   // Address of the bank row
    sc_out<sc_lv<COL_BITS> >    col_addr;   // Address of the bank column
    sc_out<sc_lv<64> >          DQ;         // Data input from DRAM controller (output makes no sense)

    SC_HAS_PROCESS(cp_driver);
    cp_driver(sc_module_name name_) : sc_module(name_) {
        SC_THREAD(driver_thread);
    }

    void driver_thread();
    void write_RF_row_col (sc_uint<ROW_BITS> &row_addr, sc_uint<COL_BITS> &col_addr,
                            sc_uint<RF_SEL_BITS> &rf_sel, sc_uint<RF_ADDR_BITS> &rf_addr);
    uint64_t loop_to_dq(uint64_t start, uint64_t end, uint64_t iter);
    uint64_t generate_random_csd(uint csd_len, std::mt19937 &gen);
};

#endif // MIXED_SIM