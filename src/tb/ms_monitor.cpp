#include "ms_monitor.h"

void ms_monitor::monitor_thread() {
    while(1) {
        wait(1, SC_NS);

        cout << "At time " << sc_time_stamp() << endl;
        cout << "enable " << enable << " dst " << dst << " csd_in " << csd_in[0] << " csd_len " << uint(csd_len) << endl;
        cout << "sa_valid: " << sa_valid << " sa_index " << sa_index << " sa_src0 " << sa_src0 << " sa_dst " << sa_dst << endl;

        wait();
    }
}

void ms_monitor::parser_method() {
    dst_uint->write(uint(dst->read()));
    sa_src0_uint->write(uint(sa_src0->read()));
    sa_dst_uint->write(uint(sa_dst->read()));
}
