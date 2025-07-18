/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementation of the SoftSIMD Processing Unit.
 *
 */

#include "softsimd_pu.h"

#if !MIXED_SIM

#ifndef __SYNTHESIS__

void softsimd_pu::comb_method() {
    uint i, j;
    sc_bv<MASK_64B*64> scalar_aux;      // Holds parsed scalar
    sc_bv<MASK_BITS> scalar_to_rep;     // Holds scalar before replication
    sc_bv<WORD_64B*64> scalar_rep('0'); // Holds replicated scalar in SystemC bit-vector
    uint64_t scalar[WORD_64B];          // Replicated scalar in 64-bit uint

    sc_lv<WORD_BITS> vwr_muxed_aux;             // Holds output of vwr_muxed
    sc_lv<WORD_64B*64> vwr_muxed_toparse('0');      // Holds output of vwr_muxed
    uint64_t from_vwr_narrow[VWR_NUM][WORD_64B];    // Holds parsed output of VWR muxes
    sc_bv<VWR_BITS> to_vwr_wide;                    // Holds parsed input to VWR
    sc_bv<WORD_64B*64> to_vwr_narrow;               // Holds parsed input to VWR muxes
    sc_lv<VWR_BITS> allzs_long(SC_LOGIC_Z);
    sc_lv<WORD_BITS> allzs_short(SC_LOGIC_Z);

    sc_bv<64> parse_aux;                // Used for parsing

    // Replicate the scalar from the SRF
    if (sa_op2_from == MUX::SRF) {
        for (i = 0; i < MASK_64B; i++) {
            parse_aux = srf_out[i];
            scalar_aux.range((i+1)*64-1, i*64) = parse_aux;
        }
        scalar_to_rep = scalar_aux(MASK_BITS-1,0);
        // Replicate scalar
        for (i = 0; i < MASK_PER_WORD; i++) {
            scalar_rep.range((i+1)*MASK_BITS-1,i*MASK_BITS) = scalar_to_rep;
        }
        // Parse replicated mask to 64-bit uint
        for (i = 0; i < WORD_64B; i++) {
            parse_aux.range(63,0) = scalar_rep.range((i+1)*64-1, i*64);
            scalar[i] = parse_aux.to_uint64();
        }
    }

    // Parse between vwr_muxed (resolved vector) and the multiplexer input
    for (i = 0; i < VWR_NUM; i++) {
        vwr_muxed_aux = vwr_muxed[i];
        vwr_muxed_toparse.range(WORD_BITS-1, 0) = vwr_muxed_aux;
        for (j = 0; j < WORD_64B; j++) {
            parse_aux.range(63,0) = vwr_muxed_toparse.range((j+1)*64-1, j*64);
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
        case MUX::DRAM:     ts_inout = dram_bus;        break;
#endif  // VWR_DRAM_CLK == 1
#else   // DUAL_BANK_INTERFACE
#if VWR_DRAM_CLK == 1
        case MUX::EVEN_BANK:    ts_inout = even_bus;    break;
        case MUX::ODD_BANK:     ts_inout = odd_bus;     break;
#endif // VWR_DRAM_CLK == 1
#endif // !DUAL_BANK_INTERFACE
        default:            ts_inout = allzs_long;    break;
    }

    // Default VWR inputs
    for (i = 0; i < VWR_NUM; i++) {
        vwr_inout[i] = allzs_long;
        vwr_muxed[i] = allzs_short;
    }

    // VWR input multiplexers. Depending on the origin, using word interface or very wide interface
    for (i = 0; i < VWR_NUM; i++) {
        switch (vwr_from[i]) {  // Maybe better multiplex and then parse?
//            case MUX_EXT:
//                if(vwr_wr_nrd[i])   DQ_burst_en = true; // If writing to VWR, enable tristate buffer
//                vwr_inout[i] = DQ_to_vwr;
//            break;
            case MUX::SA:
                for (j=0; j<WORD_64B; j++) {
                    to_vwr_narrow.range((j+1)*64-1, j*64) = sa_out[j];
                }
                vwr_muxed[i] = to_vwr_narrow;
            break;
            case MUX::PM:
                for (j=0; j<WORD_64B; j++) {
                    to_vwr_narrow.range((j+1)*64-1, j*64) = pm_out[j];
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
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[0][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX_REG_1:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[1][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX_REG_2:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[2][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX::R3:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[3][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
#if VWR_DRAM_CLK > 1
#if DUAL_BANK_INTERFACE
            case MUX::EVEN_BANK:    vwr_dram_if[i] = even_bus;  break;
            case MUX::ODD_BANK:     vwr_dram_if[i] = odd_bus;   break;
#else // DUAL_BANK_INTERFACE
            case MUX::DRAM:         vwr_dram_if[i] = dram_bus;  break;
#endif // DUAL_BANK_INTERFACE
#else // VWR_DRAM_CLK > 1
#if DUAL_BANK_INTERFACE
            case MUX::EVEN_BANK:    vwr_inout[i] = even_bus;    break;
            case MUX::ODD_BANK:     vwr_inout[i] = odd_bus;     break;
#else // DUAL_BANK_INTERFACE
            case MUX::DRAM:         vwr_inout[i] = dram_bus;    break;
            default:                vwr_inout[i] = allzs_long;  break;
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
            default:        R_in[0][j] = from_vwr_narrow[0][j]; break;
        }
    }
    // R1
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[1]) {
            default:        R_in[1][j] = sa_out[j]; break;
        }
    }
    // R2
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[2]) {
            default:        R_in[2][j] = sa_out[j]; break;
        }
    }
    // R3
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[3]) {
            case MUX::SA:   R_in[3][j] = sa_out[j]; break;
            case MUX::PM:   R_in[3][j] = pm_out[j]; break;
            default:        R_in[3][j] = sa_out[j]; break;
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
//            case MUX::ZERO: sa_op1[i] = 0;                      break;
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
            case MUX::ZERO: sa_op2[i] = 0;                      break;
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
#endif  // VWR_DRAM_CLK > 1
    }
#endif  // DUAL_BANK_INTERFACE

}

#else   // __SYNTHESIS__

void softsimd_pu::comb_method() {
    uint i, j;
    sc_bv<MASK_64B*64> scalar_aux;      // Holds parsed scalar
    sc_bv<MASK_BITS> scalar_to_rep;     // Holds scalar before replication
    sc_bv<WORD_64B*64> scalar_rep('0'); // Holds replicated scalar in SystemC bit-vector
    uint64_t scalar[WORD_64B];          // Replicated scalar in 64-bit uint

    sc_lv<DRAM_BITS> to_dram;                       // Holds output to DRAM
    sc_lv<WORD_BITS> vwr_muxed_aux;                 // Holds output of vwr_muxed
    sc_lv<WORD_64B*64> vwr_muxed_toparse('0');      // Holds output of vwr_muxed
    uint64_t from_vwr_narrow[VWR_NUM][WORD_64B];    // Holds parsed output of VWR muxes
    sc_bv<VWR_BITS> to_vwr_wide;                    // Holds parsed input to VWR
    sc_bv<WORD_64B*64> to_vwr_narrow;               // Holds parsed input to VWR muxes
    sc_lv<VWR_BITS> allzs_long(SC_LOGIC_Z);
    sc_lv<WORD_BITS> allzs_short(SC_LOGIC_Z);

    sc_bv<64> parse_aux;                // Used for parsing

    // Make up for lack of tri-state buffers
#if !DUAL_BANK_INTERFACE
    dram_out->write(dram_in);
#else
    even_out->write(even_in);
    odd_out->write(odd_in);
#endif

    // Default signals
    to_dram = vwr_out[0];

    // Replicate the scalar from the SRF
    if (sa_op2_from == MUX::SRF) {
        for (i = 0; i < MASK_64B; i++) {
            parse_aux = srf_out[i];
            scalar_aux.range((i+1)*64-1, i*64) = parse_aux;
        }
        scalar_to_rep = scalar_aux(MASK_BITS-1,0);
        // Replicate scalar
        for (i = 0; i < MASK_PER_WORD; i++) {
            scalar_rep.range((i+1)*MASK_BITS-1,i*MASK_BITS) = scalar_to_rep;
        }
        // Parse replicated mask to 64-bit uint
        for (i = 0; i < WORD_64B; i++) {
            parse_aux.range(63,0) = scalar_rep.range((i+1)*64-1, i*64);
            scalar[i] = parse_aux.to_uint64();
        }
    }

    // Parse between vwr_muxed (resolved vector) and the multiplexer input
    for (i = 0; i < VWR_NUM; i++) {
        vwr_muxed_aux = vwr_muxed_out[i];
        vwr_muxed_toparse.range(WORD_BITS-1, 0) = vwr_muxed_aux;
        for (j = 0; j < WORD_64B; j++) {
            parse_aux.range(63,0) = vwr_muxed_toparse.range((j+1)*64-1, j*64);
            from_vwr_narrow[i][j] = parse_aux.to_uint64();
        }
    }

    // Tile shuffler input multiplexer
    switch (ts_shf_from) {
        case MUX::VWR0:     ts_in = vwr_out[0];     break;
        case MUX::VWR1:     ts_in = vwr_out[1];     break;
#if !DUAL_BANK_INTERFACE    
        case MUX::VWR2:     ts_in = vwr_out[2];     break;
        case MUX::VWR3:     ts_in = vwr_out[3];     break;
#if VWR_DRAM_CLK == 1
        case MUX::DRAM:     ts_in = dram_in;        break;
#endif  // VWR_DRAM_CLK == 1
#else   // DUAL_BANK_INTERFACE
#if VWR_DRAM_CLK == 1
        case MUX::EVEN_BANK:    ts_in = even_in;    break;
        case MUX::ODD_BANK:     ts_in = odd_in;     break;
#endif // VWR_DRAM_CLK == 1
#endif // !DUAL_BANK_INTERFACE
        default:            ts_in = vwr_out[0];     break;
    }

    // Default VWR inputs
    for (i = 0; i < VWR_NUM; i++) {
        vwr_in[i] = allzs_long;
        vwr_muxed_in[i] = allzs_short;
    }

    // VWR input multiplexers. Depending on the origin, using word interface or very wide interface
    for (i = 0; i < VWR_NUM; i++) {
        switch (vwr_from[i]) {  // Maybe better multiplex and then parse?
//            case MUX_EXT:
//                if(vwr_wr_nrd[i])   DQ_burst_en = true; // If writing to VWR, enable tristate buffer
//                vwr_in[i] = DQ_to_vwr;
//            break;
            case MUX::SA:
                for (j=0; j<WORD_64B; j++) {
                    to_vwr_narrow.range((j+1)*64-1, j*64) = sa_out[j];
                }
                vwr_muxed_in[i] = to_vwr_narrow;
            break;
            case MUX::PM:
                for (j=0; j<WORD_64B; j++) {
                    to_vwr_narrow.range((j+1)*64-1, j*64) = pm_out[j];
                }
                vwr_muxed_in[i] = to_vwr_narrow;
            break;
            case MUX::TILESH:   vwr_in[i] = ts_out;     break;
            case MUX::VWR0:     vwr_in[i] = vwr_out[0]; break;
            case MUX::VWR1:     vwr_in[i] = vwr_out[1]; break;
#if !DUAL_BANK_INTERFACE
            case MUX::VWR2:     vwr_in[i] = vwr_out[2]; break;
            case MUX::VWR3:     vwr_in[i] = vwr_out[3]; break;
#endif
//            case MUX_REG_0:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[0][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX_REG_1:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[1][j];
//                }
//                vwr_muxed[i] = to_vwr_narrow;
//            break;
//            case MUX_REG_2:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[2][j];
//                }
//                vwr_muxed_in[i] = to_vwr_narrow;
//            break;
//            case MUX::R3:
//                for (j=0; j<WORD_64B; j++) {
//                    to_vwr_narrow.range((j+1)*64-1, j*64) = R_out[3][j];
//                }
//                vwr_muxed_in[i] = to_vwr_narrow;
//            break;
#if VWR_DRAM_CLK > 1
#if DUAL_BANK_INTERFACE
            case MUX::EVEN_BANK:    vwr_dram_if[i] = even_in;   break;
            case MUX::ODD_BANK:     vwr_dram_if[i] = odd_in;    break;
#else // DUAL_BANK_INTERFACE
            case MUX::DRAM:         vwr_dram_if[i] = dram_in;   break;
#endif // DUAL_BANK_INTERFACE
#else // VWR_DRAM_CLK > 1
#if DUAL_BANK_INTERFACE
            case MUX::EVEN_BANK:    vwr_in[i] = even_in;        break;
            case MUX::ODD_BANK:     vwr_in[i] = odd_in;         break;
#else // DUAL_BANK_INTERFACE
            case MUX::DRAM:         vwr_in[i] = dram_in;        break;
            default:                vwr_in[i] = dram_in;        break;
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
            default:        R_in[0][j] = from_vwr_narrow[0][j]; break;
        }
    }
    // R1
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[1]) {
            default:        R_in[2][j] = sa_out[j]; break;
        }
    }
    // R2
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[2]) {
            default:        R_in[2][j] = sa_out[j]; break;
        }
    }
    // R3
    for (j = 0; j < WORD_64B; j++) {
        switch (reg_from[3]) {
            case MUX::SA:   R_in[3][j] = sa_out[j]; break;
            case MUX::PM:   R_in[3][j] = pm_out[j]; break;
            default:        R_in[3][j] = sa_out[j]; break;
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
//            case MUX::ZERO: sa_op1[i] = 0;                      break;
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
            case MUX::ZERO: sa_op2[i] = 0;                      break;
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
        case MUX::VWR0:     to_dram = vwr_dram_if[0];   break;
        case MUX::VWR1:     to_dram = vwr_dram_if[1];   break;
        case MUX::VWR2:     to_dram = vwr_dram_if[2];   break;
        case MUX::VWR3:     to_dram = vwr_dram_if[3];   break;
        default:            to_dram = vwr_dram_if[0];   break;
#else
        case MUX::VWR0:     to_dram = vwr_out[0];       break;
        case MUX::VWR1:     to_dram = vwr_out[1];       break;
        case MUX::VWR2:     to_dram = vwr_out[2];       break;
        case MUX::VWR3:     to_dram = vwr_out[3];       break;
        default:            to_dram = vwr_out[0];       break;
#endif  // VWR_DRAM_CLK > 1
    }
#endif  // DUAL_BANK_INTERFACE

// Make up for lack of tri-state buffers
#if !DUAL_BANK_INTERFACE
    if (dram_out_en)    dram_out->write(to_dram); 
#else // DUAL_BANK_INTERFACE
#if VWR_DRAM_CLK > 1
    even_out = vwr_dram_if[0];
    odd_out = vwr_dram_if[1];
#else // VWR_DRAM_CLK > 1
    if (even_out_en)    even_out->write(vwr_out[0]);
    if (odd_out_en)     odd_out->write(vwr_out[1]);
#endif // VWR_DRAM_CLK > 1
#endif // DUAL_BANK_INTERFACE

}

#endif

void softsimd_pu::adaptation_method() {
    ib_in = (uint64_t) cu_data_out;
}

#if RECORDING && !__SYNTHESIS__

/** We write a trace with the following format:
 *  Cycle,rst,m7-0,mask_vector,addsub,shift,shuffler,from_vwr,to_vwr
 *  Cycle: current cycle
 *  rst: reset signal
 *  m7: multiplexer 7, output to VWR comes from Shift&Add ('1') or Pack&Mask ('0')
 *  m6: multiplexer 6, R4 keeps its value ('0') or takes it from one of the stages output
 *  m5: multiplexer 5, R4 takes a new value from Shift&Add ('1') or Pack&Mask ('0')
 *  m4: multiplexer 4, R3 keeps its value ('0') or takes it from the Shift&Add output ('1')
 *  m3: multiplexer 3, R2 keeps its value ('0') or takes it from the Shift&Add output ('1')
 *  m2: multiplexer 2, shifter in Shift&Add stage gets the input (op1) from R4 ('0') or VWR ('1')
 *  m1: multiplexer 1, adder in Shift&Add stage gets input op2 from R1 ('0') or VWR ('1')
 *  m0: multiplexer 0, R0 keeps its value ('0') or takes it from the VWR ('1')
 *  mask_vector: mask to specify guardbits to adder/subtracter
 *  addsub: add/subtract signal: A==op1, B==op2, (A>>s) +- B
 *      000: -A-B
 *      001: -A+B
 *      010:  A-B
 *      011:  A+B
 *      101: -A+0
 *      111:  A+0
 *  shift: control of shifter in Shift&Add stage
 *  shuffler: control of shuffler in Pack&Mask stage (bits 6-3 for length change,
 *            bit 2 to select reg., bits 1-0 to select offset)
 *  from_vwr: word of data received from the active VWR
 *  to_vwr: word of data sent to the active VWR (expected result)
 */
void softsimd_pu::record_thread() {

    uint64_t cycle = 0;
    uint rst_rcd, m_rcd, addsub_rcd, shift_rcd, shuffler_rcd;
    uint64_t mask_rcd[WORD_64B], from_vwr_rcd[WORD_64B], to_vwr_rcd[WORD_64B];
    int i, j;
    SWREPACK repack_aux;
    uint pm_start_aux;
    sc_lv<WORD_BITS> vwr_muxed_aux;             
    sc_lv<WORD_64B*64> vwr_muxed_toparse('0');
    sc_bv<64> parse_aux; 

    wait();

    // Clocked behavior
    while (1) {
        rst_rcd = rst;
        m_rcd = 0;
        for (i = 0; i < VWR_NUM; i++) {
            m_rcd |= (vwr_from[i] == MUX::SA) ? M7_SET : 0;
        }
        m_rcd |= reg_en[3] ? M6_SET : 0;
        m_rcd |= (reg_from[3] == MUX::SA) ? M5_SET : 0;
        m_rcd |= reg_en[2] ? M4_SET : 0;
        m_rcd |= reg_en[1] ? M3_SET : 0;
        m_rcd |= is_vwr(sa_op1_from) ? M2_SET : 0;
        m_rcd |= 0; // is_vwr(sa_op2_from) ? M1_SET : 0;  // Currently, it's always from R1
        m_rcd |= reg_en[0] ? M0_SET : 0;
        // for (i = 0; i < WORD_64B; i++) {
        //     mask_rcd[i] = mrf_out[i];
        // }
        swlen_to_mask(sa_size, mask_rcd);
        if (sa_en && sa_adder_en) {
            if (!sa_neg_op1 && !sa_neg_op2)
                addsub_rcd = 0b011; // A+B
            if (!sa_neg_op1 && sa_neg_op2)
                addsub_rcd = 0b010; // A-B
            if (sa_neg_op1 && !sa_neg_op2)
                addsub_rcd = (sa_op2_from == MUX::ZERO ? 0b101 : 0b001); // -A+0 / -A+B
            if (sa_neg_op1 && sa_neg_op2)
                addsub_rcd = 0b000; // -A-B
        } else if (sa_en) {
            addsub_rcd = 0b111; // A+0 (Adder pass-through)
        } else {
            addsub_rcd = 0b000; // -A-B (SA stage disabled)
        }
        shift_rcd = sa_shift;
        repack_aux = pm_repack;
        pm_start_aux = pm_in_start;
        shuffler_rcd = shuffler_control(repack_aux, pm_start_aux);
        for (i = 0; i < WORD_64B; i++) {
            from_vwr_rcd[i] = 0;
            to_vwr_rcd[i] = 0;
        }
        for (i = 0; i < VWR_NUM; i++) {
            if (vwr_enable[i] && !vwr_wr_nrd[i] && !vwr_d_nm[i]) {  // VWR enabled, reading to it and multiplexing output
//                cout << "At " <<sc_time_stamp()<< " cycle " << cycle << " VWR" << i << " is enabled and reading " << endl;
                // Parse from resolved vector to 64-bit uint and store
                vwr_muxed_aux = vwr_muxed[i];
                vwr_muxed_toparse.range(WORD_BITS-1, 0) = vwr_muxed_aux;
                for (j = 0; j < WORD_64B; j++) {
                    parse_aux.range(63,0) = vwr_muxed_toparse.range((j+1)*64-1, j*64);
                    from_vwr_rcd[j] = parse_aux.to_uint64();
                }
            }

            if (vwr_enable[i] && vwr_wr_nrd[i] && vwr_d_nm[i]) {  // VWR enabled, writing to it and demultiplexing input
//                cout << "At " <<sc_time_stamp()<< " cycle " << cycle << " VWR" << i << " is enabled and writing " << endl;
                // Parse from resolved vector to 64-bit uint and store
                if (vwr_from[i] == MUX::SA) {
                    for (j = 0; j < WORD_64B; j++) {
                        to_vwr_rcd[j] = sa_out[j];
                    }
                } else if (vwr_from[i] == MUX::PM) {
                    for (j = 0; j < WORD_64B; j++) {
                        to_vwr_rcd[j] = pm_out[j];
                    }
                }
            }
        }

        record_file << showbase << dec << cycle++ << "," << rst_rcd << ",";
        record_file << hex << m_rcd << ",";
        record_file << mask_rcd[WORD_64B-1];
        for (i = WORD_64B-2; i >= 0; i--) {
            record_file << noshowbase << mask_rcd[i];
        }
        record_file << "," << showbase << addsub_rcd << ",";
        record_file << shift_rcd << ",";
        record_file << shuffler_rcd << ",";
        record_file << from_vwr_rcd[WORD_64B-1];
        for (i = WORD_64B-2; i >= 0; i--) {
            record_file << noshowbase << from_vwr_rcd[i];
        }
        record_file << "," << showbase << to_vwr_rcd[WORD_64B-1];
        for (i = WORD_64B-2; i >= 0; i--) {
            record_file << noshowbase << to_vwr_rcd[i];
        }
        record_file << endl;

        wait();
    }
}

void softsimd_pu::swlen_to_mask(SWSIZE sw_len, uint64_t *mask) {
    uint size = swsize_to_uint(sw_len);
    sc_bv<WORD_64B*64>  mask_temp('1');
    sc_bv<64> parse_aux;

#if WORD_64B*64 > WORD_BITS
    sc_bv<WORD_64B*64-WORD_BITS> zeros('0');
    mask_temp.range(WORD_64B*64-1, WORD_BITS) = zeros;
#endif

    if (sw_len != SWSIZE::INV) {
        for (uint i = 0; i < WORD_BITS/size; i++) {
            mask_temp[(i+1)*size-1] = '0';
        }
    }
    for (uint i = 0; i < WORD_64B; i++) {
        parse_aux.range(63,0) = mask_temp.range((i+1)*64-1, i*64);
        mask[i] = parse_aux.to_uint64();
    }
}

uint softsimd_pu::shuffler_control(SWREPACK repack, uint in_start) {
    uint ctrl = 0;
    uint in_size;
    uint out_size;
    uint offset_granularity;
    uint in_start_bits;

    if (repack != SWREPACK::INV) {
        in_size = swsize_to_uint(repack_to_insize(repack));
        out_size = swsize_to_uint(repack_to_outsize(repack));
        offset_granularity = (WORD_BITS/out_size)*in_size;
        in_start_bits = in_start * in_size;

        // Fill std::map with the possible values of the control signal
        std::map<uint, uint> ctrl_map;
        for (int i = 3; i >= 0; i--) {  // Reverse order in case ctrl_map indexes are repeated, to overwrite
            ctrl_map[(offset_granularity*i) % (2*WORD_BITS)] = i;   // 0-bit offset
            ctrl_map[(WORD_BITS + offset_granularity*i) % (2*WORD_BITS)] = i+4;
        }

        // Select the control signal using the map
        ctrl = ctrl_map.at(in_start_bits) & 0x7;  // Offset control

        ctrl |= (uint(repack) & 0xF) << 3;  // Length control
    }

    return ctrl;
}
#endif  // RECORDING && !__SYNTHESIS__

#endif  // !MIXED_SIM
