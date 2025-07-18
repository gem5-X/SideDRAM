/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Implementation of the control plane for area and energy assessment.
 *
 */

#include "control_plane.h"

#if !MIXED_SIM
#if EN_MODEL == 0

#ifndef __SYNTHESIS__

void control_plane::comb_method() {
    uint i;

    // CSD RF input multiplexer
    for (i = 0; i < CSD_64B; i++) {
        switch (csdrf_wr_from) {
            case MUX::EXT:  csdrf_in[i] = cu_data_out;    break;
            default:        csdrf_in[i] = cu_data_out;    break;
        }
    }
}

#else   // __SYNTHESIS__

void control_plane::comb_method() {
    uint i;

    // CSD RF input multiplexer
    for (i = 0; i < CSD_64B; i++) {
        switch (csdrf_wr_from) {
            case MUX::EXT:  csdrf_in[i] = cu_data_out;    break;
            default:        csdrf_in[i] = cu_data_out;    break;
        }
    }
}

#endif

void control_plane::adaptation_method() {
    ib_in = (uint64_t) cu_data_out;
    data_out->write(cu_data_out);
}

#endif  // EN_MODEL == 0
#endif  // !MIXED_SIM
