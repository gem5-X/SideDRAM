/*
 * Copyright EPFL 2023
 * Rafael Medina Morillas
 *
 * Implementations for the SoftSIMD opcodes.
 *
 */

#include "opcodes.h"

bool is_vwr (MUX mux) {
    return (mux == MUX::VWR0 ||
            mux == MUX::VWR1 ||
            mux == MUX::VWR2 ||
            mux == MUX::VWR3);
}

uint swsize_to_uint (SWSIZE swsize) {
    uint size;
    switch (swsize) {
        case SWSIZE::INV:   size = 0;   break;
        case SWSIZE::B3:    size = 3;   break;
        case SWSIZE::B4:    size = 4;   break;
        case SWSIZE::B6:    size = 6;   break;
        case SWSIZE::B8:    size = 8;   break;
        case SWSIZE::B12:   size = 12;  break;
        case SWSIZE::B16:   size = 16;  break;
        case SWSIZE::B24:   size = 24;  break;
        default:            size = 0;   break;
    }
    return size;
}

SWSIZE uint_to_swsize (uint size_uint) {
    SWSIZE size;
    switch (size_uint) {
        case 3:  size = SWSIZE::B3;  break;
        case 4:  size = SWSIZE::B4;  break;
        case 6:  size = SWSIZE::B6;  break;
        case 8:  size = SWSIZE::B8;  break;
        case 12: size = SWSIZE::B12; break;
        case 16: size = SWSIZE::B16; break;
        case 24: size = SWSIZE::B24; break;
        default: size = SWSIZE::INV; break;
    }
    return size;
}

#if SW_TRANS == TWO_LEVELS
SWSIZE repack_to_insize (SWREPACK swrepack) {
    SWSIZE insize;
    switch (swrepack) {
        case SWREPACK::INV:     insize = SWSIZE::INV;   break;
        case SWREPACK::B3_B3:   insize = SWSIZE::B3;    break;
        case SWREPACK::B4_B4:   insize = SWSIZE::B4;    break;
        case SWREPACK::B6_B6:   insize = SWSIZE::B6;    break;
        case SWREPACK::B8_B8:   insize = SWSIZE::B8;    break;
        case SWREPACK::B12_B12: insize = SWSIZE::B12;   break;
        case SWREPACK::B16_B16: insize = SWSIZE::B16;   break;
        case SWREPACK::B24_B24: insize = SWSIZE::B24;   break;
        case SWREPACK::B3_B4:   insize = SWSIZE::B3;    break;
        case SWREPACK::B3_B6:   insize = SWSIZE::B3;    break;
        case SWREPACK::B4_B6:   insize = SWSIZE::B4;    break;
        case SWREPACK::B4_B8:   insize = SWSIZE::B4;    break;
        case SWREPACK::B6_B8:   insize = SWSIZE::B6;    break;
        case SWREPACK::B6_B12:  insize = SWSIZE::B6;    break;
        case SWREPACK::B8_B12:  insize = SWSIZE::B8;    break;
        case SWREPACK::B8_B16:  insize = SWSIZE::B8;    break;
        case SWREPACK::B12_B16: insize = SWSIZE::B12;   break;
        case SWREPACK::B12_B24: insize = SWSIZE::B12;   break;
        case SWREPACK::B16_B24: insize = SWSIZE::B16;   break;
        case SWREPACK::B24_B16: insize = SWSIZE::B24;   break;
        case SWREPACK::B24_B12: insize = SWSIZE::B24;   break;
        case SWREPACK::B16_B12: insize = SWSIZE::B16;   break;
        case SWREPACK::B16_B8:  insize = SWSIZE::B16;   break;
        case SWREPACK::B12_B8:  insize = SWSIZE::B12;   break;
        case SWREPACK::B12_B6:  insize = SWSIZE::B12;   break;
        case SWREPACK::B8_B6:   insize = SWSIZE::B8;    break;
        case SWREPACK::B8_B4:   insize = SWSIZE::B8;    break;
        case SWREPACK::B6_B4:   insize = SWSIZE::B6;    break;
        case SWREPACK::B6_B3:   insize = SWSIZE::B6;    break;
        case SWREPACK::B4_B3:   insize = SWSIZE::B4;    break;
        default:                insize = SWSIZE::INV;   break;
    }
    return insize;
}

SWSIZE repack_to_outsize (SWREPACK swrepack) {
    SWSIZE outsize;
    switch (swrepack) {
        case SWREPACK::INV:     outsize = SWSIZE::INV;  break;
        case SWREPACK::B3_B3:   outsize = SWSIZE::B3;   break;
        case SWREPACK::B4_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B6_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B8_B8:   outsize = SWSIZE::B8;   break;
        case SWREPACK::B12_B12: outsize = SWSIZE::B12;  break;
        case SWREPACK::B16_B16: outsize = SWSIZE::B16;  break;
        case SWREPACK::B24_B24: outsize = SWSIZE::B24;  break;
        case SWREPACK::B3_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B3_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B4_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B4_B8:   outsize = SWSIZE::B8;   break;
        case SWREPACK::B6_B8:   outsize = SWSIZE::B8;   break;
        case SWREPACK::B6_B12:  outsize = SWSIZE::B12;  break;
        case SWREPACK::B8_B12:  outsize = SWSIZE::B12;  break;
        case SWREPACK::B8_B16:  outsize = SWSIZE::B16;  break;
        case SWREPACK::B12_B16: outsize = SWSIZE::B16;  break;
        case SWREPACK::B12_B24: outsize = SWSIZE::B24;  break;
        case SWREPACK::B16_B24: outsize = SWSIZE::B24;  break;
        case SWREPACK::B24_B16: outsize = SWSIZE::B16;  break;
        case SWREPACK::B24_B12: outsize = SWSIZE::B12;  break;
        case SWREPACK::B16_B12: outsize = SWSIZE::B12;  break;
        case SWREPACK::B16_B8:  outsize = SWSIZE::B8;   break;
        case SWREPACK::B12_B8:  outsize = SWSIZE::B8;   break;
        case SWREPACK::B12_B6:  outsize = SWSIZE::B6;   break;
        case SWREPACK::B8_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B8_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B6_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B6_B3:   outsize = SWSIZE::B3;   break;
        case SWREPACK::B4_B3:   outsize = SWSIZE::B3;   break;
        default:                outsize = SWSIZE::INV;  break;
    }
    return outsize;
}

uint repack_to_shift (SWREPACK repack) {
    uint shift;
    switch (repack) {
        case SWREPACK::B3_B4:   shift = 1;  break;
        case SWREPACK::B3_B6:   shift = 3;  break;
        case SWREPACK::B4_B6:   shift = 2;  break;
        case SWREPACK::B4_B8:   shift = 4;  break;
        case SWREPACK::B6_B8:   shift = 2;  break;
        case SWREPACK::B6_B12:  shift = 6;  break;
        case SWREPACK::B8_B12:  shift = 4;  break;
        case SWREPACK::B8_B16:  shift = 8;  break;
        case SWREPACK::B12_B16: shift = 4;  break;
        case SWREPACK::B12_B24: shift = 12; break;
        case SWREPACK::B16_B24: shift = 8;  break;
        default:                shift = 0;
    }
    return shift;
}
#endif

#if SW_TRANS == ONE_LEVEL
SWSIZE repack_to_insize (SWREPACK swrepack) {   // Pengbo version
    SWSIZE insize;
    switch (swrepack) {
        case SWREPACK::INV:     insize = SWSIZE::INV;   break;
        case SWREPACK::B3_B3:   insize = SWSIZE::B3;    break;
        case SWREPACK::B4_B4:   insize = SWSIZE::B4;    break;
        case SWREPACK::B6_B6:   insize = SWSIZE::B6;    break;
        case SWREPACK::B8_B8:   insize = SWSIZE::B8;    break;
        case SWREPACK::B12_B12: insize = SWSIZE::B12;   break;
        case SWREPACK::B16_B16: insize = SWSIZE::B16;   break;
        case SWREPACK::B24_B24: insize = SWSIZE::B24;   break;
        case SWREPACK::B3_B4:   insize = SWSIZE::B3;    break;
        case SWREPACK::B4_B6:   insize = SWSIZE::B4;    break;
        case SWREPACK::B6_B8:   insize = SWSIZE::B6;    break;
        case SWREPACK::B8_B12:  insize = SWSIZE::B8;    break;
        case SWREPACK::B12_B16: insize = SWSIZE::B12;   break;
        case SWREPACK::B16_B24: insize = SWSIZE::B16;   break;
        case SWREPACK::B24_B16: insize = SWSIZE::B24;   break;
        case SWREPACK::B16_B12: insize = SWSIZE::B16;   break;
        case SWREPACK::B12_B8:  insize = SWSIZE::B12;   break;
        case SWREPACK::B8_B6:   insize = SWSIZE::B8;    break;
        case SWREPACK::B6_B4:   insize = SWSIZE::B6;    break;
        case SWREPACK::B4_B3:   insize = SWSIZE::B4;    break;
        default:                insize = SWSIZE::INV;   break;
    }
    return insize;
}

SWSIZE repack_to_outsize (SWREPACK swrepack) {
    SWSIZE outsize;
    switch (swrepack) {
        case SWREPACK::INV:     outsize = SWSIZE::INV;  break;
        case SWREPACK::B3_B3:   outsize = SWSIZE::B3;   break;
        case SWREPACK::B4_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B6_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B8_B8:   outsize = SWSIZE::B8;   break;
        case SWREPACK::B12_B12: outsize = SWSIZE::B12;  break;
        case SWREPACK::B16_B16: outsize = SWSIZE::B16;  break;
        case SWREPACK::B24_B24: outsize = SWSIZE::B24;  break;
        case SWREPACK::B3_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B4_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B6_B8:   outsize = SWSIZE::B8;   break;
        case SWREPACK::B8_B12:  outsize = SWSIZE::B12;  break;
        case SWREPACK::B12_B16: outsize = SWSIZE::B16;  break;
        case SWREPACK::B16_B24: outsize = SWSIZE::B24;  break;
        case SWREPACK::B24_B16: outsize = SWSIZE::B16;  break;
        case SWREPACK::B16_B12: outsize = SWSIZE::B12;  break;
        case SWREPACK::B12_B8:  outsize = SWSIZE::B8;   break;
        case SWREPACK::B8_B6:   outsize = SWSIZE::B6;   break;
        case SWREPACK::B6_B4:   outsize = SWSIZE::B4;   break;
        case SWREPACK::B4_B3:   outsize = SWSIZE::B3;   break;
        default:                outsize = SWSIZE::INV;  break;
    }
    return outsize;
}

uint repack_to_shift (SWREPACK repack) {
    uint shift;
    switch (repack) {
        case SWREPACK::B3_B4:   shift = 1;  break;
        case SWREPACK::B4_B6:   shift = 2;  break;
        case SWREPACK::B6_B8:   shift = 2;  break;
        case SWREPACK::B8_B12:  shift = 4;  break;
        case SWREPACK::B12_B16: shift = 4;  break;
        case SWREPACK::B16_B24: shift = 8;  break;
        default:                shift = 0;
    }
    return shift;
}
#endif
