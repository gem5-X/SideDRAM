/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Structures defining the functions to build and decode macro/microinstructions with DLBs of medium size.
 *
 */

#include "encoded_shift_format.h"

#if (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)

uint64_t build_macroinstr (uint itt_idx, uint imm, OPC_STORAGE dst, OPC_STORAGE src, SWSIZE sw_len, uint shift_sa, uint rf_n, uint src0_n,
                           OPC_STORAGE src0, SWREPACK repack, uint pack_sta, uint shift_pm, uint dst_n, uint dst_n_sa) {
    // As three formats are possible, we decide looking at if SW_LEN and REPACK are zeroed
    uint64_t macroinstr = 0;

    if (sw_len != SWSIZE::INV) { // SA
        macroinstr |= (uint(sw_len) & ((1UL << MACRO_SW_LEN_BITS) - 1));
        macroinstr <<= MACRO_RF_N_BITS;
        macroinstr |= (rf_n & ((1UL << MACRO_RF_N_BITS) - 1));
        macroinstr <<= MACRO_DST_N_SA_BITS;
        macroinstr |= (dst_n_sa & ((1UL << MACRO_DST_N_SA_BITS) - 1));
        macroinstr <<= MACRO_SRC0_N_BITS;
        macroinstr |= (src0_n & ((1UL << MACRO_SRC0_N_BITS) - 1));  // If multiplying, this can set the length of CSD used
        macroinstr <<= MACRO_DST_BITS;
        macroinstr |= (uint(dst) & ((1UL << MACRO_DST_BITS) - 1));
        macroinstr <<= MACRO_SRC_BITS;
        macroinstr |= (uint(src0) & ((1UL << MACRO_SRC_BITS) - 1));
    } else if (repack != SWREPACK::INV) { // PM
        macroinstr |= (uint(repack) & ((1UL << MACRO_REPACK_BITS) - 1));
        macroinstr <<= MACRO_PACK_STA_BITS;
        macroinstr |= (pack_sta & ((1UL << MACRO_PACK_STA_BITS) - 1));
        macroinstr <<= MACRO_DST_N_BITS;
        macroinstr |= (dst_n & ((1UL << MACRO_DST_N_BITS) - 1));
        macroinstr <<= MACRO_DST_BITS;
        macroinstr |= (uint(dst) & ((1UL << MACRO_DST_BITS) - 1));
        macroinstr <<= MACRO_SRC_BITS;  // Zero fill to reduce decoder HW
    } else { // MOV
        macroinstr |= (imm & ((1UL << MACRO_IMM_BITS) - 1));
        macroinstr <<= MACRO_DST_BITS;
        macroinstr |= (uint(dst) & ((1UL << MACRO_DST_BITS) - 1));
        macroinstr <<= MACRO_SRC_BITS;
        macroinstr |= (uint(src) & ((1UL << MACRO_SRC_BITS) - 1));
    }
    macroinstr |= (itt_idx & ((1UL << ITT_IDX_BITS) - 1)) << SHIFT_ITT_IDX;   // ITT_IDX
    return macroinstr;
}

SWSIZE get_ms_size (uint64_t macroinstr) {
    SWSIZE size;
    macroinstr >>= MACRO_RF_N_BITS+MACRO_DST_N_SA_BITS+MACRO_SRC0_N_BITS+MACRO_DST_BITS+MACRO_SRC_BITS;
    size = (SWSIZE) (macroinstr & ((1UL << MACRO_SW_LEN_BITS) - 1));
    return size;
}

OPC_STORAGE get_ms_src (uint64_t macroinstr) {
    OPC_STORAGE src;
    src = (OPC_STORAGE) (macroinstr & ((1UL << MACRO_SRC_BITS) - 1));
    return src;
}

uint get_ms_src_n (uint64_t macroinstr) {
    uint src_n;
    macroinstr >>= MACRO_SRC_BITS + MACRO_DST_BITS;
    src_n = macroinstr & ((1UL << MACRO_SRC0_N_BITS) - 1);
    return src_n;
}

OPC_STORAGE get_ms_dst (uint64_t macroinstr) {
    OPC_STORAGE dst;
    dst = (OPC_STORAGE) ((macroinstr >> MACRO_SRC_BITS) & ((1UL << MACRO_DST_BITS) - 1));
    return dst;
}

uint get_ms_dst_n (uint64_t macroinstr) {
    uint dst_n;
    macroinstr >>= MACRO_SRC_BITS + MACRO_DST_BITS + MACRO_SRC0_N_BITS;
    dst_n = macroinstr & ((1UL << MACRO_DST_N_SA_BITS) - 1);
    return dst_n;
}

uint get_ms_csd_addr (uint64_t macroinstr) {
    uint src;
    src = (macroinstr >> (MACRO_DST_N_SA_BITS+MACRO_SRC0_N_BITS+MACRO_DST_BITS+MACRO_SRC_BITS))\
           & ((1UL << MACRO_RF_N_BITS) - 1);
    return src;
}

bool is_vwr (OPC_STORAGE opc) {
    return opc_to_sa_dst(opc) == SA_DST::VWR;
}

void unpack_fields (uint64_t macrofields, bool mov_valid, bool sa_valid, bool pm_valid,
                    uint32_t* mov_fields, uint32_t* sa_fields, uint32_t* pm_fields) {
    uint64_t macro_aux;
    OPC_STORAGE temp;
    MOV_SRC mov_src;
    MOV_DST mov_dst;
    uint mov_imm;
    MOV_DST_SA mov_dst_sa;
    uint mov_dst_n_sa;
    SA_SRC0 sa_src0;
    SA_DST sa_dst;
    SWSIZE sa_sw_len;
    PM_DST pm_dst;
    uint pm_pack_sta;
    SWREPACK pm_repack;

    *mov_fields = 0;
    *sa_fields = 0;
    *pm_fields = 0;

    if (mov_valid && !sa_valid && !pm_valid) {
        macro_aux = macrofields;
        temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_SRC_BITS) - 1));
        mov_src = opc_to_mov_src(temp);
        macro_aux >>= MACRO_SRC_BITS;
        temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_DST_BITS) - 1));
        mov_dst = opc_to_mov_dst(temp);
        macro_aux >>= MACRO_DST_BITS;
        mov_imm = macro_aux & ((1UL << MACRO_IMM_BITS) - 1);
        mov_dst_sa = MOV_DST_SA::VWR0;
        mov_dst_n_sa = 0;
    }

    if (sa_valid) {
        macro_aux = macrofields;
        temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_SRC_BITS) - 1));
        sa_src0 = opc_to_sa_src0(temp);
        macro_aux >>= MACRO_SRC_BITS;
        temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_DST_BITS) - 1));
        sa_dst = opc_to_sa_dst(temp);
        macro_aux >>= MACRO_DST_BITS;
        macro_aux >>= MACRO_SRC0_N_BITS;
        macro_aux >>= MACRO_DST_N_SA_BITS;
        macro_aux >>= MACRO_RF_N_BITS;
        sa_sw_len = (SWSIZE) (macro_aux & ((1UL << MACRO_SW_LEN_BITS) - 1));

        if (mov_valid) {
            macro_aux = macrofields;
            temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_SRC_BITS) - 1));
            mov_src = opc_to_mov_src(temp);
            macro_aux >>= MACRO_SRC_BITS;
            mov_dst = MOV_DST::SA;
            temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_DST_BITS) - 1));
            mov_dst_sa = opc_to_mov_dst_sa(temp);
            macro_aux >>= MACRO_DST_BITS;
            mov_imm = macro_aux & ((1UL << MACRO_SRC0_N_BITS) - 1);
            macro_aux >>= MACRO_SRC0_N_BITS;
            mov_dst_n_sa = macro_aux & ((1UL << MACRO_DST_N_SA_BITS) - 1);
        }
    }

    if (pm_valid) {
        macro_aux = macrofields;
        macro_aux >>= MACRO_SRC_BITS;
        temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_DST_BITS) - 1));
        pm_dst = opc_to_pm_dst(temp);
        macro_aux >>= MACRO_DST_BITS;
        macro_aux >>= MACRO_DST_N_BITS;
        pm_pack_sta = macro_aux & ((1UL << MACRO_PACK_STA_BITS) - 1);
        macro_aux >>= MACRO_PACK_STA_BITS;
        pm_repack = (SWREPACK) (macro_aux & ((1UL << MACRO_REPACK_BITS) - 1));

        if (mov_valid) {
            macro_aux = macrofields;
            macro_aux >>= MACRO_SRC_BITS;
            mov_src = MOV_SRC::PM;
            temp = (OPC_STORAGE) (macro_aux & ((1UL << MACRO_DST_BITS) - 1));
            mov_dst = opc_to_mov_dst(temp);
            macro_aux >>= MACRO_DST_BITS;
            mov_imm = macro_aux & ((1UL << MACRO_DST_N_BITS) - 1);
            mov_dst_sa = MOV_DST_SA::VWR0;
            mov_dst_n_sa = 0;
        }
    }

    if (mov_valid) {
        *mov_fields |= uint(mov_imm) & ((1UL << MOV_IMM_BITS) - 1);
        *mov_fields <<= MOV_DST_BITS;
        *mov_fields |= uint(mov_dst) & ((1UL << MOV_DST_BITS) - 1);
        *mov_fields <<= MOV_SRC_BITS;
        *mov_fields |= uint(mov_src) & ((1UL << MOV_SRC_BITS) - 1);
        *mov_fields <<= MOV_DST_SA_BITS;
        *mov_fields |= uint(mov_dst_sa) & ((1UL << MOV_DST_SA_BITS) - 1);
        *mov_fields <<= MOV_DST_N_SA_BITS;
        *mov_fields |= uint(mov_dst_n_sa) & ((1UL << MOV_DST_N_SA_BITS) - 1);
    }

    if (sa_valid) {
        *sa_fields |= uint(sa_sw_len) & ((1UL << SA_SW_LEN_BITS) - 1);
        *sa_fields <<= SA_DST_BITS;
        *sa_fields |= uint(sa_dst) & ((1UL << SA_DST_BITS) - 1);
        *sa_fields <<= SA_SRC0_BITS;
        *sa_fields |= uint(sa_src0) & ((1UL << SA_SRC0_BITS) - 1);
    }

    if (pm_valid) {
        *pm_fields |= uint(pm_repack) & ((1UL << PM_REPACK_BITS) - 1);
        *pm_fields <<= PM_PACK_STA_BITS;
        *pm_fields |= uint(pm_pack_sta) & ((1UL << PM_PACK_STA_BITS) - 1);
        *pm_fields <<= PM_DST_BITS;
        *pm_fields |= uint(pm_dst) & ((1UL << PM_DST_BITS) - 1);
    }
}

OPC_STORAGE mov_dst_to_opc (MOV_DST mov_dst) {
    switch (mov_dst) {
        case MOV_DST::VWR0: return OPC_STORAGE::VWR0;   break;
        case MOV_DST::VWR1: return OPC_STORAGE::VWR1;   break;
        case MOV_DST::VWR2: return OPC_STORAGE::VWR2;   break;
        case MOV_DST::VWR3: return OPC_STORAGE::VWR3;   break;
        case MOV_DST::DRAM: return OPC_STORAGE::DRAM;   break;
        case MOV_DST::R0:   return OPC_STORAGE::R0;     break;
        case MOV_DST::SA:   return OPC_STORAGE::SA;     break;
        default:            return OPC_STORAGE::R0;     break;
    }
}

MOV_DST opc_to_mov_dst (OPC_STORAGE mov_dst) {
    switch (mov_dst) {
        case OPC_STORAGE::VWR0: return MOV_DST::VWR0;   break;
        case OPC_STORAGE::VWR1: return MOV_DST::VWR1;   break;
        case OPC_STORAGE::VWR2: return MOV_DST::VWR2;   break;
        case OPC_STORAGE::VWR3: return MOV_DST::VWR3;   break;
        case OPC_STORAGE::DRAM: return MOV_DST::DRAM;   break;
        case OPC_STORAGE::R0:   return MOV_DST::R0;     break;
        case OPC_STORAGE::SA:   return MOV_DST::SA;     break;
        default:                return MOV_DST::R0;     break;
    }
}

OPC_STORAGE mov_src_to_opc (MOV_SRC mov_src) {
    switch (mov_src) {
        case MOV_SRC::VWR0: return OPC_STORAGE::VWR0;   break;
        case MOV_SRC::VWR1: return OPC_STORAGE::VWR1;   break;
        case MOV_SRC::VWR2: return OPC_STORAGE::VWR2;   break;
        case MOV_SRC::VWR3: return OPC_STORAGE::VWR3;   break;
        case MOV_SRC::DRAM: return OPC_STORAGE::DRAM;   break;
        case MOV_SRC::SA:   return OPC_STORAGE::SA;     break;
        case MOV_SRC::PM:   return OPC_STORAGE::PM;     break;
        default:            return OPC_STORAGE::SA;     break;
    }
}

MOV_SRC opc_to_mov_src (OPC_STORAGE mov_src) {
    switch (mov_src) {
        case OPC_STORAGE::VWR0: return MOV_SRC::VWR0;   break;
        case OPC_STORAGE::VWR1: return MOV_SRC::VWR1;   break;
        case OPC_STORAGE::VWR2: return MOV_SRC::VWR2;   break;
        case OPC_STORAGE::VWR3: return MOV_SRC::VWR3;   break;
        case OPC_STORAGE::DRAM: return MOV_SRC::DRAM;   break;
        case OPC_STORAGE::SA:   return MOV_SRC::SA;     break;
        case OPC_STORAGE::PM:   return MOV_SRC::PM;     break;
        default:                return MOV_SRC::SA;     break;
    }
}

OPC_STORAGE mov_dst_sa_to_opc (MOV_DST_SA mov_dst_sa) {
    switch (mov_dst_sa) {
        case MOV_DST_SA::VWR0:  return OPC_STORAGE::VWR0;   break;
        case MOV_DST_SA::VWR1:  return OPC_STORAGE::VWR1;   break;
        case MOV_DST_SA::VWR2:  return OPC_STORAGE::VWR2;   break;
        case MOV_DST_SA::VWR3:  return OPC_STORAGE::VWR3;   break;
        default:                return OPC_STORAGE::VWR0;     break;
    }
}

MOV_DST_SA opc_to_mov_dst_sa (OPC_STORAGE mov_dst_sa) {
    switch (mov_dst_sa) {
        case OPC_STORAGE::VWR0: return MOV_DST_SA::VWR0;   break;
        case OPC_STORAGE::VWR1: return MOV_DST_SA::VWR1;   break;
        case OPC_STORAGE::VWR2: return MOV_DST_SA::VWR2;   break;
        case OPC_STORAGE::VWR3: return MOV_DST_SA::VWR3;   break;
        default:                return MOV_DST_SA::VWR0;   break;
    }
}

uint16_t build_mov_microinstr (MOP mov_op) {
    uint16_t microinstr = 0;
    microinstr |= (uint(mov_op) & ((1UL << MOV_OP_BITS) - 1));
    return microinstr;
}

void decode_mov_microinstr (uint16_t microinstr, uint32_t fields, MOP* mov_op, OPC_STORAGE* dst,
                            OPC_STORAGE* src, uint* imm, OPC_STORAGE* dst_sa, uint* dst_n_sa) {
    *mov_op = (MOP) (microinstr & ((1UL << MOV_OP_BITS) - 1));
    *dst_n_sa = (fields & ((1UL << MOV_DST_N_SA_BITS) - 1));
    fields >>= MOV_DST_N_SA_BITS;
    MOV_DST_SA dst_sa_temp = (MOV_DST_SA) (fields & ((1UL << MOV_DST_SA_BITS) - 1));
    *dst_sa = mov_dst_sa_to_opc(dst_sa_temp);
    fields >>= MOV_DST_SA_BITS;
    MOV_SRC src_temp = (MOV_SRC) (fields & ((1UL << MOV_SRC_BITS) - 1));
    *src = mov_src_to_opc(src_temp);
    fields >>= MOV_SRC_BITS;
    MOV_DST dst_temp = (MOV_DST) (fields & ((1UL << MOV_DST_BITS) - 1));
    *dst = mov_dst_to_opc(dst_temp);
    fields >>= MOV_DST_BITS;
    *imm = (fields & ((1UL << MOV_IMM_BITS) - 1));
}

OPC_STORAGE sa_dst_to_opc (SA_DST sa_dst) {
    switch (sa_dst) {
        case SA_DST::R3:    return OPC_STORAGE::R3;     break;
        case SA_DST::R1:    return OPC_STORAGE::R1;     break;
        case SA_DST::R2:    return OPC_STORAGE::R2;     break;
        case SA_DST::R1R2:  return OPC_STORAGE::R1R2;   break;
        case SA_DST::VWR:   return OPC_STORAGE::VWR;    break;
        default:            return OPC_STORAGE::R3;     break;
    }
}

SA_DST opc_to_sa_dst (OPC_STORAGE sa_dst) {
    switch (sa_dst) {
        case OPC_STORAGE::R3:   return SA_DST::R3;      break;
        case OPC_STORAGE::R1:   return SA_DST::R1;      break;
        case OPC_STORAGE::R2:   return SA_DST::R2;      break;
        case OPC_STORAGE::R1R2: return SA_DST::R1R2;    break;
        case OPC_STORAGE::VWR:  return SA_DST::VWR;     break;
        case OPC_STORAGE::VWR0: return SA_DST::VWR;     break;
        case OPC_STORAGE::VWR1: return SA_DST::VWR;     break;
        case OPC_STORAGE::VWR2: return SA_DST::VWR;     break;
        case OPC_STORAGE::VWR3: return SA_DST::VWR;     break;
        default:                return SA_DST::R3;      break;
    }
}

OPC_STORAGE sa_src0_to_opc (SA_SRC0 sa_src0) {
    switch (sa_src0) {
        case SA_SRC0::VWR:  return OPC_STORAGE::VWR;    break;
        case SA_SRC0::R3:   return OPC_STORAGE::R3;     break;
        case SA_SRC0::ZERO: return OPC_STORAGE::ZERO;   break;
        default:            return OPC_STORAGE::ZERO;   break;
    }
}

SA_SRC0 opc_to_sa_src0 (OPC_STORAGE sa_src0) {
    switch (sa_src0) {
        case OPC_STORAGE::VWR:  return SA_SRC0::VWR;    break;
        case OPC_STORAGE::VWR0: return SA_SRC0::VWR;    break;
        case OPC_STORAGE::VWR1: return SA_SRC0::VWR;    break;
        case OPC_STORAGE::VWR2: return SA_SRC0::VWR;    break;
        case OPC_STORAGE::VWR3: return SA_SRC0::VWR;    break;
        case OPC_STORAGE::R3:   return SA_SRC0::R3;     break;
        case OPC_STORAGE::ZERO: return SA_SRC0::ZERO;   break;
        default:                return SA_SRC0::ZERO;   break;
    }
}

uint16_t build_sa_microinstr (ADDOP sa_add, uint sa_shift) {
    uint16_t microinstr = 0;
    microinstr |= (uint(sa_add) & ((1UL << SA_ADD_BITS) - 1));
    microinstr <<= SA_SHIFT_BITS;
    microinstr |= (sa_shift & ((1UL << SA_SHIFT_BITS) - 1));
    return microinstr;
}

void decode_sa_microinstr (uint16_t microinstr, uint32_t fields, ADDOP* sa_add, uint* sa_shift,
                            SWSIZE* sw_len, OPC_STORAGE* dst, OPC_STORAGE* src0, OPC_STORAGE* src1) {
    *sa_shift = (microinstr & ((1UL << SA_SHIFT_BITS) - 1));
    microinstr >>= SA_SHIFT_BITS;
    *sa_add = (ADDOP) (microinstr & ((1UL << SA_ADD_BITS) - 1));
    SA_SRC0 src0_temp = (SA_SRC0) (fields & ((1UL << SA_SRC0_BITS) - 1));
    *src0 = sa_src0_to_opc(src0_temp);
    fields >>= SA_SRC0_BITS;
    SA_DST dst_temp = (SA_DST) (fields & ((1UL << SA_DST_BITS) - 1));
    *dst = sa_dst_to_opc(dst_temp);
    fields >>= SA_DST_BITS;
    *sw_len = (SWSIZE) (fields & ((1UL << SA_SW_LEN_BITS) - 1));

    *src1 = OPC_STORAGE::R0;    // In this implementation it's always R0
}

OPC_STORAGE pm_dst_to_opc (PM_DST pm_dst) {
    switch (pm_dst) {
        case PM_DST::VWR:   return OPC_STORAGE::VWR;    break;
        case PM_DST::R3:    return OPC_STORAGE::R3;     break;
        default:            return OPC_STORAGE::R3;     break;
    }
}

PM_DST opc_to_pm_dst (OPC_STORAGE pm_dst) {
    switch (pm_dst) {
        case OPC_STORAGE::VWR:  return PM_DST::VWR;     break;
        case OPC_STORAGE::VWR0: return PM_DST::VWR;     break;
        case OPC_STORAGE::VWR1: return PM_DST::VWR;     break;
        case OPC_STORAGE::VWR2: return PM_DST::VWR;     break;
        case OPC_STORAGE::VWR3: return PM_DST::VWR;     break;
        case OPC_STORAGE::R3:   return PM_DST::R3;      break;
        default:                return PM_DST::R3;      break;
    }
}

uint16_t build_pm_microinstr (MASKOP pm_mask, PM_DST pm_dst, bool pm_perm) {
    uint16_t microinstr = 0;
    microinstr |= (uint(pm_mask) & ((1UL << PM_MASK_BITS) - 1));
    microinstr <<= PM_DST_BITS;
    microinstr |= (uint(pm_dst) & ((1UL << PM_DST_BITS) - 1));
    microinstr <<= PM_PERM_BITS;
    microinstr |= (uint(pm_perm) & ((1UL << PM_PERM_BITS) - 1));
    return microinstr;
}

void decode_pm_microinstr (uint16_t microinstr, uint32_t fields, MASKOP* pm_mask, OPC_STORAGE* dst, bool *perm,
                            OPC_STORAGE* src0, OPC_STORAGE* src1, SWREPACK* repack, uint* pack_sta, uint* shift) {
    *perm = (bool) (microinstr & ((1UL << PM_PERM_BITS) - 1));
    microinstr >>= PM_PERM_BITS;
    PM_DST dst_temp  = (PM_DST) (microinstr & ((1UL << PM_DST_BITS) - 1));
    *dst = pm_dst_to_opc(dst_temp);
    microinstr >>= PM_DST_BITS;
    *pm_mask = (MASKOP) (microinstr & ((1UL << PM_DST_BITS) - 1));
    fields >>= PM_DST_BITS;    // Skipping since it's only useful for MOV
    *pack_sta = (fields & ((1UL << PM_PACK_STA_BITS) - 1));
    fields >>= PM_PACK_STA_BITS;
    *repack = (SWREPACK) (fields & ((1UL << PM_REPACK_BITS) - 1));
    *shift = repack_to_shift(*repack);
    *src0 = OPC_STORAGE::R1;    // In this implementation it's always R1
    *src1 = OPC_STORAGE::R2;    // In this implementation it's always R2
}

#endif // ENCODED_SHIFT_FORMAT
