#include "LPDDR4_AB.h"
#include "DRAM.h"

#include <vector>
#include <functional>
#include <cassert>

using namespace std;
using namespace ramulator;

string LPDDR4_AB::standard_name = "LPDDR4_AB";
string LPDDR4_AB::level_str [int(Level::MAX)] = {"Ch", "Ra", "Ba", "Ro", "Co"};

map<string, enum LPDDR4_AB::Org> LPDDR4_AB::org_map = {
    {"LPDDR4_4Gb_x16", LPDDR4_AB::Org::LPDDR4_4Gb_x16},
    {"LPDDR4_6Gb_x16", LPDDR4_AB::Org::LPDDR4_6Gb_x16},
    {"LPDDR4_8Gb_x16", LPDDR4_AB::Org::LPDDR4_8Gb_x16},
};

map<string, enum LPDDR4_AB::Speed> LPDDR4_AB::speed_map = {
    {"LPDDR4_1600", LPDDR4_AB::Speed::LPDDR4_1600},
    {"LPDDR4_2400", LPDDR4_AB::Speed::LPDDR4_2400},
    {"LPDDR4_3200", LPDDR4_AB::Speed::LPDDR4_3200},
    {"LPDDR4_200MHz", LPDDR4_AB::Speed::LPDDR4_200MHz},
};

LPDDR4_AB::LPDDR4_AB(Org org, Speed speed)
    : org_entry(org_table[int(org)]),
    speed_entry(speed_table[int(speed)]),
    read_latency(speed_entry.nCL + speed_entry.nDQSCK + speed_entry.nBL)
{
    init_speed();
    init_prereq();
    init_rowhit(); // SAUGATA: added row hit function
    init_rowopen();
    init_lambda();
    init_timing();
}

LPDDR4_AB::LPDDR4_AB(const string& org_str, const string& speed_str) :
    LPDDR4_AB(org_map[org_str], speed_map[speed_str])
{
}

void LPDDR4_AB::set_channel_number(int channel) {
  org_entry.count[int(Level::Channel)] = channel;
}

void LPDDR4_AB::set_rank_number(int rank) {
  org_entry.count[int(Level::Rank)] = rank;
}


void LPDDR4_AB::init_speed()
{
    // 12Gb/16Gb RFCab/RFCpb TBD
    // Numbers are in DRAM cycles
    const static int RFCPB_TABLE[int(Org::MAX)][int(Speed::MAX)] = {
        {48,  72,  96, 24},
        {72, 108, 144, 36},
        {72, 108, 144, 36}
    };

    const static int RFCAB_TABLE[int(Org::MAX)][int(Speed::MAX)] = {
        {104, 156, 208, 52},
        {144, 216, 288, 52},
        {144, 216, 288, 52}
    };

    const static int REFI_TABLE[int(RefreshMode::MAX)][int(Speed::MAX)] = {
        {3124, 4685, 6247, 1562},
        {1563, 2344, 3125, 782},
        { 782, 1172, 1563, 391}
    };

    const static int XSR_TABLE[int(Org::MAX)][int(Speed::MAX)] = {
        {110, 165, 220, 55},
        {150, 225, 300, 75},
        {150, 225, 300, 75},
    };

    int speed = 0, density = 0;
    switch (speed_entry.rate) {
        case 1600: speed = 0; break;
        case 2400: speed = 1; break;
        case 3200: speed = (speed_entry.freq==1600) ? 2 : 3; break;
        default: assert(false);
    };
    switch (org_entry.size >> 10){
        case 2: density = 0; break;
        case 3: density = 1; break;
        case 4: density = 2; break;
        default: assert(false && "12Gb/16Gb is still TBD");
    }
    speed_entry.nRFCpb = RFCPB_TABLE[density][speed];
    speed_entry.nRFCab = RFCAB_TABLE[density][speed];
    speed_entry.nREFI = REFI_TABLE[int(refresh_mode)][speed];
    speed_entry.nXSR = XSR_TABLE[density][speed];
}


void LPDDR4_AB::init_prereq()
{
    // RD
    prereq[int(Level::Rank)][int(Command::RD)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::PowerUp): return Command::MAX;
            case int(State::ActPowerDown): return Command::PDX;
            case int(State::PrePowerDown): return Command::PDX;
            case int(State::SelfRefresh): return Command::SREFX;
            default: assert(false);
        }};
    prereq[int(Level::Bank)][int(Command::RD)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::Closed): return Command::ACT;
            case int(State::Opened):
                if (node->row_state.find(id) != node->row_state.end())
                    return cmd;
                return Command::PRE;
            default: assert(false);
        }};

    // WR
    prereq[int(Level::Rank)][int(Command::WR)] = prereq[int(Level::Rank)][int(Command::RD)];
    prereq[int(Level::Bank)][int(Command::WR)] = prereq[int(Level::Bank)][int(Command::RD)];

    // REF
    prereq[int(Level::Rank)][int(Command::REF)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        for (auto bank : node->children) {
            if (bank->state == State::Closed)
                continue;
            return Command::PREA;
        }
        return Command::REF;};

    // PD
    prereq[int(Level::Rank)][int(Command::PDE)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::PowerUp): return Command::PDE;
            case int(State::ActPowerDown): return Command::PDE;
            case int(State::PrePowerDown): return Command::PDE;
            case int(State::SelfRefresh): return Command::SREFX;
            default: assert(false);
        }};

    // SR
    prereq[int(Level::Rank)][int(Command::SREF)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::PowerUp): return Command::SREF;
            case int(State::ActPowerDown): return Command::PDX;
            case int(State::PrePowerDown): return Command::PDX;
            case int(State::SelfRefresh): return Command::SREF;
            default: assert(false);
        }};
}

// SAUGATA: added row hit check functions to see if the desired location is currently open
void LPDDR4_AB::init_rowhit()
{
    // RD
    rowhit[int(Level::Bank)][int(Command::RD)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::Closed): return false;
            case int(State::Opened):
                if (node->row_state.find(id) != node->row_state.end())
                    return true;
                return false;
            default: assert(false);
        }};

    // WR
    rowhit[int(Level::Bank)][int(Command::WR)] = rowhit[int(Level::Bank)][int(Command::RD)];
}

void LPDDR4_AB::init_rowopen()
{
    // RD
    rowopen[int(Level::Bank)][int(Command::RD)] = [] (DRAM<LPDDR4_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::Closed): return false;
            case int(State::Opened): return true;
            default: assert(false);
        }};

    // WR
    rowopen[int(Level::Bank)][int(Command::WR)] = rowopen[int(Level::Bank)][int(Command::RD)];
}

void LPDDR4_AB::init_lambda()
{
    lambda[int(Level::Bank)][int(Command::ACT)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        for (auto bank : node->parent->children) {  // Open row at all banks 
            bank->state = State::Opened;
            bank->row_state[id] = State::Opened;}};
    lambda[int(Level::Bank)][int(Command::PRE)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        for (auto bank : node->parent->children) {  // Open row at all banks 
            bank->state = State::Closed;
            bank->row_state.clear();}};
    lambda[int(Level::Rank)][int(Command::PREA)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        for (auto bank : node->children) {
            bank->state = State::Closed;
            bank->row_state.clear();}};
    lambda[int(Level::Rank)][int(Command::REF)] = [] (DRAM<LPDDR4_AB>* node, int id) {};
    lambda[int(Level::Bank)][int(Command::RD)] = [] (DRAM<LPDDR4_AB>* node, int id) {};
    lambda[int(Level::Bank)][int(Command::WR)] = [] (DRAM<LPDDR4_AB>* node, int id) {};
    lambda[int(Level::Bank)][int(Command::RDA)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        for (auto bank : node->parent->children) {  // Open row at all banks 
            bank->state = State::Closed;
            bank->row_state.clear();}};
    lambda[int(Level::Bank)][int(Command::WRA)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        for (auto bank : node->parent->children) {  // Open row at all banks 
            bank->state = State::Closed;
            bank->row_state.clear();}};
    lambda[int(Level::Rank)][int(Command::PDE)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        for (auto bank : node->children) {
            if (bank->state == State::Closed)
                continue;
            node->state = State::ActPowerDown;
            return;
        }
        node->state = State::PrePowerDown;};
    lambda[int(Level::Rank)][int(Command::PDX)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        node->state = State::PowerUp;};
    lambda[int(Level::Rank)][int(Command::SREF)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        node->state = State::SelfRefresh;};
    lambda[int(Level::Rank)][int(Command::SREFX)] = [] (DRAM<LPDDR4_AB>* node, int id) {
        node->state = State::PowerUp;};
}


void LPDDR4_AB::init_timing()
{
    SpeedEntry& s = speed_entry;
    vector<TimingEntry> *t;

    /*** Channel ***/
    t = timing[int(Level::Channel)];

    // CAS <-> CAS
    t[int(Command::RD)].push_back({Command::RD, 1, s.nBL});
    t[int(Command::RD)].push_back({Command::RDA, 1, s.nBL});
    t[int(Command::RDA)].push_back({Command::RD, 1, s.nBL});
    t[int(Command::RDA)].push_back({Command::RDA, 1, s.nBL});
    t[int(Command::WR)].push_back({Command::WR, 1, s.nBL});
    t[int(Command::WR)].push_back({Command::WRA, 1, s.nBL});
    t[int(Command::WRA)].push_back({Command::WR, 1, s.nBL});
    t[int(Command::WRA)].push_back({Command::WRA, 1, s.nBL});


    /*** Rank ***/
    t = timing[int(Level::Rank)];

    // CAS <-> CAS
    t[int(Command::RD)].push_back({Command::RD, 1, s.nCCD});
    t[int(Command::RD)].push_back({Command::RDA, 1, s.nCCD});
    t[int(Command::RDA)].push_back({Command::RD, 1, s.nCCD});
    t[int(Command::RDA)].push_back({Command::RDA, 1, s.nCCD});
    t[int(Command::WR)].push_back({Command::WR, 1, s.nCCD});
    t[int(Command::WR)].push_back({Command::WRA, 1, s.nCCD});
    t[int(Command::WRA)].push_back({Command::WR, 1, s.nCCD});
    t[int(Command::WRA)].push_back({Command::WRA, 1, s.nCCD});

    t[int(Command::RD)].push_back({Command::WR, 1, s.nCL + s.nBL + s.nDQSCK + 1 - s.nCWL});
    t[int(Command::RD)].push_back({Command::WRA, 1, s.nCL + s.nBL + s.nDQSCK + 1 - s.nCWL});
    t[int(Command::RDA)].push_back({Command::WR, 1, s.nCL + s.nBL + s.nDQSCK + 1 - s.nCWL});
    t[int(Command::RDA)].push_back({Command::WRA, 1, s.nCL + s.nBL + s.nDQSCK + 1 - s.nCWL});

    t[int(Command::WR)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nWTR + 1});
    t[int(Command::WR)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nWTR + 1});
    t[int(Command::WRA)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nWTR + 1});
    t[int(Command::WRA)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nWTR + 1});


    // CAS <-> CAS (between sibling ranks)
    t[int(Command::RD)].push_back({Command::RD, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RD)].push_back({Command::RDA, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RDA)].push_back({Command::RD, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RDA)].push_back({Command::RDA, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RD)].push_back({Command::WR, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RD)].push_back({Command::WRA, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RDA)].push_back({Command::WR, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RDA)].push_back({Command::WRA, 1, s.nBL + s.nRTRS, true});
    t[int(Command::RD)].push_back({Command::WR, 1, s.nCL + s.nBL + s.nDQSCK + 1 + s.nRTRS - s.nCWL, true});
    t[int(Command::RD)].push_back({Command::WRA, 1, s.nCL + s.nBL + s.nDQSCK + 1 + s.nRTRS - s.nCWL, true});
    t[int(Command::RDA)].push_back({Command::WR, 1, s.nCL + s.nBL + s.nDQSCK + 1 + s.nRTRS - s.nCWL, true});
    t[int(Command::RDA)].push_back({Command::WRA, 1, s.nCL + s.nBL + s.nDQSCK + 1 + s.nRTRS - s.nCWL, true});
    t[int(Command::WR)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nRTRS - s.nCL, true});
    t[int(Command::WR)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nRTRS - s.nCL, true});
    t[int(Command::WRA)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nRTRS - s.nCL, true});
    t[int(Command::WRA)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nRTRS - s.nCL, true});

    // CAS <-> RAS (Upgraded from Bank level)
    t[int(Command::ACT)].push_back({Command::RD, 1, s.nRCD});
    t[int(Command::ACT)].push_back({Command::RDA, 1, s.nRCD});
    t[int(Command::ACT)].push_back({Command::WR, 1, s.nRCD});
    t[int(Command::ACT)].push_back({Command::WRA, 1, s.nRCD});

    t[int(Command::RD)].push_back({Command::PRE, 1, s.nRTP});
    t[int(Command::WR)].push_back({Command::PRE, 1, s.nCWL + s.nBL + s.nWR});

    t[int(Command::RDA)].push_back({Command::ACT, 1, s.nRTP + s.nRPpb});
    t[int(Command::WRA)].push_back({Command::ACT, 1, s.nCWL + s.nBL + s.nWR + s.nRPpb});

    // CAS <-> PREA
    t[int(Command::RD)].push_back({Command::PREA, 1, s.nRTP});
    t[int(Command::WR)].push_back({Command::PREA, 1, s.nCWL + s.nBL + s.nWR});

    // CAS <-> PD
    t[int(Command::RD)].push_back({Command::PDE, 1, s.nCL + s.nBL + 1});
    t[int(Command::RDA)].push_back({Command::PDE, 1, s.nCL + s.nBL + 1});
    t[int(Command::WR)].push_back({Command::PDE, 1, s.nCWL + s.nBL + s.nWR});
    t[int(Command::WRA)].push_back({Command::PDE, 1, s.nCWL + s.nBL + s.nWR + 1}); // +1 for pre
    t[int(Command::PDX)].push_back({Command::RD, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::RDA, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::WR, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::WRA, 1, s.nXP});

    // RAS <-> RAS (Some upgrades from Bank level)
    t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRRD});
    t[int(Command::ACT)].push_back({Command::ACT, 4, s.nFAW});
    t[int(Command::ACT)].push_back({Command::PREA, 1, s.nRAS});
    t[int(Command::PREA)].push_back({Command::ACT, 1, s.nRPab});
    t[int(Command::PRE)].push_back({Command::PRE, 1, s.nPPD});

    t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRC});
    t[int(Command::ACT)].push_back({Command::PRE, 1, s.nRAS});
    t[int(Command::PRE)].push_back({Command::ACT, 1, s.nRPpb});
    t[int(Command::PRE)].push_back({Command::REFPB, 1, s.nRPpb});
    // between different banks
    t[int(Command::ACT)].push_back({Command::REFPB, 1, s.nRRD, true});
    t[int(Command::REFPB)].push_back({Command::ACT, 1, s.nRRD, true});

    // RAS <-> REF
    t[int(Command::PRE)].push_back({Command::REF, 1, s.nRPpb});
    t[int(Command::PREA)].push_back({Command::REF, 1, s.nRPab});
    t[int(Command::REF)].push_back({Command::ACT, 1, s.nRFCab});

    // REFPB (Upgraded from Bank level)
    t[int(Command::REFPB)].push_back({Command::REFPB, 1, s.nRFCpb});
    t[int(Command::REFPB)].push_back({Command::ACT, 1, s.nRFCpb});

    // RAS <-> PD
    t[int(Command::ACT)].push_back({Command::PDE, 1, 1});
    t[int(Command::PDX)].push_back({Command::ACT, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::PRE, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::PREA, 1, s.nXP});

    // RAS <-> SR
    t[int(Command::PRE)].push_back({Command::SREF, 1, s.nRPpb});
    t[int(Command::PREA)].push_back({Command::SREF, 1, s.nRPab});
    t[int(Command::SREFX)].push_back({Command::ACT, 1, s.nXSR});

    // REF <-> REF
    t[int(Command::REF)].push_back({Command::REF, 1, s.nRFCab});
    t[int(Command::REF)].push_back({Command::REFPB, 1, s.nRFCab});
    t[int(Command::REFPB)].push_back({Command::REF, 1, s.nRFCpb});

    // REF <-> PD
    t[int(Command::REF)].push_back({Command::PDE, 1, 1});
    t[int(Command::REFPB)].push_back({Command::PDE, 1, 1});
    t[int(Command::PDX)].push_back({Command::REF, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::REFPB, 1, s.nXP});

    // REF <-> SR
    t[int(Command::SREFX)].push_back({Command::REF, 1, s.nXSR});
    t[int(Command::SREFX)].push_back({Command::REFPB, 1, s.nXSR});

    // PD <-> PD
    t[int(Command::PDE)].push_back({Command::PDX, 1, s.nCKE});
    t[int(Command::PDX)].push_back({Command::PDE, 1, s.nXP});

    // PD <-> SR
    t[int(Command::PDX)].push_back({Command::SREF, 1, s.nXP});
    t[int(Command::SREFX)].push_back({Command::PDE, 1, s.nXSR});

    // SR <-> SR
    t[int(Command::SREF)].push_back({Command::SREFX, 1, s.nSR});
    t[int(Command::SREFX)].push_back({Command::SREF, 1, s.nXSR});

    // /*** Bank ***/
    // t = timing[int(Level::Bank)];

    // // CAS <-> RAS
    // t[int(Command::ACT)].push_back({Command::RD, 1, s.nRCD});
    // t[int(Command::ACT)].push_back({Command::RDA, 1, s.nRCD});
    // t[int(Command::ACT)].push_back({Command::WR, 1, s.nRCD});
    // t[int(Command::ACT)].push_back({Command::WRA, 1, s.nRCD});

    // t[int(Command::RD)].push_back({Command::PRE, 1, s.nRTP});
    // t[int(Command::WR)].push_back({Command::PRE, 1, s.nCWL + s.nBL + s.nWR});

    // t[int(Command::RDA)].push_back({Command::ACT, 1, s.nRTP + s.nRPpb});
    // t[int(Command::WRA)].push_back({Command::ACT, 1, s.nCWL + s.nBL + s.nWR + s.nRPpb});

    // // RAS <-> RAS
    // t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRC});
    // t[int(Command::ACT)].push_back({Command::PRE, 1, s.nRAS});
    // t[int(Command::PRE)].push_back({Command::ACT, 1, s.nRPpb});
    // t[int(Command::PRE)].push_back({Command::REFPB, 1, s.nRPpb});

    // // between different banks
    // t[int(Command::ACT)].push_back({Command::REFPB, 1, s.nRRD, true});
    // t[int(Command::REFPB)].push_back({Command::ACT, 1, s.nRRD, true});

    // // REFPB
    // t[int(Command::REFPB)].push_back({Command::REFPB, 1, s.nRFCpb});
    // t[int(Command::REFPB)].push_back({Command::ACT, 1, s.nRFCpb});
}
