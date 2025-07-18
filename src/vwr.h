/*
 * Copyright EPFL 2024
 * Rafael Medina Morillas
 *
 * Description of a Very Wide Register (VWR).
 *
 */

#ifndef SRC_VWR_H_
#define SRC_VWR_H_

#include "systemc.h"

#include "cnm_base.h"
#include "tristate_buffer.h"

template<uint large_width, uint small_width, uint max_idx>
class vwr: public sc_module {
public:

#ifndef __SYNTHESIS__

    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 enable; // Enable access
    sc_in<bool>                 wr_nrd; // Write (1) or Read (0)
    sc_in<uint>                 idx;    // Indexes the mux/demux
    sc_in<bool>                 mask_en;// Enables using the mask
    sc_in<uint64_t>             mask;   // Mask VWR-wide access
    sc_in<bool>                 d_nm;   // Selects if multiplexing (0) or demultiplexing (1)
    sc_inout_rv<large_width>    lport;  // VWR wide Read/Write port
    sc_inout_rv<small_width>    sport;  // Word wide Read/Write port

    // Internal signals and variables
    sc_signal<sc_lv<large_width> > reg, reg_nxt, lbus_out;  // Register file contents
    sc_signal<sc_lv<small_width> > sbus_out;  // Register file contents
    sc_signal<bool> lbuf_en, sbuf_en;     // Enable tri-state buffer outputs

    // Internal modules
    tristate_buffer<large_width> *lbuffer;
    tristate_buffer<small_width> *sbuffer;

    SC_CTOR(vwr) {

        lbuffer = new tristate_buffer<large_width>("VWR_tristate_buffer");
        lbuffer->input(lbus_out);
        lbuffer->enable(lbuf_en);
        lbuffer->output(lport);

        sbuffer = new tristate_buffer<small_width>("Word_tristate_buffer");
        sbuffer->input(sbus_out);
        sbuffer->enable(sbuf_en);
        sbuffer->output(sport);

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << reg << lport << sport << idx << mask_en << mask << enable << wr_nrd << d_nm;

        reg = 0;
    }

    // Clocked behavior
    void clk_thread() {
        // Reset behavior
        reg = 0;

        wait();

        // Clocked behavior
        while(1) {
            if (enable->read() && wr_nrd->read()) {
                reg = reg_nxt;
            }

            wait();
        }
    }

    // Update internal signals
    void comb_method() {

        // Default signal and variables
        sc_lv<large_width> reg_aux = reg;
        reg_nxt = reg;

        // Read from ports
        sc_lv<large_width> lbus_in = lport;
        sc_lv<small_width> sbus_in = sport;
        sc_biguint<max_idx> mask_in;
#if FLEXIBLE_MASK
        mask_in = mask->read();
#else
        if (mask->read()) {
            mask_in[mask->read() - 1] = 1;  // Access word in (mask - 1)
        } else {
            for (uint i = 0; i < max_idx; i++) {
                mask_in[i] = 1;
            }
        }
#endif

        // Default write to the tristate buffer inputs
        lbus_out = reg;
        sbus_out = reg_aux.range(small_width-1, 0);

        if (idx->read() < max_idx) {    // If index within the VWR

            // Writing from the small port (demultiplex)
            if (wr_nrd->read() && d_nm->read()) {
                reg_aux.range((idx + 1) * small_width - 1, idx * small_width) = sbus_in;
            }
            // Reading from the small port (multiplex)
            if (!wr_nrd->read() && !d_nm->read()) {
                sbus_out = reg_aux.range((idx + 1) * small_width - 1, idx * small_width);
            }
        }

        if (wr_nrd->read() && !d_nm->read()) {  // If writing and multiplexing, write from lport, else from sport
        // Mask access to the large word according to the control signal (only write, as read should be handled outside)
            if (mask_en->read()) {
                for (uint i = 0; i < max_idx; i++) {
                    if (mask_in[i]) {
                        reg_aux.range((i+1)*small_width-1, i*small_width) = lbus_in.range((i+1)*small_width-1, i*small_width);
                    }
                }
            } else {
                reg_aux = lbus_in;
            }
        }

        if (max_idx*small_width < large_width) {
            reg_aux.range(large_width-1, max_idx*small_width) = 0; // The extra bits are zero (for the moment)
        }
        reg_nxt = reg_aux;

        // Manage tristate buffers
        lbuf_en = enable->read() && !wr_nrd->read();
        sbuf_en = !d_nm->read() && !wr_nrd->read();
    }

#else   // __SYNTHESIS__

    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 enable;     // Enable access
    sc_in<bool>                 wr_nrd;     // Write (1) or Read (0)
    sc_in<uint>                 idx;        // Indexes the mux/demux
    sc_in<bool>                 mask_en;    // Enables using the mask
    sc_in<uint64_t>             mask;       // Mask VWR-wide access
    sc_in<bool>                 d_nm;       // Selects if multiplexing (0) or demultiplexing (1)
    sc_in<sc_lv<large_width> >  lport_in;   // VWR wide write port
    sc_in<sc_lv<small_width> >  sport_in;   // Word wide write port
    sc_out<sc_lv<large_width> > lport_out;  // VWR wide write port
    sc_out<sc_lv<small_width> > sport_out;  // Word wide write port

    // Internal signals and variables
    sc_signal<sc_lv<large_width> > reg, reg_nxt;  // Register file contents
    sc_lv<large_width> lbus_out;  // Register file contents
    sc_lv<small_width> sbus_out;  // Register file contents

    // Internal modules

    SC_CTOR(vwr) {

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << reg << lport_in << sport_in << idx << mask_en << mask << enable << wr_nrd << d_nm;

        reg = 0;
    }

    template<uint idx>
    void write_from_sport(sc_lv<small_width> input, sc_lv<large_width> &output) {
        if (idx < max_idx)
            output.range((idx+1)*small_width-1, idx*small_width) = input;
    }

    void write_from_sport_switch(sc_lv<small_width> input, sc_lv<large_width> &output, uint idx) {
        switch (idx) {
            case 0:     write_from_sport<0>(input, output);  break;
            case 1:     write_from_sport<1>(input, output);  break;
            case 2:     write_from_sport<2>(input, output);  break;
            case 3:     write_from_sport<3>(input, output);  break;
            case 4:     write_from_sport<4>(input, output);  break;
            case 5:     write_from_sport<5>(input, output);  break;
            case 6:     write_from_sport<6>(input, output);  break;
            case 7:     write_from_sport<7>(input, output);  break;
            case 8:     write_from_sport<8>(input, output);  break;
            case 9:     write_from_sport<9>(input, output);  break;
            case 10:    write_from_sport<10>(input, output); break;
            case 11:    write_from_sport<11>(input, output); break;
            case 12:    write_from_sport<12>(input, output); break;
            case 13:    write_from_sport<13>(input, output); break;
            case 14:    write_from_sport<14>(input, output); break;
            case 15:    write_from_sport<15>(input, output); break;
            case 16:    write_from_sport<16>(input, output); break;
            default:    break;
        }
    }

    template<uint idx>
    void read_to_sport(sc_lv<large_width> input, sc_lv<small_width> &output) {
        if (idx < max_idx)
            output = input.range((idx+1)*small_width-1, idx*small_width);
    }

    void read_to_sport_switch(sc_lv<large_width> input, sc_lv<small_width> &output, uint idx) {
        switch (idx) {
            case 0:     read_to_sport<0>(input, output);  break;
            case 1:     read_to_sport<1>(input, output);  break;
            case 2:     read_to_sport<2>(input, output);  break;
            case 3:     read_to_sport<3>(input, output);  break;
            case 4:     read_to_sport<4>(input, output);  break;
            case 5:     read_to_sport<5>(input, output);  break;
            case 6:     read_to_sport<6>(input, output);  break;
            case 7:     read_to_sport<7>(input, output);  break;
            case 8:     read_to_sport<8>(input, output);  break;
            case 9:     read_to_sport<9>(input, output);  break;
            case 10:    read_to_sport<10>(input, output); break;
            case 11:    read_to_sport<11>(input, output); break;
            case 12:    read_to_sport<12>(input, output); break;
            case 13:    read_to_sport<13>(input, output); break;
            case 14:    read_to_sport<14>(input, output); break;
            case 15:    read_to_sport<15>(input, output); break;
            case 16:    read_to_sport<16>(input, output); break;
            default:    break;
        }
    }

    // Clocked behavior
    void clk_thread() {
        // Reset behavior
        reg = 0;

        wait();

        // Clocked behavior
        while(1) {
            if (enable->read() && wr_nrd->read()) {
                reg = reg_nxt;
            }

            wait();
        }
    }

    // Update internal signals
    void comb_method() {

        // Make up for the lack of tri-state buffers
//        lport_out = lport_in; // These create combinational loops
//        sport_out = sport_in;
        lport_out->write(0);
        sport_out->write(0);

        // Default signal and variables
        sc_lv<large_width> reg_aux = reg;
        reg_nxt = reg;

        // Read from ports
        sc_lv<large_width> lbus_in = lport_in;
        sc_lv<small_width> sbus_in = sport_in;
        sc_biguint<max_idx> mask_in;
#if FLEXIBLE_MASK
        mask_in = mask->read();
#else
        if (mask->read()) {
            mask_in[mask->read() - 1] = 1;  // Access word in (mask - 1)
        } else {
            for (uint i = 0; i < max_idx; i++) {
                mask_in[i] = 1;
            }
        }
#endif

        // Default write to the tristate buffer inputs
        lbus_out = reg;
        sbus_out = reg_aux.range(small_width-1, 0);

        if (idx->read() < max_idx) {    // If index within the VWR

            // Writing from the small port (demultiplex)
            if (wr_nrd->read() && d_nm->read()) {
                write_from_sport_switch(sbus_in, reg_aux, idx);
            }
            // Reading from the small port (multiplex)
            if (!wr_nrd->read() && !d_nm->read()) {
                sc_lv<small_width> sbus_aux;
                read_to_sport_switch(reg_aux, sbus_aux, idx);
                sbus_out = sbus_aux;
            }
        }

        if (wr_nrd->read() && !d_nm->read()) {  // If writing and multiplexing, write from lport, else from sport
        // Mask access to the large word according to the control signal (only write, as read should be handled outside)
            if (mask_en->read()) {
                for (uint i = 0; i < max_idx; i++) {
                    if (mask_in[i]) {
                        reg_aux.range((i+1)*small_width-1, i*small_width) = lbus_in.range((i+1)*small_width-1, i*small_width);
                    }
                }
            } else {
                reg_aux = lbus_in;
            }
        }

        

        if (max_idx*small_width < large_width) {
            reg_aux.range(large_width-1, max_idx*small_width) = 0; // The extra bits are zero (for the moment)
        }
        reg_nxt = reg_aux;

        // Manage (lack of) tristate buffers
        if (enable->read() && !wr_nrd->read()) {
            lport_out->write(lbus_out);
        }

        if (!d_nm->read() && !wr_nrd->read()) {
            sport_out->write(sbus_out);
        }
    }

#endif  // __SYNTHESIS__

};

#if VWR_DRAM_CLK > 1
template<uint large_width, uint small_width, uint dram_width>
class vwr_multicycle: public sc_module {
public:
    sc_in_clk                   clk;
    sc_in<bool>                 rst;
    sc_in<bool>                 enable;     // Enable access
    sc_in<bool>                 wr_nrd;     // Write (1) or Read (0)
    sc_in<uint>                 dram_idx;   // Indexes the DRAM mux/demux
    sc_in<bool>                 dram_d_nm;  // Selects if multiplexing (0) or demultiplexing (1) from/to DRAM
    sc_in<uint>                 idx;        // Indexes the mux/demux
    sc_in<bool>                 d_nm;       // Selects if multiplexing (0) or demultiplexing (1)
    sc_inout_rv<large_width>    lport;      // VWR wide Read/Write port
    sc_inout_rv<small_width>    sport;      // Word wide Read/Write port
    sc_inout_rv<dram_width>     dport;      // DRAM wide Read/Write port

    // Internal signals and variables
    uint max_dram_idx = large_width / dram_width;
    uint max_idx = large_width / small_width;
    sc_signal<sc_lv<large_width> > reg, reg_nxt, lbus_out;  // Register file contents
    sc_signal<sc_lv<small_width> > sbus_out;  // Register file contents
    sc_signal<sc_lv<dram_width> > dbus_out;  // Register file contents
    sc_signal<bool> lbuf_en, sbuf_en, dbuf_en;  // Enable tri-state buffer outputs

    // Internal modules
    tristate_buffer<large_width> *lbuffer;
    tristate_buffer<small_width> *sbuffer;
    tristate_buffer<dram_width> *dbuffer;

    SC_CTOR(vwr_multicycle) {

        lbuffer = new tristate_buffer<large_width>("VWR_tristate_buffer");
        lbuffer->input(lbus_out);
        lbuffer->enable(lbuf_en);
        lbuffer->output(lport);

        sbuffer = new tristate_buffer<small_width>("Word_tristate_buffer");
        sbuffer->input(sbus_out);
        sbuffer->enable(sbuf_en);
        sbuffer->output(sport);

        dbuffer = new tristate_buffer<dram_width>("DRAM_tristate_buffer");
        dbuffer->input(dbus_out);
        dbuffer->enable(dbuf_en);
        dbuffer->output(dport);

        SC_THREAD(clk_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        SC_METHOD(comb_method);
        sensitive << reg << lport << sport << dport << dram_idx << dram_d_nm << idx << enable << wr_nrd << d_nm;

        reg = 0;
    }

    // Clocked behavior
    void clk_thread() {
        // Reset behavior
        reg = 0;

        wait();

        // Clocked behavior
        while(1) {
            if (enable->read() && wr_nrd->read()) {
                reg = reg_nxt;
            }

            wait();
        }
    }

    // Update internal signals
    void comb_method() {

        // Default signal and variables
        sc_lv<large_width> reg_aux = reg;
        reg_nxt = reg;

        // Read from ports
        sc_lv<large_width> lbus_in = lport;
        sc_lv<small_width> sbus_in = sport;
        sc_lv<dram_width> dbus_in = dport;

        // Default write to the tristate buffer inputs
        lbus_out = reg;
        sbus_out = reg_aux.range(small_width-1, 0);
        dbus_out = reg_aux.range(dram_width-1, 0);

        if (dram_idx->read() < max_dram_idx) {    // If index within the VWR

            // Demultiplex
            if (dram_d_nm->read()) {
                reg_aux.range((dram_idx + 1) * dram_width - 1, dram_idx * dram_width) = dbus_in;
            // Multiplex
            } else {
                dbus_out = reg_aux.range((dram_idx + 1) * dram_width - 1, dram_idx * dram_width);
            }
        }

        if (idx->read() < max_idx) {    // If index within the VWR

            // Demultiplex
            if (d_nm->read() && !dram_d_nm->read()) {   // If demultiplexing dram bus, we don't demux here
                reg_aux.range((idx + 1) * small_width - 1, idx * small_width) = sbus_in;
            // Multiplex
            } else {
                sbus_out = reg_aux.range((idx + 1) * small_width - 1, idx * small_width);
            }
        }

        if (wr_nrd->read() && !d_nm->read() && !dram_d_nm->read()) {    // If writing and multiplexing, write from lport, else from sport or dport
            reg_aux = lbus_in;
        }
        if (max_idx*small_width < large_width) {
            reg_aux.range(large_width-1, max_idx*small_width) = 0; // The extra bits are zero (for the moment)
        }
        reg_nxt = reg_aux;

        // Manage tristate buffers
        lbuf_en = enable->read() && !wr_nrd->read();
        sbuf_en = !d_nm->read() && !wr_nrd->read();
        dbuf_en = !dram_d_nm->read() && !wr_nrd->read();
    }
};
#endif  // VWR_DRAM_CLK > 1

#endif /* SRC_VWR_H_ */
