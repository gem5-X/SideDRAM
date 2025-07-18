#include "softs_monitor.h"

void softs_monitor::monitor_thread() {
    while(1) {
        wait(1, SC_NS);

        cout << endl << "At time " << sc_time_stamp() << endl;

        cout << "SHIFT & ADD" << endl;
        cout << "Enable " << SA_en << " size " << SA_size << " shift " << SA_shift << " s_na " << SA_s_na << endl;
        cout << showbase << hex << "op1: " << SA_op1[0] << " op2: " << SA_op2[0] << " out: " << SA_out[0] << dec << endl;

        cout << "PACK & MASK" << endl;
        cout << "Enable " << PM_en << " in_size " << PM_in_size << " out_size " << PM_out_size << " in_start " << PM_in_start;
        cout << " op_sel " << MASK_SEL_STRING.at(MASKOP(PM_op_sel->read())) << " shift " << PM_shift << endl;
        cout << hex << "w1: " << PM_w1[0] << " w2: " << PM_w2[0] << " mask: " << PM_mask_in[0] << " out: " << PM_out[0] << dec << endl;

        wait();
    }
}
