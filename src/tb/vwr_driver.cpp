#include "vwr_driver.h"

void vwr_driver::driver_thread() {

    uint clk_period = 10;
    uint i,j,k;
    uint max_idx = VWR_BITS / WORD_BITS;
//    uint max_pos = VWR_BITS / GRANULARITY;
//    uint max_len = WORD_BITS / GRANULARITY;
    uint subwords[6] = {3, 4, 6, 8, 12, 16};
//    uint gran[8] = {3, 4, 6, 8, 12, 16, 24, 48};
    sc_lv<VWR_BITS> largezs(SC_LOGIC_Z);
    sc_lv<WORD_BITS> smallzs(SC_LOGIC_Z);
    sc_lv<VWR_BITS> laux = 0;
    sc_lv<WORD_BITS> ones(SC_LOGIC_1);

    // Initialization
    rst->write(false);
    wr_nrd->write(false);
    idx->write(0);
    d_nm->write(false);
//    pos->write(0);
//    len->write(0);
    lbus->write(largezs);
    sbus->write(smallzs);

    wait(clk_period / 2, SC_NS);
    rst->write(true);
    wait(0, SC_NS);

    wait(clk_period / 2 + 1, SC_NS);

    for (i=0;; i++) {
            uint sw_size = subwords[i%6];

            cout << "Writing VWR ------------------------------------------------" << endl;

            // Write complete register
            for (j=0; j<VWR_BITS/sw_size; j++) {
                laux.range((j+1) * sw_size - 1, j * sw_size) = i + j;
            }
            lbus->write(laux);
            wr_nrd->write(true);
            wait(clk_period, SC_NS);

            lbus->write(largezs);
            wr_nrd->write(false);

            cout << endl << "Accessing VWR at different positions -----------" << endl;

            // Access it completely at different lengths
            for (j=0; j<max_idx; j++) {
                idx->write(j);
                wait(clk_period, SC_NS);
            }

            cout << endl << "Writing VWR at different positions -------------" << endl;

            // Write one of the small words
            for (j=0; j<max_idx; j++) {
                idx->write(j);
                d_nm->write(true);
                sbus->write(ones.to_uint() - i - k++);
                wr_nrd->write(true);
                wait(clk_period, SC_NS);
            }

            sbus->write(smallzs);
            d_nm->write(false);
            idx->write(0);
            wr_nrd->write(false);
        }

//    for (i=0;; i++) {
//        uint sw_size = subwords[i%6];
//
//        cout << "Writing VWR ------------------------------------------------" << endl;
//
//        // Write complete register
//        for (j=0; j<VWR_BITS/sw_size; j++) {
//            laux.range((j+1) * sw_size - 1, j * sw_size) = i + j;
//        }
//        lbus->write(laux);
//        wr_nrd->write(true);
//        wait(clk_period, SC_NS);
//
//        lbus->write(largezs);
//        wr_nrd->write(false);
//
//        cout << endl << "Accessing VWR at different positions and lengths -----------" << endl;
//
//        // Access it completely at different lengths
//        for (j=0; j<8; j++) {
//            for (k=0; k<VWR_BITS/gran[j]/8; k++) {
//                pos->write(k*gran[j]);
//                len->write(gran[j]);
//                wait(clk_period, SC_NS);
//            }
//        }
//
//        cout << endl << "Writing VWR at different positions and lengths -------------" << endl;
//
//        // Write one of the small words
//        for (j=0; j<8; j++) {
//            for (k=0; k<VWR_BITS/gran[j]/8; k++) {
//                pos->write(k*gran[j]);
//                len->write(gran[j]);
//                d_nm->write(true);
//                sbus->write(ones.to_uint() - i - k);
//                wr_nrd->write(true);
//                wait(clk_period, SC_NS);
//            }
//        }
//
//        sbus->write(smallzs);
//        d_nm->write(false);
//        pos->write(0);
//        len->write(0);
//        wr_nrd->write(false);
//    }
}
