#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <random>

using namespace std;

// Format of raw traces: Address    R/W     Data

int main(int argc, const char *argv[])
{   
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <input-file> <output-file>" << endl;
        return 0;
    }

    string fi = argv[1];    // Input file name
    string fo = argv[2];    // Output file name
    string line;

    // Open input and output files
    ifstream input;
    ofstream output;
    input.open(fi);
    output.open(fo);

    output << showbase << internal << setfill ('0') << hex;

    // Run through the sequence of instructions and extract commands for ramulator
    while (getline(input, line)) {
        istringstream iss(line);
        unsigned long int addr;
        string cmd;

        if (!(iss >> hex >> addr >> cmd)) { break; }   // Error

        output << showbase << internal << setfill ('0') << hex << addr << "\t" << cmd << endl;
    }

    
    input.close();
    output.close();
    return 0;
}