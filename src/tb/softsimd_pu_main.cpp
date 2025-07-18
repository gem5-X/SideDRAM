#include "softsimd_pu_main.h"

#ifdef MTI_SYSTEMC

    SC_MODULE_EXPORT(top);

#else

int sc_main(int argc, char *argv[]) {
    sc_clock                        clk("clk", CLK_PERIOD, RESOLUTION);
    sc_signal<bool>                 rst;
    sc_signal<bool>                 RD;         // DRAM read command
    sc_signal<bool>                 WR;         // DRAM write command
    sc_signal<bool>                 ACT;        // DRAM activate command
//    sc_signal<bool>                   RSTB;       //
    sc_signal<bool>                 AB_mode;    // Signals if the All-Banks mode is enabled
    sc_signal<bool>                 pim_mode;   // Signals if the PIM mode is enabled
    sc_signal<sc_uint<BANK_BITS> >  bank_addr;  // Address of the bank
    sc_signal<sc_uint<ROW_BITS> >   row_addr;   // Address of the bank row
    sc_signal<sc_uint<COL_BITS> >   col_addr;   // Address of the bank column
    sc_signal<uint64_t>             DQ;         // Data input from DRAM controller (output makes no sense
#if DUAL_BANK_INTERFACE
    sc_signal_rv<DRAM_BITS>         even_bus;   // Direct data in/out to the even bank
    sc_signal_rv<DRAM_BITS>         odd_bus;    // Direct data in/out to the odd bank
#else
    sc_signal_rv<DRAM_BITS>         dram_bus;   // Direct data in/out to the DRAM
#endif

    softsimd_pu dut("IMCCoreUnderTest");
    dut.clk(clk);
    dut.rst(rst);
    dut.RD(RD);
    dut.WR(WR);
    dut.ACT(ACT);
//    dut.RSTB(RSTB);
    dut.AB_mode(AB_mode);
    dut.pim_mode(pim_mode);
    dut.bank_addr(bank_addr);
    dut.row_addr(row_addr);
    dut.col_addr(col_addr);
    dut.DQ(DQ);
#if DUAL_BANK_INTERFACE
    dut.even_bus(even_bus);
    dut.odd_bus(odd_bus);
#else
    dut.dram_bus(dram_bus);
#endif

    softsimd_pu_driver driver("Driver");
    driver.rst(rst);
    driver.RD(RD);
    driver.WR(WR);
    driver.ACT(ACT);
//    driver.RSTB(RSTB);
    driver.AB_mode(AB_mode);
    driver.pim_mode(pim_mode);
    driver.bank_addr(bank_addr);
    driver.row_addr(row_addr);
    driver.col_addr(col_addr);
    driver.DQ(DQ);
#if DUAL_BANK_INTERFACE
    driver.even_bus(even_bus);
    driver.odd_bus(odd_bus);
#else
    driver.dram_bus(dram_bus);
#endif

    softsimd_pu_monitor monitor("Monitor");
    monitor.clk(clk);
    monitor.rst(rst);
    monitor.RD(RD);
    monitor.WR(WR);
    monitor.ACT(ACT);
//    monitor.RSTB(RSTB);
    monitor.AB_mode(AB_mode);
    monitor.pim_mode(pim_mode);
    monitor.bank_addr(bank_addr);
    monitor.row_addr(row_addr);
    monitor.col_addr(col_addr);
    monitor.DQ(DQ);
#if DUAL_BANK_INTERFACE
    monitor.even_bus(even_bus);
    monitor.odd_bus(odd_bus);
#else
    monitor.dram_bus(dram_bus);
#endif

    sc_report_handler::set_actions(SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_,
            SC_DO_NOTHING);

    sc_trace_file *tracefile;
    tracefile = sc_create_vcd_trace_file("waveforms/softsimd_microcode_wave");

    sc_trace(tracefile, clk, "clk");
    sc_trace(tracefile, rst, "rst");
    sc_trace(tracefile, dut.cu->rf_access, "rf_access");
    sc_trace(tracefile, dut.cu->decode_en, "decode_en");
    sc_trace(tracefile, RD, "RD");
    sc_trace(tracefile, WR, "WR");
    sc_trace(tracefile, ACT, "ACT");
//    sc_trace(tracefile, RSTB, "RSTB");
    sc_trace(tracefile, AB_mode, "AB_mode");
    sc_trace(tracefile, pim_mode, "pim_mode");
    sc_trace(tracefile, bank_addr, "bank_addr");
    sc_trace(tracefile, row_addr, "row_addr");
    sc_trace(tracefile, col_addr, "col_addr");
    sc_trace(tracefile, DQ, "DQ");
    sc_trace(tracefile, dut.cu_data_out, "cu_data_out");
    sc_trace(tracefile, dut.PC, "PC");
    sc_trace(tracefile, dut.macroinstr, "macroinstr");
    sc_trace(tracefile, dut.cu->itt_idx, "ITT_IDX");
    sc_trace(tracefile, dut.cu->common_reg, "common_fields_reg");
    sc_trace(tracefile, dut.cu->idm_decode_en, "MOV_dec_en");
    sc_trace(tracefile, dut.cu->idsa_decode_en, "SA_dec_en");
    sc_trace(tracefile, dut.cu->idpm_decode_en, "PM_dec_en");
    sc_trace(tracefile, dut.cu->dlbm_index, "MOV_index");
    sc_trace(tracefile, dut.cu->dlbsa_index, "SA_index");
    sc_trace(tracefile, dut.cu->dlbpm_index, "PM_index");
    sc_trace(tracefile, dut.cu->mov_fields, "MOV_fields");
    sc_trace(tracefile, dut.cu->sa_fields, "SA_fields");
    sc_trace(tracefile, dut.cu->pm_fields, "PM_fields");
#if DUAL_BANK_INTERFACE
    sc_trace(tracefile, even_bus, "even_bus");
    sc_trace(tracefile, odd_bus, "odd_bus");
#else
    sc_trace(tracefile, dut.dram_out_en, "dram_out_en");
    sc_trace(tracefile, dut.dram_from, "dram_from");
    sc_trace(tracefile, dram_bus, "dram_bus");
#endif
#if HW_LOOP
    sc_trace(tracefile, dut.cu->pcu->loop_reg_en, "loop_reg_en");
    sc_trace(tracefile, dut.cu->pcu->loop_sta_addr, "loop_sta_addr");
    sc_trace(tracefile, dut.cu->pcu->loop_end_addr, "loop_end_addr");
    sc_trace(tracefile, dut.cu->pcu->loop_num_iter, "loop_num_iter");
    sc_trace(tracefile, dut.cu->pcu->loop_sta_addr_reg, "loop_sta_addr_reg");
    sc_trace(tracefile, dut.cu->pcu->loop_end_addr_reg, "loop_end_addr_reg");
    sc_trace(tracefile, dut.cu->pcu->loop_num_iter_reg, "loop_num_iter_reg");
    sc_trace(tracefile, dut.cu->pcu->loop_curr_iter_reg, "loop_curr_iter_reg");
#endif
    sc_trace(tracefile, dut.cu->ms->enable, "ms_enable");
    sc_trace(tracefile, dut.cu->ms->csd_in[0], "ms_csd_in");
    sc_trace(tracefile, dut.cu->ms->csd_len, "ms_csd_len");
    sc_trace(tracefile, dut.cu->ms->sa_valid, "ms_sa_valid");
    sc_trace(tracefile, dut.cu->ms->sa_index, "ms_sa_index");
    sc_trace(tracefile, dut.cu->ms->state_out, "ms_state");
    sc_trace(tracefile, dut.ib_wr_en, "ib_wr_en");
    sc_trace(tracefile, dut.ib_wr_addr, "ib_wr_addr");
    sc_trace(tracefile, dut.ib_in, "ib_in");
    sc_trace(tracefile, dut.ts_in_en, "ts_in_en");
    sc_trace(tracefile, dut.ts_out_en, "ts_out_en");
    sc_trace(tracefile, dut.ts_out_start, "ts_out_start");
    sc_trace(tracefile, dut.ts_mode, "ts_mode");
    sc_trace(tracefile, dut.ts_shf_from, "ts_shf_from");
    sc_trace(tracefile, dut.ts->shuffle_reg, "ts_reg");
    sc_trace(tracefile, dut.ts_inout, "ts_inout");
    sc_trace(tracefile, dut.vwr_enable[0], "vwr_enable_0");
    sc_trace(tracefile, dut.vwr_wr_nrd[0], "vwr_wr_nrd_0");
#if VWR_DRAM_CLK > 1
    sc_trace(tracefile, dut.vwr_dram_d_nm[0], "vwr_dram_d_nm_0");
    sc_trace(tracefile, dut.vwr_dram_idx, "vwr_dram_idx");
#endif
    sc_trace(tracefile, dut.vwr_d_nm[0], "vwr_d_nm_0");
    sc_trace(tracefile, dut.vwr_idx[0], "vwr_idx_0");
    sc_trace(tracefile, dut.vwr_mask[0], "vwr_mask_0");
    sc_trace(tracefile, dut.vwr_from[0], "vwr_from_0");
    sc_trace(tracefile, dut.vwr_inout[0], "vwr_inout_0");
    sc_trace(tracefile, dut.vwr_muxed[0], "vwr_muxed_0");
    sc_trace(tracefile, dut.vwreg[0]->reg, "vwr_0");
    sc_trace(tracefile, dut.vwr_enable[1], "vwr_enable_1");
    sc_trace(tracefile, dut.vwr_wr_nrd[1], "vwr_wr_nrd_1");
#if VWR_DRAM_CLK > 1
    sc_trace(tracefile, dut.vwr_dram_d_nm[1], "vwr_dram_d_nm_1");
#endif
    sc_trace(tracefile, dut.vwr_d_nm[1], "vwr_d_nm_1");
    sc_trace(tracefile, dut.vwr_idx[1], "vwr_idx_1");
    sc_trace(tracefile, dut.vwr_mask[1], "vwr_mask_1");
    sc_trace(tracefile, dut.vwr_from[1], "vwr_from_1");
    sc_trace(tracefile, dut.vwr_inout[1], "vwr_inout_1");
    sc_trace(tracefile, dut.vwr_muxed[1], "vwr_muxed_1");
    sc_trace(tracefile, dut.vwreg[1]->reg, "vwr_1");
#if VWR_NUM > 2
    sc_trace(tracefile, dut.vwr_enable[2], "vwr_enable_2");
    sc_trace(tracefile, dut.vwr_wr_nrd[2], "vwr_wr_nrd_2");
#if VWR_DRAM_CLK > 1
    sc_trace(tracefile, dut.vwr_dram_d_nm[2], "vwr_dram_d_nm_2");
#endif
    sc_trace(tracefile, dut.vwr_d_nm[2], "vwr_d_nm_2");
    sc_trace(tracefile, dut.vwr_idx[2], "vwr_idx_2");
    sc_trace(tracefile, dut.vwr_mask[2], "vwr_mask_2");
    sc_trace(tracefile, dut.vwr_from[2], "vwr_from_2");
    sc_trace(tracefile, dut.vwr_inout[2], "vwr_inout_2");
    sc_trace(tracefile, dut.vwr_muxed[2], "vwr_muxed_2");
    sc_trace(tracefile, dut.vwreg[2]->reg, "vwr_2");
#if VWR_NUM > 3
    sc_trace(tracefile, dut.vwr_enable[3], "vwr_enable_3");
    sc_trace(tracefile, dut.vwr_wr_nrd[3], "vwr_wr_nrd_3");
#if VWR_DRAM_CLK > 1
    sc_trace(tracefile, dut.vwr_dram_d_nm[3], "vwr_dram_d_nm_3");
#endif
    sc_trace(tracefile, dut.vwr_d_nm[3], "vwr_d_nm_3");
    sc_trace(tracefile, dut.vwr_idx[3], "vwr_idx_3");
    sc_trace(tracefile, dut.vwr_mask[3], "vwr_mask_3");
    sc_trace(tracefile, dut.vwr_from[3], "vwr_from_3");
    sc_trace(tracefile, dut.vwr_inout[3], "vwr_inout_3");
    sc_trace(tracefile, dut.vwr_muxed[3], "vwr_muxed_3");
    sc_trace(tracefile, dut.vwreg[3]->reg, "vwr_3");
#endif
#endif
    sc_trace(tracefile, dut.reg_en[0], "reg_en_0");
    sc_trace(tracefile, dut.reg_from[0], "reg_from_0");
    sc_trace(tracefile, dut.R_in[0][0], "R_in_0");
    sc_trace(tracefile, dut.R_out[0][0], "R_out_0");
    sc_trace(tracefile, dut.reg_en[1], "reg_en_1");
    sc_trace(tracefile, dut.reg_from[1], "reg_from_1");
    sc_trace(tracefile, dut.R_in[1][0], "R_in_1");
    sc_trace(tracefile, dut.R_out[1][0], "R_out_1");
    sc_trace(tracefile, dut.reg_en[2], "reg_en_2");
    sc_trace(tracefile, dut.reg_from[2], "reg_from_2");
    sc_trace(tracefile, dut.R_in[2][0], "R_in_2");
    sc_trace(tracefile, dut.R_out[2][0], "R_out_2");
    sc_trace(tracefile, dut.reg_en[3], "reg_en_3");
    sc_trace(tracefile, dut.reg_from[3], "reg_from_3");
    sc_trace(tracefile, dut.R_in[3][0], "R_in_3");
    sc_trace(tracefile, dut.R_out[3][0], "R_out_3");
    sc_trace(tracefile, dut.srf_rd_addr, "srf_rd_addr");
    sc_trace(tracefile, dut.srf_wr_addr, "srf_wr_addr");
    sc_trace(tracefile, dut.srf_wr_en, "srf_wr_en");
    sc_trace(tracefile, dut.srf_wr_from, "srf_wr_from");
    sc_trace(tracefile, dut.srf_in[0], "srf_in");
    sc_trace(tracefile, dut.srf_out[0], "srf_out");
    sc_trace(tracefile, dut.mrf_rd_addr, "mrf_rd_addr");
    sc_trace(tracefile, dut.mrf_wr_addr, "mrf_wr_addr");
    sc_trace(tracefile, dut.mrf_wr_en, "mrf_wr_en");
    sc_trace(tracefile, dut.mrf_wr_from, "mrf_wr_from");
    sc_trace(tracefile, dut.mrf_in[0], "mrf_in");
    sc_trace(tracefile, dut.mrf_out[0], "mrf_out");
    sc_trace(tracefile, dut.csdrf_rd_addr, "csdrf_rd_addr");
    sc_trace(tracefile, dut.csdrf_wr_addr, "csdrf_wr_addr");
    sc_trace(tracefile, dut.csdrf_wr_en, "csdrf_wr_en");
    sc_trace(tracefile, dut.csdrf_wr_from, "csdrf_wr_from");
    sc_trace(tracefile, dut.csdrf_in[0], "csdrf_in");
    sc_trace(tracefile, dut.csdrf_out[0], "csdrf_out");
    sc_trace(tracefile, dut.sa_en, "sa_en");
    sc_trace(tracefile, dut.sa_adder_en, "sa_adder_en");
    sc_trace(tracefile, dut.sa_s_na, "sa_s_na");
    sc_trace(tracefile, dut.sa_shift, "sa_shift");
    sc_trace(tracefile, dut.sa_stage->decoded_size, "sa_size");
    sc_trace(tracefile, dut.sa_op1_from, "sa_op1_from");
    sc_trace(tracefile, dut.sa_op2_from, "sa_op2_from");
    sc_trace(tracefile, dut.sa_op1[0], "sa_op1");
    sc_trace(tracefile, dut.sa_op2[0], "sa_op2");
    sc_trace(tracefile, dut.sa_stage->shift_out[0], "sa_shifted");
    sc_trace(tracefile, dut.sa_stage->and_or_out[0], "sa_masked_op1");
    sc_trace(tracefile, dut.sa_out[0], "sa_out");
    sc_trace(tracefile, dut.pm_en, "pm_en");
    sc_trace(tracefile, dut.pm_stage->decoded_in_size, "pm_in_size");
    sc_trace(tracefile, dut.pm_stage->decoded_out_size, "pm_out_size");
    sc_trace(tracefile, dut.pm_in_start, "pm_in_start");
    sc_trace(tracefile, dut.pm_shift, "pm_shift");
    sc_trace(tracefile, dut.pm_op_sel, "pm_op_sel");
    sc_trace(tracefile, dut.pm_op1_from, "pm_op1_from");
    sc_trace(tracefile, dut.pm_op2_from, "pm_op2_from");
    sc_trace(tracefile, dut.pm_w1[0], "pm_w1");
    sc_trace(tracefile, dut.pm_w2[0], "pm_w2");
    sc_trace(tracefile, dut.pm_out[0], "pm_out");

    sc_start();

    sc_close_vcd_trace_file(tracefile);

    return 0;
}

#endif
