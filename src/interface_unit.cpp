/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the Interface Unit for SoftSIMD.
 *
 */

#include "interface_unit.h"

void interface_unit::comb_method() {
    // Separate row address in MSB and rest
    sc_uint<ROW_BITS> row = row_addr->read();
    sc_uint<COL_BITS> col = col_addr->read();
    bool rmsb = row.range(ROW_BITS - 1, ROW_BITS - 1);
    sc_uint<ROW_BITS - 1> rlsb = row.range(ROW_BITS - 2, 0);
    sc_uint<ROW_BITS - 1 + COL_BITS> rowcol_addr;
    rowcol_addr.range(ROW_BITS - 1 + COL_BITS - 1, COL_BITS) = rlsb;
    rowcol_addr.range(COL_BITS - 1, 0) = col;
    sc_uint<RF_SEL_BITS> rf_sel = rowcol_addr.range(RF_SEL_BITS + RF_ADDR_BITS - 1, RF_ADDR_BITS);

    // Defaults
    rf_access->write(false);
    decode_en->write(false);
    data_out->write(0);
#if HW_LOOP
    loop_reg_en->write(false);
    loop_sta_addr->write(0);
    loop_end_addr->write(0);
    loop_num_iter->write(0);
#endif

#if STATIC_CSD_LEN
    csd_len_en->write(false);
    csd_len->write(0);
#endif

    if (pim_mode) {
        if (!rmsb) { // PIM mode, enable instruction decoding if RD/WR command and Row MSB = 0
            if (RD || WR) {
                decode_en->write(true);
            }
        } else  {
            // Write to RFs
            if (rf_sel >= uint(RF_SEL::IB) && rf_sel < uint(RF_SEL::VWR)) {  // Write to RFs
                data_out->write(DQ);
                rf_access->write(WR && !RD);
#if HW_LOOP
            // Write to the HW loop registers
            // 64-bit DQ -> 16 unused bits | 16 bits for start address | 16 bits for end address | 16 bits for the number of iterations
            } else if (rf_sel == uint(RF_SEL::LOOP_REG) && WR && !RD) {
                loop_reg_en->write(true);
                loop_sta_addr->write((DQ->read() & 0x0000FFFF00000000) >> 32);
                loop_end_addr->write((DQ->read() & 0x00000000FFFF0000) >> 16);
                loop_num_iter->write((DQ->read() & 0x000000000000FFFF));
#endif
#if STATIC_CSD_LEN
            } else if (rf_sel == uint(RF_SEL::CSD_LEN) && WR && !RD) {
                csd_len_en->write(true);
                csd_len->write(DQ->read());
#endif
            }
        }
    }
}
