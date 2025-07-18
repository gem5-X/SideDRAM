#include <cstdio>
#include <cstdlib>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <string>

#include "half.hpp"
#include "../../src/defs.h"

using namespace std;

#define HALF_MASK ((1 << 16) - 1)

int main(int argc, const char *argv[])
{
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <results-file>" << endl;
        return 0;
    }

    string ri = argv[1];    // Input results file name
    string riline;
    string cycle, address;
    uint64_t dataAux;
    half_float::half half_aux;

    // Open input file
    ifstream results;
    results.open(ri);

    while (getline(results, riline)) {

        istringstream riiss(riline);

        riiss >> cycle >> address;
        cout << cycle << " " << address << "\t";
        while (riiss >> showbase >> hex >> dataAux) {
            for (int i=0; i<4; i++) {
                half_aux = half_float::half(half_float::detail::binary, dataAux & HALF_MASK);
                cout << half_aux << " ";
                dataAux = dataAux >> 16;
            }
            cout << "\t";
        }

        cout << endl;
    }
}