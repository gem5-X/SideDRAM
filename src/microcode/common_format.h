/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Functions and structures common to the different instruction formats
 *
 */

#ifndef SRC_MICROCODE_COMMON_FORMAT_H_
#define SRC_MICROCODE_COMMON_FORMAT_H_

#include "../defs.h"
#include "../opcodes.h"
#include "../log2.h"

// Format of ITT fields
typedef struct itt_field {
    bool valid;
    uint addr;
    bool stop;

    inline itt_field& operator= (const itt_field& rhs) {
        valid = rhs.valid;
        addr = rhs.addr;
        stop = rhs.stop;
        return *this;
    }

    inline bool operator== (const itt_field& rhs) const {
        return (valid == rhs.valid && addr == rhs.addr && stop == rhs.stop);
    }
} itt_field;

// Function for building macroinstruction
uint64_t build_macroinstr (uint itt_idx, uint imm, OPC_STORAGE dst, OPC_STORAGE src, SWSIZE sw_len, uint shift_sa, uint rf_n, uint src0_n,
                           OPC_STORAGE src0, SWREPACK repack, uint pack_sta, uint shift_pm, uint dst_n, uint dst_n_sa);

// Get DST and CSD_addr from macroinstruction to the multiplication sequencer
SWSIZE get_ms_size (uint64_t macroinstr);
OPC_STORAGE get_ms_src (uint64_t macroinstr);
uint get_ms_src_n (uint64_t macroinstr);
OPC_STORAGE get_ms_dst (uint64_t macroinstr);
uint get_ms_dst_n (uint64_t macroinstr);
uint get_ms_csd_addr (uint64_t macroinstr);
bool is_vwr (OPC_STORAGE opc);

// Translate macroinstruction fields into specific stage fields
void unpack_fields (uint64_t macrofields, bool mov_valid, bool sa_valid, bool pm_valid,
                    uint32_t* mov_fields, uint32_t* sa_fields, uint32_t* pm_fields);

// Decode MOV microinstruction
void decode_mov_microinstr (uint16_t microinstr, uint32_t fields, MOP* mov_op, OPC_STORAGE* dst,
                            OPC_STORAGE* src, uint* imm, OPC_STORAGE* dst_sa, uint* dst_n_sa);

// Decode Shift&Add microinstruction
void decode_sa_microinstr (uint16_t microinstr, uint32_t fields, ADDOP* sa_add, uint* sa_shift,
                            SWSIZE* sw_len, OPC_STORAGE* dst, OPC_STORAGE* src0, OPC_STORAGE* src1);

// Decode Pack&Mask microinstruction
void decode_pm_microinstr (uint16_t microinstr, uint32_t fields, MASKOP* pm_mask, OPC_STORAGE* dst, bool* perm,
                            OPC_STORAGE* src0, OPC_STORAGE* src1, SWREPACK* repack, uint* pack_sta, uint* shift);

#endif /* SRC_MICROCODE_COMMON_FORMAT_H_ */
