/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Description of a generic 2 port (read and write) register file.
 *
 */

#ifndef RF_TWOPORT_H_
#define RF_TWOPORT_H_

#include "systemc.h"

template<class T, uint size>
class rf_twoport: public sc_module {
public:
    sc_in_clk   clk;
    sc_in<bool> rst;
    sc_in<uint> rd_addr;	// Index read
    sc_out<T>   rd_port;	// Read port
    sc_in<bool> wr_en;		// Enable writing
    sc_in<uint> wr_addr;	// Index the address to be written
    sc_in<T>    wr_port;    // Write port

    //Internal signals and variables
    sc_signal<T> reg[size];		// Register file contents

    SC_CTOR(rf_twoport) {
        SC_METHOD(read_method);
        sensitive << rd_addr;
        for (uint i = 0; i < size; i++)
            sensitive << reg[i];

        SC_THREAD(write_update_thread);
        sensitive << clk.pos();
        async_reset_signal_is(rst, false);

        for (uint i = 0; i < size; i++)
            reg[i] = (T) 0;

    }

    // Shows the indexed contents for reading
    void read_method() {
        if (rd_addr->read() < size)
            rd_port->write(reg[rd_addr->read()]);
        else
            rd_port->write((T) 0);
    }

    // Write to the RF
    void write_update_thread() {
        // Reset behaviour
        for (uint i = 0; i < size; i++) {
            reg[i] = (T) 0;
        }

        wait();

        // Clocked behaviour
        while (1) {
            if (wr_en->read() && wr_addr->read() < size) {
                reg[wr_addr->read()] = wr_port;
            }

            wait();
        }
    }
};

#endif /* RF_TWOPORT_H_ */
