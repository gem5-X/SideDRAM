#include "pch_main.h"

#ifdef MTI_SYSTEMC

	SC_MODULE_EXPORT(top);

#if MIXED_SIM

	void top::clock_method() {
		clk = static_cast<sc_logic>(clk_g);
	}

#endif

#else

int sc_main(int argc, char *argv[]) {
    sc_clock                        clk("clk", CLK_PERIOD, RESOLUTION);
    sc_signal<bool>                 rst;
    sc_signal<bool>                 RD;							// DRAM read command
    sc_signal<bool>                 WR;							// DRAM write command
    sc_signal<bool>                 ACT;						// DRAM activate command
//    sc_signal<bool>					RSTB;						//
    sc_signal<bool>                 AB_mode;		            // Signals if the All-Banks mode is enabled
    sc_signal<bool>                 pim_mode;			        // Signals if the PIM mode is enabled
    sc_signal<sc_uint<BANK_BITS> >  bank_addr;			        // Address of the bank
    sc_signal<sc_uint<ROW_BITS> >   row_addr;			        // Address of the bank row
    sc_signal<sc_uint<COL_BITS> >   col_addr;		            // Address of the bank column
    sc_signal<uint64_t>             DQ;	                        // Data input from DRAM controller (output makes no sense
#if DUAL_BANK_INTERFACE
    sc_signal_rv<DRAM_BITS>         even_buses[CORES_PER_PCH];  // Direct data in/out to the even banks
    sc_signal_rv<DRAM_BITS>         odd_buses[CORES_PER_PCH];   // Direct data in/out to the odd banks
#else
    sc_signal_rv<DRAM_BITS>         dram_buses[CORES_PER_PCH];  // Direct data in/out to the banks
#endif

    uint i;

#if (RECORDING || EN_MODEL)
    imc_pch dut("IMCCoreUnderTest", std::string(argv[1]));
#else
    imc_pch dut("IMCCoreUnderTest");
#endif
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
    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
        dut.even_buses[i](even_buses[i]);
        dut.odd_buses[i](odd_buses[i]);
#else
        dut.dram_buses[i](dram_buses[i]);
#endif
    }

    pch_driver driver("Driver", std::string(argv[1]));
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
    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
        driver.even_buses[i](even_buses[i]);
        driver.odd_buses[i](odd_buses[i]);
#else
        driver.dram_buses[i](dram_buses[i]);
#endif
    }

    pch_monitor monitor("Monitor");
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
    for (i = 0; i < CORES_PER_PCH; i++) {
#if DUAL_BANK_INTERFACE
        monitor.even_buses[i](even_buses[i]);
        monitor.odd_buses[i](odd_buses[i]);
#else
        monitor.dram_buses[i](dram_buses[i]);
#endif
    }

    sc_report_handler::set_actions(SC_ID_VECTOR_CONTAINS_LOGIC_VALUE_,
            SC_DO_NOTHING);

#if VCD_TRACE
    sc_trace_file *tracefile;
    tracefile = sc_create_vcd_trace_file("waveforms/pch_softsimd_wave");

    sc_trace(tracefile, clk, "clk");
    sc_trace(tracefile, rst, "rst");
    sc_trace(tracefile, RD, "RD");
    sc_trace(tracefile, WR, "WR");
    sc_trace(tracefile, ACT, "ACT");
    sc_trace(tracefile, AB_mode, "AB_mode");
    sc_trace(tracefile, pim_mode, "pim_mode");
    sc_trace(tracefile, bank_addr, "bank_addr");
    sc_trace(tracefile, row_addr, "row_addr");
    sc_trace(tracefile, col_addr, "col_addr");
    sc_trace(tracefile, DQ, "DQ");
    sc_trace(tracefile, dut.imc_cores[0]->PC, "PC");
    sc_trace(tracefile, dut.imc_cores[0]->macroinstr, "macroinstr");
    sc_trace(tracefile, dut.imc_cores[0]->cu->itt_idx, "itt_idx");
    sc_trace(tracefile, dut.imc_cores[0]->cu->dlbm_index, "dlbm_index");
    sc_trace(tracefile, dut.imc_cores[0]->cu->dlbsa_index, "dlbsa_index");
    sc_trace(tracefile, dut.imc_cores[0]->cu->dlbpm_index, "dlbpm_index");
    sc_trace(tracefile, dut.imc_cores[0]->cu->mov_instr, "mov_instr");
    sc_trace(tracefile, dut.imc_cores[0]->cu->sa_instr, "sa_instr");
    sc_trace(tracefile, dut.imc_cores[0]->cu->pm_instr, "pm_instr");
    sc_trace(tracefile, dut.imc_cores[0]->cu->mov_fields, "mov_fields");
    sc_trace(tracefile, dut.imc_cores[0]->cu->sa_fields, "sa_fields");
    sc_trace(tracefile, dut.imc_cores[0]->cu->pm_fields, "pm_fields");
    sc_trace(tracefile, dut.imc_cores[0]->cu->idm->nop_cnt_reg, "nop_reg");
    sc_trace(tracefile, dut.imc_cores[0]->vwreg[0]->reg, "VWR0");
    sc_trace(tracefile, dut.imc_cores[0]->vwreg[1]->reg, "VWR1");
#if VWR_NUM > 2
    sc_trace(tracefile, dut.imc_cores[0]->vwreg[2]->reg, "VWR2");
#if VWR_NUM > 3
    sc_trace(tracefile, dut.imc_cores[0]->vwreg[3]->reg, "VWR3");
#endif
#endif
//    sc_trace(tracefile, dut.imc_cores[0]->cu->rf_access, "rf_access");
//    sc_trace(tracefile, dut.imc_cores[0]->csdrf_in[0], "csdrf_in");
//    sc_trace(tracefile, dut.imc_cores[0]->csdrf_out[0], "csdrf_out");
//    sc_trace(tracefile, dut.imc_cores[0]->csdrf_rd_addr, "csdrf_rd_addr");
//    sc_trace(tracefile, dut.imc_cores[0]->csdrf_wr_addr, "csdrf_wr_addr");
//    sc_trace(tracefile, dut.imc_cores[0]->csdrf_wr_en, "csdrf_wr_en");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_idx[0], "VWR0_idx");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_idx[1], "VWR1_idx");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_idx[2], "VWR2_idx");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_idx[3], "VWR3_idx");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_enable[0], "VWR0_en");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_enable[1], "VWR1_en");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_enable[2], "VWR2_en");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_enable[3], "VWR3_en");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_wr_nrd[0], "VWR0_wr_nrd");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_wr_nrd[1], "VWR1_wr_nrd");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_wr_nrd[2], "VWR2_wr_nrd");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_wr_nrd[3], "VWR3_wr_nrd");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_d_nm[0], "VWR0_d_nm");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_d_nm[1], "VWR1_d_nm");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_d_nm[2], "VWR2_d_nm");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->vwr_d_nm[3], "VWR3_d_nm");
    sc_trace(tracefile, dut.imc_cores[0]->R_out[0][0], "R0");
    sc_trace(tracefile, dut.imc_cores[0]->R_out[1][0], "R1");
    sc_trace(tracefile, dut.imc_cores[0]->R_out[2][0], "R2");
    sc_trace(tracefile, dut.imc_cores[0]->R_out[3][0], "R3");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->op1[0], "sa_op1");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->op2[0], "sa_op2");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->adder_en, "sa_adder_en");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->neg_op1, "sa_neg_op1");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->neg_op2, "sa_neg_op2");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->shift, "sa_shift");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->shift_out[0], "sa_shifted");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->and_or_out1[0], "sa_and_or_out1");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->and_or_out2[0], "sa_and_or_out2");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->decoded_size, "sa_size");
    sc_trace(tracefile, dut.imc_cores[0]->sa_stage->output[0], "sa_out");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->w1[0], "pm_w1");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->w2[0], "pm_w2");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->decoded_in_size, "pm_in_size");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->decoded_out_size, "pm_out_size");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->in_start, "pm_in_start");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->shift, "pm_shift");
    sc_trace(tracefile, dut.imc_cores[0]->pm_stage->output[0], "pm_out");
    sc_trace(tracefile, dut.imc_cores[0]->cu->ms_en, "ms_enable");
    sc_trace(tracefile, dut.imc_cores[0]->cu->ms->state_out, "ms_state");
    sc_trace(tracefile, dut.imc_cores[0]->cu->csd_len, "ms_csd_len");
    sc_trace(tracefile, dut.imc_cores[0]->cu->csd_mult[0], "ms_csd_in");
    sc_trace(tracefile, dut.imc_cores[0]->cu->ms_mov_valid, "ms_mov_valid");
    sc_trace(tracefile, dut.imc_cores[0]->cu->ms_sa_valid, "ms_sa_valid");
    sc_trace(tracefile, dut.imc_cores[0]->cu->ms_sa_index, "ms_sa_index");
//    sc_trace(tracefile, dut.imc_cores[0]->cu->ms_sa_shift, "ms_sa_shift");
    sc_trace(tracefile, dut.imc_cores[0]->ts->shuffle_reg, "ts_reg");
    sc_trace(tracefile, dut.imc_cores[0]->ts->input_en, "ts_in_en");
    sc_trace(tracefile, dut.imc_cores[0]->ts->output_en, "ts_out_en");
    sc_trace(tracefile, dut.imc_cores[0]->ts->out_start, "ts_out_start");
    sc_trace(tracefile, dut.imc_cores[0]->ts->mode_test, "ts_mode");
#if DUAL_BANK_INTERFACE
    sc_trace(tracefile, even_buses[0], "even_bus");
    sc_trace(tracefile, odd_buses[0], "odd_bus");
#else
    sc_trace(tracefile, dram_buses[0], "dram_bus");
#endif
#endif // VCD_TRACE

    sc_start();

#if VCD_TRACE
    sc_close_vcd_trace_file(tracefile);
#endif

#if EN_MODEL
    for (i = 0; i < CORES_PER_PCH; i++) {
        dut.imc_cores[i]->cu->write_energy_stats();
    }
#endif

    return 0;
}

#endif
