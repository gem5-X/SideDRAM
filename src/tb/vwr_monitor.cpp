#include "vwr_monitor.h"

void vwr_monitor::monitor_thread() {
    while(1) {
        wait(1, SC_NS);

        cout << "At time " << sc_time_stamp() << endl;
        cout << "wr_nrd " << wr_nrd << " d_nm " << d_nm << " idx " << idx << endl;
        cout << "Large word: " << lbus << endl;
        cout << "Small word: " << sbus << endl << endl;

        wait();
    }
}
