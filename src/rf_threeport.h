/*
 * Copyright EPFL 2021
 * Rafael Medina Morillas
 *
 * Description of a generic 3 port (2 read, 1 write) register file.
 *
 */

#ifndef RF_THREEPORT_H_
#define RF_THREEPORT_H_

#include "systemc.h"

template<class T, uint size>
class rf_threeport: public sc_module {
public:
    sc_in_clk clk;
    sc_in<bool> rst;
    sc_in<uint> rd_addr1;	// Index read
    sc_in<uint> rd_addr2;	// Index read
    sc_out<T>   rd_port1;	// Read port
    sc_out<T>   rd_port2;	// Read port
    sc_in<bool> wr_en;		// Enable writing
    sc_in<uint> wr_addr;	// Index the address to be written
    sc_in<T>    wr_port;	// Write port

    //Internal signals and variables
    sc_signal<T> reg[size];		// Register file contents

    SC_CTOR(rf_threeport) {
        SC_METHOD(read_method);
        sensitive << rd_addr1 << rd_addr2;
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
        if (rd_addr1->read() < size)
            rd_port1->write(reg[rd_addr1->read()]);
        else
            rd_port1->write((T) 0);
        if (rd_addr2->read() < size)
            rd_port2->write(reg[rd_addr2->read()]);
        else
            rd_port2->write((T) 0);
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

#endif /* RF_THREEPORT_H_ */
