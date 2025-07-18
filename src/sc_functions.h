/*
 * sc_functions.h
 *
 *  Created on: Jan 16, 2024
 *      Author: rafa
 */

#ifndef SRC_SC_FUNCTIONS_H_
#define SRC_SC_FUNCTIONS_H_

#include "systemc.h"
#include "microcode/common_format.h"
#include "opcodes.h"

// Microcode ITT fields
ostream& operator<< (ostream& os, const itt_field& field);
void sc_trace (sc_trace_file*& tf, const itt_field& field, std::string nm);

// ADD opcodes
ostream& operator<< (ostream& os, const ADDOP& addop);
void sc_trace (sc_trace_file*& tf, const ADDOP& addop, std::string nm);

// Storage opcodes
ostream& operator<< (ostream& os, const OPC_STORAGE& store);
void sc_trace (sc_trace_file*& tf, const OPC_STORAGE& store, std::string nm);

// MUX control signals
ostream& operator<< (ostream& os, const MUX& mux);
void sc_trace (sc_trace_file*& tf, const MUX& mux, std::string nm);

// RF selection opcodes
ostream& operator<< (ostream& os, const RF_SEL& sel);
void sc_trace (sc_trace_file*& tf, const RF_SEL& sel, std::string nm);

// SubWord size opcodes
ostream& operator<< (ostream& os, const SWSIZE& swsize);
void sc_trace (sc_trace_file*& tf, const SWSIZE& swsize, std::string nm);

// SubWord repack opcodes
ostream& operator<< (ostream& os, const SWREPACK& swrepack);
void sc_trace (sc_trace_file*& tf, const SWREPACK& swrepack, std::string nm);

// Mask opcodes
ostream& operator<< (ostream& os, const MASKOP& mask);
void sc_trace (sc_trace_file*& tf, const MASKOP& mask, std::string nm);

// Tile shuffler mode opcodes
ostream& operator<< (ostream& os, const TS_MODE& mode);
void sc_trace (sc_trace_file*& tf, const TS_MODE& mode, std::string nm);

#endif /* SRC_SC_FUNCTIONS_H_ */
