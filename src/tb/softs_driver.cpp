#include "softs_driver.h"

void softs_driver::driver_thread() {

    uint clk_period = 10;
    uint i,j,k,m;
    uint64_t sw_aux1, sw_aux2, word_aux;

    // Initialization
    rst->write(false);
    SA_en->write(false);
    SA_shift->write(0);
    SA_size->write(0);
    SA_s_na->write(false);
    for (i = 0; i < WORD_64B; i++) {
        SA_op1[i]->write(0);
        SA_op2[i]->write(0);
    }
    PM_en->write(false);
    for (i = 0; i < WORD_64B; i++) {
        PM_w1[i]->write(0);
        PM_w2[i]->write(0);
    }
    PM_in_size->write(0);
    PM_out_size->write(0);
    PM_in_start->write(0);
    for (i = 0; i < MASK_64B; i++) {
        PM_mask_in[i]->write(0);
    }
    PM_op_sel->write(uint(MASKOP::NOP));
    PM_shift->write(0);

    wait(clk_period / 2, SC_NS);
    rst->write(true);
    wait(0, SC_NS);

    wait(clk_period / 2 + 1, SC_NS);

    // Test shifters
    for (i=0; i<9; i++) {   // No sign extension
        SA_en->write(true);
        SA_shift->write(i);
        SA_size->write(0);
        for (j = 0; j < WORD_64B; j++)
            SA_op1[j]->write(0x7FF98007F9CB89E5);  // -7 7 -1589 2533, with guard bits
        PM_en->write(true);
        PM_out_size->write(0);
        PM_shift->write(i);
        for (j = 0; j < WORD_64B; j++)
            PM_w1[j]->write(0x7FF98007F9CB89E5);   // -7 7 -1589 2533, with guard bits
        wait(clk_period, SC_NS);
    }

    for (i=0; i<9; i++) {   // Sign extension, 16-bit subwords
        SA_en->write(true);
        SA_shift->write(i);
        SA_size->write(16);
        for (j = 0; j < WORD_64B; j++)
            SA_op1[j]->write(0x7FF98007F9CB89E5);  // -7 7 -1589 2533, with guard bits
        PM_en->write(true);
        PM_out_size->write(16);
        PM_shift->write(i);
        for (j = 0; j < WORD_64B; j++)
            PM_w1[j]->write(0x7FF98007F9CB89E5);   // -7 7 -1589 2533, with guard bits
        wait(clk_period, SC_NS);
    }

    for (i=0; i<9; i++) {   // Sign extension, 8-bit subwords
        SA_en->write(true);
        SA_shift->write(i);
        SA_size->write(8);
        for (j = 0; j < WORD_64B; j++)
            SA_op1[j]->write(0xFF8071318303FAEA);
        PM_en->write(true);
        PM_out_size->write(8);
        PM_shift->write(i);
        for (j = 0; j < WORD_64B; j++)
            PM_w1[j]->write(0xFF8071318303FAEA);
        wait(clk_period, SC_NS);
    }

    for (i=0; i<9; i++) {   // Sign extension, 12-bit subwords
        SA_en->write(true);
        SA_shift->write(i);
        SA_size->write(12);
        for (j = 0; j < WORD_64B; j++)
            SA_op1[j]->write(0x0FF887F77705A800);
        PM_en->write(true);
        PM_out_size->write(12);
        PM_shift->write(i);
        for (j = 0; j < WORD_64B; j++)
            PM_w1[j]->write(0x0FF887F77705A800);
        wait(clk_period, SC_NS);
    }
    SA_en->write(false);
    SA_shift->write(0);
    SA_size->write(0);
    for (j = 0; j < WORD_64B; j++)
        SA_op1[j]->write(0);
    PM_en->write(false);
    PM_out_size->write(0);
    PM_shift->write(0);
    for (j = 0; j < WORD_64B; j++)
        PM_w1[j]->write(0);



    // Test adder/subtractor
    // Using subwords, we test simultaneous adds and subtractions +/+, +/-, -/+, -/-
    // Check which guard bits to use in each case
    SA_en->write(true);
    SA_s_na->write(false);  // Addition (guard bits accordingly)
    for (i = 0; i < WORD_64B; i++) {
        SA_op1[i]->write(0x00017FF600647C18);  // Op1 = 1    -10 100 -1000
        SA_op2[i]->write(0x03E800017FF67F9C);  // Op2 = 1000   1 -10  -100
    }
    wait(clk_period, SC_NS);

    SA_en->write(true);
    SA_s_na->write(true);  // Subtraction (guard bits accordingly)
    for (i = 0; i < WORD_64B; i++) {
        SA_op1[i]->write(0x8001FFF68064FC18);  // Op1 = 1    -10 100 -1000
        SA_op2[i]->write(0x03E800017FF67F9C);  // Op2 = 1000   1 -10  -100
    }
    wait(clk_period, SC_NS);

    SA_en->write(false);
    SA_s_na->write(false);
    for (i = 0; i < WORD_64B; i++) {
        SA_op1[i]->write(0);
        SA_op2[i]->write(0);
    }

    // Test data repacker
    // Starting from index 0
    for (i=4; i<=MASK_BITS/3; i+=2) {  // Loop through input sizes
        // Generate input word
        sw_aux1 = 2 & ((1 << (i-1)) - 1);
        sw_aux2 = -2 & ((1 << (i-1)) - 1);
        word_aux = 0;
        for (j=0; j<=64/i; j+=2) {
            word_aux |= (sw_aux1) << j*i;
            word_aux |= (sw_aux2) << (j+1)*i;
        }

        for (j=4; j<=MASK_BITS/3; j+=2) {  // Loop through output sizes
            PM_en->write(true);
            for (k = 0; k < WORD_64B; k++)
                PM_w1[k]->write(word_aux);
            PM_in_size->write(i);
            PM_out_size->write(j);
            PM_in_start->write(0);
            wait(clk_period, SC_NS);
        }
    }

    // Changing start index
    k = 0;
    for (i=4; i<=MASK_BITS/3; i+=2) {  // Loop through input sizes
        // Generate input word
        sw_aux1 = 2 & ((1 << (i-1)) - 1);
        sw_aux2 = -2 & ((1 << (i-1)) - 1);
        word_aux = 0;
        for (j=0; j<64/i; j+=2) {
            word_aux |= (sw_aux1) << j*i;
            word_aux |= (sw_aux2) << (j+1)*i;
        }

        for (j=4; j<=MASK_BITS/3; j+=2) {  // Loop through output sizes
            PM_en->write(true);
            for (m = 0; m < WORD_64B; m++)
                PM_w1[m]->write(word_aux);
            PM_in_size->write(i);
            PM_out_size->write(j);
            PM_in_start->write((k++ % (2*WORD_BITS/i)));
            wait(clk_period, SC_NS);
        }
    }
    PM_en->write(false);
    for (i = 0; i < WORD_64B; i++)
        PM_w1[i]->write(0);
    PM_in_size->write(0);
    PM_out_size->write(0);
    PM_in_start->write(0);



    // Test mask unit
    for (i=uint(MASKOP::NOP); i<uint(MASKOP::XOR)+1; i++) {
        PM_en->write(true);
        for (j = 0; j < WORD_64B; j++) {
            PM_w1[j]->write(0);
        }
        for (j = 0; j < MASK_64B; j++) {
            PM_mask_in[j]->write(0);
        }
        PM_op_sel->write(i);
        wait(clk_period, SC_NS);

        PM_en->write(true);
        for (j = 0; j < WORD_64B; j++) {
            PM_w1[j]->write(0);
        }
        for (j = 0; j < MASK_64B; j++) {
            PM_mask_in[j]->write(-1);
        }
        PM_op_sel->write(i);
        wait(clk_period, SC_NS);

        PM_en->write(true);
        for (j = 0; j < WORD_64B; j++) {
            PM_w1[j]->write(-1);
        }
        for (j = 0; j < MASK_64B; j++) {
            PM_mask_in[j]->write(0);
        }
        PM_op_sel->write(i);
        wait(clk_period, SC_NS);

        PM_en->write(true);
        for (j = 0; j < WORD_64B; j++) {
            PM_w1[j]->write(-1);
        }
        for (j = 0; j < MASK_64B; j++) {
            PM_mask_in[j]->write(-1);
        }
        PM_op_sel->write(i);
        wait(clk_period, SC_NS);

        PM_en->write(true);
        for (j = 0; j < WORD_64B; j++) {
            PM_w1[j]->write(0x5555AAAA5555AAAA);
        }
        for (j = 0; j < MASK_64B; j++) {
            PM_mask_in[j]->write(0xAAAA5555AAAA5555);
        }
        PM_op_sel->write(i);
        wait(clk_period, SC_NS);
    }
    PM_en->write(false);
    PM_w1->write(0);
    PM_mask_in->write(0);
    PM_op_sel->write(0);

    // Combine?


}
