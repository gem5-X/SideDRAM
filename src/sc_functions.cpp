/*
 * sc_functions.cpp
 *
 *  Created on: Jan 16, 2024
 *      Author: rafa
 */

#include "sc_functions.h"

ostream& operator<< (ostream& os, const itt_field& field) {
    os << "valid=" << field.valid << " addr=" << field.addr << " stop=" << field.stop;
    return os;
}

void sc_trace (sc_trace_file*& tf, const itt_field& field, std::string nm) {
    sc_trace(tf, field.valid, nm + ".valid");
    sc_trace(tf, field.addr, nm + ".addr");
    sc_trace(tf, field.stop, nm + ".stop");
}

ostream& operator<< (ostream& os, const ADDOP& addop) {
    os << uint(addop);
    return os;
}

void sc_trace (sc_trace_file*& tf, const ADDOP& addop, std::string nm) {
    sc_trace(tf, uint(addop), nm);
}

ostream& operator<< (ostream& os, const OPC_STORAGE& store) {
    os << uint(store);
    return os;
}

void sc_trace (sc_trace_file*& tf, const OPC_STORAGE& store, std::string nm) {
    sc_trace(tf, uint(store), nm);
}

ostream& operator<< (ostream& os, const MUX& mux) {
    os << uint(mux);
    return os;
}

void sc_trace (sc_trace_file*& tf, const MUX& mux, std::string nm) {
    sc_trace(tf, uint(mux), nm);
}

ostream& operator<< (ostream& os, const RF_SEL& sel) {
    os << uint(sel);
    return os;
}

void sc_trace (sc_trace_file*& tf, const RF_SEL& sel, std::string nm) {
    sc_trace(tf, uint(sel), nm);
}

ostream& operator<< (ostream& os, const SWSIZE& swsize) {
    os << uint(swsize);
    return os;
}

void sc_trace (sc_trace_file*& tf, const SWSIZE& swsize, std::string nm) {
    sc_trace(tf, uint(swsize), nm);
}

ostream& operator<< (ostream& os, const SWREPACK& swrepack) {
    os << uint(swrepack);
    return os;
}

void sc_trace (sc_trace_file*& tf, const SWREPACK& swrepack, std::string nm) {
    sc_trace(tf, uint(swrepack), nm);
}

ostream& operator<< (ostream& os, const MASKOP& mask) {
    os << uint(mask);
    return os;
}

void sc_trace (sc_trace_file*& tf, const MASKOP& mask, std::string nm) {
    sc_trace(tf, uint(mask), nm);
}

ostream& operator<< (ostream& os, const TS_MODE& mode) {
    os << uint(mode);
    return os;
}

void sc_trace (sc_trace_file*& tf, const TS_MODE& mode, std::string nm) {
    sc_trace(tf, uint(mode), nm);
}

