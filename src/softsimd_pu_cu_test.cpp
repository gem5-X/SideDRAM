/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD Processing Unit, testing the RTL of the control unit.
 *
 */

#include "softsimd_pu_cu_test.h"

#if MIXED_SIM

bool logic_to_bool (sc_logic in) {
    if (in == SC_LOGIC_1)   return true;
    else                    return false;
}

sc_logic bool_to_logic (bool in) {
    if (in) return SC_LOGIC_1;
    else    return SC_LOGIC_0;
}

void softsimd_pu::comb_method() {
    uint i, j;
    sc_bv<MASK_BITS> scalar_aux;        // Holds parsed scalar
    sc_bv<WORD_BITS> scalar_rep;        // Holds replicated scalar in SystemC bit-vector
    uint64_t scalar[WORD_64B];          // Replicated scalar in 64-bit uint

    sc_lv<WORD_BITS> vwr_muxed_aux;                 // Holds output of vwr_muxed
    uint64_t from_vwr_narrow[VWR_NUM][WORD_64B];    // Holds parsed output of VWR muxes
    sc_bv<VWR_BITS> to_vwr_wide;                    // Holds parsed input to VWR
    sc_bv<WORD_BITS> to_vwr_narrow;                 // Holds parsed input to VWR muxes
    sc_lv<WORD_BITS> allzs(SC_LOGIC_Z);

    sc_bv<64> parse_aux;                // Used for parsing

    // Replicate the scalar from the SRF
    if (sa_op2_from == MUX::SRF) {
        for (i = 0; i < MASK_64B; i++) {
            parse_aux = srf_out[i];
            if ((i+1)*64-1 < MASK_BITS) {
                scalar_aux.range((i+1)*64-1, i*64) = parse_aux;
            } else {
                scalar_aux.range(MASK_BITS-1, i*64) = parse_aux;
            }
        }
        // Replicate scalar
        for (i = 0; i < MASK_PER_WORD; i++) {
            scalar_rep.range((i+1)*MASK_BITS-1,i*MASK_BITS) = scalar_aux;
        }
        // Parse replicated mask to 64-bit uint
        for (i = 0; i < WORD_64B; i++) {
            if ((i+1)*64-1 < WORD_BITS) {
                parse_aux.range(63,0) = scalar_rep.range((i+1)*64-1, i*64);
            } else {
                parse_aux = 0;
                parse_aux.range(WORD_BITS%64-1,0) = scalar_rep.range(WORD_BITS-1, i*64);
            }
            scalar[i] = parse_aux.to_uint64();
        }
    }

    // Parse between vwr_muxed (resolved vector) and the multiplexer input
    for (i = 0; i < VWR_NUM; i++) {
        vwr_muxed_aux = vwr_muxed[i];
        for (j = 0; j < WORD_64B; j++) {
            if ((j+1)*64-1 < WORD_BITS) {
                parse_aux.range(63,0) = vwr_muxed_aux.range((j+1)*64-1, j*64);
            } else {
                parse_aux = 0;
                parse_aux.range(WORD_BITS%64-1,0) = vwr_muxed_aux.range(WORD_BITS-1, j*64);
            }
            from_vwr_narrow[i][j] = parse_aux.to_uint64();
        }
    }

    // Tile shuffler input multiplexer
    switch (ts_shf_from) {
        case MUX::VWR0:     ts_inout = vwr_inout[0];    break;
        case MUX::VWR1:     ts_inout = vwr_inout[1];    break;
#if !DUAL_BANK_INTERFACE
        case MUX::VWR2:     ts_inout = vwr_inout[2];    break;
        case MUX::VWR3:     ts_inout = vwr_inout[3];    break;
#if VWR_DRAM_CLK == 1
#ifdef __SYNTHESIS__
        case MUX::DRAM:     ts_inout = dram_in;         break;
#else
        case MUX::DRAM:     ts_inout = dram_bus;        break;
#endif
#endif
#else
#if VWR_DRAM_CLK == 1
#ifdef __SYNTHESIS__
        case MUX::EVEN_BANK:    ts_inout = even_in;     break;
        case MUX::ODD_BANK:     ts_inout = odd_in;      break;
#else
        case MUX::EVEN_BANK:    ts_inout = even_bus;    break;
        case MUX::ODD_BANK:     ts_inout = odd_bus;     break;
#endif
#endif
#endif // !DUAL_BANK_INTERFACE
        default:            ts_inout = vwr_inout[0];    break;
    }

    // Default VWR inputs
    for (i = 0; i < VWR_NUM; i++) {
#ifdef __SYNTHESIS__
        vwr_inout[i] = dram_in;
#else
        vwr_inout[i] = dram_bus;
#endif
        vwr_muxed[i] = allzs;
    }

    // VWR input multiplexers. Depending on the origin, using word interface or very wide interface
    for (i = 0; i < VWR_NUM; i++) {
        switch (vwr_from[i]) {  // Maybe better multiplex and then parse?
//            case MUX_EXT:
//                if(vwr_wr_nrd[i])   DQ_burst_en = true; // If writing to VWR, enable tristate buffer
//                vwr_inout[i] = DQ_to_vwr;
//            break;
            case MUX::PM:
                for (j=0; j<WORD_64B; j++) {
                    if (j < WORD_64B-1) to_vwr_narrow.range((j+1)*64-1, j*64) = pm_out[j];
                    else                to_vwr_narrow.range(WORD_BITS-1, j*64) = pm_out[j];
                }
                vwr_muxed[i] = to_vwr_narrow;
            break;
            case MUX::TILESH:   vwr_inout[i] = ts_inout;        break;
            case MUX::VWR0:     vwr_inout[i] = vwr_inout[0];    break;
            case MUX::VWR1:     vwr_inout[i] = vwr_inout[1];    break;
#if !DUAL_BANK_INTERFACE
            case MUX::VWR2:     vwr_inout[i] = vwr_inout[2];    break;
            case MUX::VWR3:     vwr_inout[i] = vwr_inout[3];    break;
#endif
//            case MUX_REG_0:
//                for (j=0; j<WORD_64B; j++) {
//                    if (j < WORD_64B-1) to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[0][j];
//                    else                to_vwr_narrow.range(WORD_BITS-1, j*64) = R_out[0][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX_REG_1:
//                for (j=0; j<WORD_64B; j++) {
//                    if (j < WORD_64B-1) to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[1][j];
//                    else                to_vwr_narrow.range(WORD_BITS-1, j*64) = R_out[1][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX_REG_2:
//                for (j=0; j<WORD_64B; j++) {
//                    if (j < WORD_64B-1) to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[2][j];
//                    else                to_vwr_narrow.range(WORD_BITS-1, j*64) = R_out[2][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
            case MUX::R3:
                for (j=0; j<WORD_64B; j++) {
                    if (j < WORD_64B-1) to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[3][j];
                    else                to_vwr_narrow.range(WORD_BITS-1, j*64) = R_out[3][j];
                }
                vwr_muxed[i] = to_vwr_narrow;
            break;
#if VWR_DRAM_CLK > 1
#if DUAL_BANK_INTERFACE
#ifdef __SYNTHESIS__
            case MUX::EVEN_BANK:    vwr_dram_if[i] = even_in;  break;
            case MUX::ODD_BANK:     vwr_dram_if[i] = odd_in;   break;
#else
            case MUX::EVEN_BANK:    vwr_dram_if[i] = even_bus; break;
            case MUX::ODD_BANK:     vwr_dram_if[i] = odd_bus;  break;
#endif
#else // DUAL_BANK_INTERFACE
#ifdef __SYNTHESIS__
            case MUX::DRAM:     vwr_dram_if[i] = dram_in;   break;
#else
            case MUX::DRAM:     vwr_dram_if[i] = dram_bus;  break;
#endif
#endif // DUAL_BANK_INTERFACE
#else // VWR_DRAM_CLK > 1
#if DUAL_BANK_INTERFACE
#ifdef __SYNTHESIS__
            case MUX::EVEN_BANK:    vwr_inout[i] = even_in;    break;
            case MUX::ODD_BANK:     vwr_inout[i] = odd_in;     break;
#else
            case MUX::EVEN_BANK:    vwr_inout[i] = even_bus;    break;
            case MUX::ODD_BANK:     vwr_inout[i] = odd_bus;     break;
#endif
#else // DUAL_BANK_INTERFACE
#ifdef __SYNTHESIS__
            case MUX::DRAM:     vwr_inout[i] = dram_in;     break;
            default:            vwr_inout[i] = dram_in;     break;
#else
            case MUX::DRAM:     vwr_inout[i] = dram_bus;    break;
            default:            vwr_inout[i] = dram_bus;    break;
#endif
#endif // DUAL_BANK_INTERFACE
#endif // VWR_DRAM_CLK > 1
        }
    }

    // Registers input multiplexers
//    for (i = 0; i < REG_NUM; i++) {
//        for (j = 0; j < WORD_64B; j++) {
//            switch (reg_from[i]) {
//                case MUX_STAGE1:    R_in[i][j] = sa_out[j];             break;
//                case MUX_STAGE2:    R_in[i][j] = pm_out[j];             break;
//                case MUX_VWR_0:     R_in[i][j] = from_vwr_narrow[0][j]; break;
//                case MUX_VWR_1:     R_in[i][j] = from_vwr_narrow[1][j]; break;
//#if !DUAL_BANK_INTERFACE
//                case MUX_VWR_2:     R_in[i][j] = from_vwr_narrow[2][j]; break;
//                case MUX_VWR_3:     R_in[i][j] = from_vwr_narrow[3][j]; break;
//#endif
//                case MUX_REG_0:     R_in[i][j] = R_out[0][j];           break;
//                case MUX_REG_1:     R_in[i][j] = R_out[1][j];           break;
//                case MUX_REG_2:     R_in[i][j] = R_out[2][j];           break;
//                case MUX_REG_3:     R_in[i][j] = R_out[3][j];           break;
//                default:            R_in[i][j] = sa_out[j];             break;
//            }
//        }
//    }
    // R0
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[0]) {
            case MUX::VWR0: R_in[0][j] = from_vwr_narrow[0][j]; break;
            case MUX::VWR1: R_in[0][j] = from_vwr_narrow[1][j]; break;
#if !DUAL_BANK_INTERFACE
            case MUX::VWR2: R_in[0][j] = from_vwr_narrow[2][j]; break;
            case MUX::VWR3: R_in[0][j] = from_vwr_narrow[3][j]; break;
#endif
            default:        R_in[0][j] = R_out[0][j];           break;
        }
    }
    // R1
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[1]) {
            case MUX::SA:   R_in[1][j] = sa_out[j];     break;
            default:        R_in[1][j] = R_out[1][j];   break;
        }
    }
    // R2
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[2]) {
            case MUX::SA:   R_in[2][j] = sa_out[j];     break;
            default:        R_in[2][j] = R_out[2][j];   break;
        }
    }
    // R3
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[3]) {
            case MUX::SA:   R_in[3][j] = sa_out[j];     break;
            case MUX::PM:   R_in[3][j] = pm_out[j];     break;
            default:        R_in[3][j] = R_out[3][j];   break;
        }
    }

    // SRF input multiplexer
    for (i = 0; i < MASK_64B; i++) {
        switch (srf_wr_from) {
            case MUX::EXT:  srf_in[i] = cu_data_out;    break;
            default:        srf_in[i] = cu_data_out;    break;
        }
    }

    // Mask RF input multiplexer
    for (i = 0; i < MASK_64B; i++) {
        switch (mrf_wr_from) {
            case MUX::EXT:  mrf_in[i] = cu_data_out;    break;
            default:        mrf_in[i] = cu_data_out;    break;
        }
    }

    // CSD RF input multiplexer
    for (i = 0; i < CSD_64B; i++) {
        switch (csdrf_wr_from) {
            case MUX::EXT:  csdrf_in[i] = cu_data_out;    break;
            default:        csdrf_in[i] = cu_data_out;    break;
        }
    }

    // Shift & Add input multiplexers
    for (i = 0; i < WORD_64B; i++) {
        switch (sa_op1_from) {  // Input to shifter (A>>)
            case MUX::ZERO: sa_op1[i] = 0;                      break;
            case MUX::VWR0: sa_op1[i] = from_vwr_narrow[0][i];  break;
            case MUX::VWR1: sa_op1[i] = from_vwr_narrow[1][i];  break;
#if !DUAL_BANK_INTERFACE
            case MUX::VWR2: sa_op1[i] = from_vwr_narrow[2][i];  break;
            case MUX::VWR3: sa_op1[i] = from_vwr_narrow[3][i];  break;
#endif
//            case MUX::R0:   sa_op1[i] = R_out[0][i];            break;
//            case MUX::R1:   sa_op1[i] = R_out[1][i];            break;
//            case MUX::R2:   sa_op1[i] = R_out[2][i];            break;
            case MUX::R3:   sa_op1[i] = R_out[3][i];            break;
            default:        sa_op1[i] = R_out[3][i];            break;
        }

        switch (sa_op2_from) {  // Input to adder/sub (+/-B)
            case MUX::SRF:  sa_op2[i] = scalar[i];              break;
//            case MUX::VWR0: sa_op2[i] = from_vwr_narrow[0][i];  break;
//            case MUX::VWR1: sa_op2[i] = from_vwr_narrow[1][i];  break;
//#if !DUAL_BANK_INTERFACE
//            case MUX::VWR2: sa_op2[i] = from_vwr_narrow[2][i];  break;
//            case MUX::VWR3: sa_op2[i] = from_vwr_narrow[3][i];  break;
//#endif
            case MUX::R0:   sa_op2[i] = R_out[0][i];            break;
//            case MUX::R1:   sa_op2[i] = R_out[1][i];            break;
//            case MUX::R2:   sa_op2[i] = R_out[2][i];            break;
//            case MUX::R3:   sa_op2[i] = R_out[3][i];            break;
            default:        sa_op2[i] = R_out[0][i];            break;
        }
    }


    // Pack & Mask input multiplexers
//    for (i = 0; i < WORD_64B; i++) {
//        switch (pm_op1_from) {
//            case MUX_STAGE1:    pm_w1[i] = sa_out[i];               break;
//            case MUX_VWR_0:     pm_w1[i] = from_vwr_narrow[0][i];   break;
//            case MUX_VWR_1:     pm_w1[i] = from_vwr_narrow[1][i];   break;
//#if !DUAL_BANK_INTERFACE
//            case MUX_VWR_2:     pm_w1[i] = from_vwr_narrow[2][i];   break;
//            case MUX_VWR_3:     pm_w1[i] = from_vwr_narrow[3][i];   break;
//#endif
//            case MUX_REG_0:     pm_w1[i] = R_out[0][i];             break;
//            case MUX_REG_1:     pm_w1[i] = R_out[1][i];             break;
//            case MUX_REG_2:     pm_w1[i] = R_out[2][i];             break;
//            case MUX_REG_3:     pm_w1[i] = R_out[3][i];             break;
//            default:            pm_w1[i] = sa_out[i];               break;
//        }
//
//        switch (pm_op2_from) {
//            case MUX_STAGE1:    pm_w2[i] = sa_out[i];               break;
//            case MUX_VWR_0:     pm_w2[i] = from_vwr_narrow[0][i];   break;
//            case MUX_VWR_1:     pm_w2[i] = from_vwr_narrow[1][i];   break;
//#if !DUAL_BANK_INTERFACE
//            case MUX_VWR_2:     pm_w2[i] = from_vwr_narrow[2][i];   break;
//            case MUX_VWR_3:     pm_w2[i] = from_vwr_narrow[3][i];   break;
//#endif
//            case MUX_REG_0:     pm_w2[i] = R_out[0][i];             break;
//            case MUX_REG_1:     pm_w2[i] = R_out[1][i];             break;
//            case MUX_REG_2:     pm_w2[i] = R_out[2][i];             break;
//            case MUX_REG_3:     pm_w2[i] = R_out[3][i];             break;
//            default:            pm_w2[i] = sa_out[i];               break;
//        }
//    }
    for (i = 0; i < WORD_64B; i++) {
        pm_w1[i] = R_out[1][i];
        pm_w2[i] = R_out[2][i];
    }

#if !DUAL_BANK_INTERFACE
    // DRAM input multiplexer
    switch (dram_from) {
#if VWR_DRAM_CLK > 1
        case MUX::VWR0:     to_dram = vwr_dram_if[0];     break;
        case MUX::VWR1:     to_dram = vwr_dram_if[1];     break;
        case MUX::VWR2:     to_dram = vwr_dram_if[2];     break;
        case MUX::VWR3:     to_dram = vwr_dram_if[3];     break;
        default:            to_dram = vwr_dram_if[0];     break;
#else
        case MUX::VWR0:     to_dram = vwr_inout[0];     break;
        case MUX::VWR1:     to_dram = vwr_inout[1];     break;
        case MUX::VWR2:     to_dram = vwr_inout[2];     break;
        case MUX::VWR3:     to_dram = vwr_inout[3];     break;
        default:            to_dram = vwr_inout[0];     break;
#endif
    }
#endif

#if __SYNTHESIS__
#if !DUAL_BANK_INTERFACE
    dram_out = to_dram;
#else // DUAL_BANK_INTERFACE
#if VWR_DRAM_CLK > 1
    even_out = vwr_dram_if[0];
    odd_out = vwr_dram_if[1];
#else // VWR_DRAM_CLK > 1
    even_out = vwr_inout[0];
    odd_out = vwr_inout[1];
#endif // VWR_DRAM_CLK > 1
#endif // DUAL_BANK_INTERFACE
#endif
}

void softsimd_pu::adaptation_method() {
    ib_in = (uint32_t) cu_data_out;
}

void softsimd_pu::mixed_method() {
    uint i;

    // Inputs (bool and sc_uint to sc_logic and sc_lv)
    clk_mixed = bool_to_logic(clk);
    rst_mixed = bool_to_logic(rst);
    RD_mixed = bool_to_logic(RD);
    WR_mixed = bool_to_logic(WR);
    ACT_mixed = bool_to_logic(ACT);
    AB_mode_mixed = bool_to_logic(AB_mode);
    pim_mode_mixed = bool_to_logic(pim_mode);
    sc_uint<BANK_BITS> bank_aux = bank_addr;    bank_addr_mixed = bank_aux;
    sc_uint<ROW_BITS> row_aux = row_addr;       row_addr_mixed = row_aux;
    sc_uint<COL_BITS> col_aux = col_addr;       col_addr_mixed = col_aux;
    uint64_t DQ_aux = DQ;                       DQ_mixed = DQ_aux;
    uint64_t macroinstr_aux = macroinstr;       macroinstr = macroinstr_aux;
    uint64_t csdrf_out_aux = csdrf_out[0];      csdrf_out_mixed[0] = csdrf_out_aux;

    // Outputs (sc_logic and sc_lv to bool and sc_uint)
    sc_lv<32> PC_aux = PC_out_mixed;                    PC = PC_aux.to_uint();
    sc_lv<64> cu_data_out_aux = cu_data_out_mixed;      cu_data_out = cu_data_out_aux.to_uint64();
    ib_wr_en = logic_to_bool(ib_wr_en_mixed);
    sc_lv<32> ib_wr_addr_aux = ib_wr_addr_mixed;        ib_wr_addr = ib_wr_addr_aux.to_uint();
    ts_in_en = logic_to_bool(ts_in_en_mixed);
    ts_out_en = logic_to_bool(ts_out_en_mixed);
    sc_lv<32> ts_out_start_aux = ts_out_start_mixed;    ts_out_start = ts_out_start_aux.to_uint();
    sc_lv<32> ts_mode_aux = ts_mode_mixed;              ts_mode = ts_mode_aux.to_uint();
    sc_lv<32> ts_shf_from_aux = ts_shf_from_mixed;      ts_shf_from = (MUX) ts_shf_from_aux.to_uint();
    sc_lv<64> vwr_mask_aux;
    sc_lv<32> vwr_idx_aux;
    sc_lv<32> vwr_from_aux;
    for (i=0; i<VWR_NUM; i++) {
        vwr_enable[i] = logic_to_bool(vwr_enable_mixed[i]);
        vwr_wr_nrd[i] = logic_to_bool(vwr_wr_nrd_mixed[i]);
        vwr_d_nm[i] = logic_to_bool(vwr_d_nm_mixed[i]);
        vwr_mask_en[i] = logic_to_bool(vwr_mask_en_mixed[i]);
        vwr_mask_aux = vwr_mask_mixed[i];   vwr_mask[i] = vwr_mask_aux.to_uint64();
        vwr_idx_aux = vwr_idx_mixed[i];     vwr_idx[i] = vwr_idx_aux.to_uint();
        vwr_from_aux = vwr_from_mixed[i];   vwr_from[i] = (MUX) vwr_from_aux.to_uint();
#if VWR_DRAM_CLK > 1
        vwr_dram_d_nm[i] = logic_to_bool(vwr_dram_d_nm_mixed[i]);
#endif
    }
#if VWR_DRAM_CLK > 1
    sc_lv<32> vwr_dram_idx_aux = vwr_dram_idx_mixed;            vwr_dram_idx = vwr_dram_idx_aux.to_uint();
#endif
    sc_lv<32> reg_from_aux;
    for (i=0; i<REG_NUM; i++) {
        reg_en[i] = logic_to_bool(reg_en_mixed[i]);
        reg_from_aux = reg_from_mixed[i];                       reg_from[i] = (MUX) reg_from_aux.to_uint();
    }
    sc_lv<32> srf_rd_addr_aux = srf_rd_addr_mixed;              srf_rd_addr = srf_rd_addr_aux.to_uint();
    srf_wr_en = logic_to_bool(srf_wr_en_mixed);
    sc_lv<32> srf_wr_addr_aux = srf_wr_addr_mixed;              srf_wr_addr = srf_wr_addr_aux.to_uint();
    sc_lv<32> srf_wr_from_aux = srf_wr_from_mixed;              srf_wr_from = (MUX) srf_wr_from_aux.to_uint();
    sc_lv<32> mrf_rd_addr_aux = mrf_rd_addr_mixed;              mrf_rd_addr = mrf_rd_addr_aux.to_uint();
    mrf_wr_en = logic_to_bool(mrf_wr_en_mixed);
    sc_lv<32> mrf_wr_addr_aux = mrf_wr_addr_mixed;              mrf_wr_addr = mrf_wr_addr_aux.to_uint();
    sc_lv<32> mrf_wr_from_aux = mrf_wr_from_mixed;              mrf_wr_from = (MUX) mrf_wr_from_aux.to_uint();
    sc_lv<32> csdrf_rd_addr_aux = csdrf_rd_addr_mixed;          csdrf_rd_addr = csdrf_rd_addr_aux.to_uint();
    csdrf_wr_en = logic_to_bool(csdrf_wr_en_mixed);
    sc_lv<32> csdrf_wr_addr_aux = csdrf_wr_addr_mixed;          csdrf_wr_addr = csdrf_wr_addr_aux.to_uint();
    sc_lv<32> csdrf_wr_from_aux = csdrf_wr_from_mixed;          csdrf_wr_from = (MUX) csdrf_wr_from_aux.to_uint();
    sa_en = logic_to_bool(sa_en_mixed);
    sa_adder_en = logic_to_bool(sa_adder_en_mixed);
    sa_s_na = logic_to_bool(sa_s_na_mixed);
    sc_lv<32> sa_shift_aux = sa_shift_mixed;        sa_shift = sa_shift_aux.to_uint();
    sc_lv<32> sa_size_aux = sa_size_mixed;          sa_size = (SWSIZE) sa_size_aux.to_uint();
    sc_lv<32> sa_op1_from_aux = sa_op1_from_mixed;  sa_op1_from = (MUX) sa_op1_from_aux.to_uint();
    sc_lv<32> sa_op2_from_aux = sa_op2_from_mixed;  sa_op2_from = (MUX) sa_op2_from_aux.to_uint();
    pm_en = logic_to_bool(pm_en_mixed);
    sc_lv<32> pm_in_size_aux = pm_in_size_mixed;    pm_in_size = (SWSIZE) pm_in_size_aux.to_uint();
    sc_lv<32> pm_out_size_aux = pm_out_size_mixed;  pm_out_size = (SWSIZE) pm_out_size_aux.to_uint();
    sc_lv<32> pm_in_start_aux = pm_in_start_mixed;  pm_in_start = pm_in_start_aux.to_uint();
    sc_lv<32> pm_op_sel_aux = pm_op_sel_mixed;      pm_op_sel = (MASKOP) pm_op_sel_aux.to_uint();
    sc_lv<32> pm_shift_aux = pm_shift_mixed;        pm_shift = pm_shift_aux.to_uint();
    sc_lv<32> pm_op1_from_aux = pm_op1_from_mixed;  pm_op1_from = (MUX) pm_op1_from_aux.to_uint();
    sc_lv<32> pm_op2_from_aux = pm_op2_from_mixed;  pm_op2_from = (MUX) pm_op2_from_aux.to_uint();
#if DUAL_BANK_INTERFACE
    even_out_en = logic_to_bool(even_out_en_mixed);
    odd_out_en = logic_to_bool(odd_out_en_mixed);
#else
    dram_out_en = logic_to_bool(dram_out_en_mixed);
    sc_lv<32> dram_from_aux = dram_from_mixed;      dram_from = (MUX) dram_from_aux.to_uint();
#endif
}

#endif
