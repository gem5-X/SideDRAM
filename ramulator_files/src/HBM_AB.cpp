#include "HBM_AB.h"
#include "DRAM.h"

#include <vector>
#include <functional>
#include <cassert>

using namespace std;
using namespace ramulator;

string HBM_AB::standard_name = "HBM_AB";
string HBM_AB::level_str [int(Level::MAX)] = {"Ch", "Ra", "Bg", "Ba", "Ro", "Co"};

map<string, enum HBM_AB::Org> HBM_AB::org_map = {
    {"HBM_1Gb", HBM_AB::Org::HBM_1Gb},
    {"HBM_2Gb", HBM_AB::Org::HBM_2Gb},
    {"HBM_4Gb", HBM_AB::Org::HBM_4Gb},
};

map<string, enum HBM_AB::Speed> HBM_AB::speed_map = {
    {"HBM_1Gbps", HBM_AB::Speed::HBM_1Gbps},
    {"HBM_300MHz", HBM_AB::Speed::HBM_300MHz},
    {"HBM_300MHz_row_if", HBM_AB::Speed::HBM_300MHz_row_if},
    {"HBM_300MHz_multiBG_if", HBM_AB::Speed::HBM_300MHz_multiBG_if},
};

HBM_AB::HBM_AB(Org org, Speed speed)
    : org_entry(org_table[int(org)]),
    speed_entry(speed_table[int(speed)]),
    read_latency(speed_entry.nCL + speed_entry.nBL)
{
    init_speed();
    init_prereq();
    init_rowhit(); // SAUGATA: added row hit function
    init_rowopen();
    init_lambda();
    init_timing();
}

HBM_AB::HBM_AB(const string& org_str, const string& speed_str) :
    HBM_AB(org_map[org_str], speed_map[speed_str])
{
}

void HBM_AB::set_channel_number(int channel) {
  org_entry.count[int(Level::Channel)] = channel;
}

void HBM_AB::set_rank_number(int rank) {
  org_entry.count[int(Level::Rank)] = rank;
}


void HBM_AB::init_speed()   // @NOTE Changed for 300 MHz
{
    const static int RFC_TABLE[int(Speed::MAX)][int(Org::MAX)] = {
        {55, 80, 130},
        {34, 49, 79},
        {34, 49, 79},
        {34, 49, 79}
    };
    const static int REFI1B_TABLE[int(Speed::MAX)][int(Org::MAX)] = {
        {64, 128, 256},
        {39, 77, 154},
        {39, 77, 154},
        {39, 77, 154}
    };
    const static int XS_TABLE[int(Speed::MAX)][int(Org::MAX)] = {
        {60, 85, 135},
        {37, 52, 82},
        {37, 52, 82},
        {37, 52, 82}
    };

    int speed = 0, density = 0;
    switch (speed_entry.rate) {
        case 1000: speed = 0; break;
        case 600: speed = 1; break;
        default: assert(false);
    };
    switch (org_entry.size >> 10){
        case 1: density = 0; break;
        case 2: density = 1; break;
        case 4: density = 2; break;
        default: assert(false);
    }
    speed_entry.nRFC = RFC_TABLE[speed][density];
    speed_entry.nREFI1B = REFI1B_TABLE[speed][density];
    speed_entry.nXS = XS_TABLE[speed][density];
}


void HBM_AB::init_prereq()  // @NOTE to access rows of rest of banks, we could keep the row id and run through the sibling banks and BGs
{                           // However, if we update correctly, this should not be needed
    // RD
    prereq[int(Level::Rank)][int(Command::RD)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::PowerUp): return Command::MAX;
            case int(State::ActPowerDown): return Command::PDX;
            case int(State::PrePowerDown): return Command::PDX;
            case int(State::SelfRefresh): return Command::SRX;
            default: assert(false);
        }};
    prereq[int(Level::Bank)][int(Command::RD)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::Closed): return Command::ACT;
            case int(State::Opened):
                if (node->row_state.find(id) != node->row_state.end())
                    return cmd;
                else return Command::PRE;
            default: assert(false);
        }};
    //     for (auto bg : node->parent->parent->children)  // Look for the open row in all banks at every BG
    //         for (auto bank: bg->children) {
    //             switch (int(bank->state)) {
    //                 case int(State::Closed): return Command::ACT;
    //                 case int(State::Opened):
    //                     if (bank->row_state.find(id) != bank->row_state.end())
    //                         continue;
    //                     else return Command::PRE;
    //                 default: assert(false);
    //             }
    //         }
    //     return cmd;
    // };

    // WR
    prereq[int(Level::Rank)][int(Command::WR)] = prereq[int(Level::Rank)][int(Command::RD)];
    prereq[int(Level::Bank)][int(Command::WR)] = prereq[int(Level::Bank)][int(Command::RD)];

    // REF
    prereq[int(Level::Rank)][int(Command::REF)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        for (auto bg : node->children)
            for (auto bank: bg->children) {
                if (bank->state == State::Closed)
                    continue;
                return Command::PREA;
            }
        return Command::REF;};

    // REFSB
    prereq[int(Level::Bank)][int(Command::REFSB)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        if (node->state == State::Closed) return Command::REFSB;
        return Command::PRE;};
    //     for (auto bg : node->parent->parent->children)  // We're in AllBanks mode, so works like REF
    //         for (auto bank: bg->children) {
    //             if (bank->state == State::Closed)
    //                 continue;
    //             return Command::PRE;
    //         }
    //     return Command::REFSB;        
    // };

    // PD
    prereq[int(Level::Rank)][int(Command::PDE)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::PowerUp): return Command::PDE;
            case int(State::ActPowerDown): return Command::PDE;
            case int(State::PrePowerDown): return Command::PDE;
            case int(State::SelfRefresh): return Command::SRX;
            default: assert(false);
        }};

    // SR
    prereq[int(Level::Rank)][int(Command::SRE)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::PowerUp): return Command::SRE;
            case int(State::ActPowerDown): return Command::PDX;
            case int(State::PrePowerDown): return Command::PDX;
            case int(State::SelfRefresh): return Command::SRE;
            default: assert(false);
        }};
}

// SAUGATA: added row hit check functions to see if the desired location is currently open
void HBM_AB::init_rowhit()  // @NOTE to access rows of rest of banks, we could keep the row id and run through the sibling banks and BGs
{                           // However, if we update correctly, this should not be needed
    // RD
    rowhit[int(Level::Bank)][int(Command::RD)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::Closed): return false;
            case int(State::Opened):
                if (node->row_state.find(id) != node->row_state.end())
                    return true;
                return false;
            default: assert(false);
        }};
    //     for (auto bg : node->parent->parent->children)  // Look for row hits at all banks at every BG
    //         for (auto bank: bg->children) {
    //             switch (int(bank->state)) {
    //                 case int(State::Closed): return false;
    //                 case int(State::Opened):
    //                     if (bank->row_state.find(id) != bank->row_state.end())
    //                         continue;
    //                     return false;
    //                 default: assert(false);
    //             }
    //         }
    //     return true;
    // };

    // WR
    rowhit[int(Level::Bank)][int(Command::WR)] = rowhit[int(Level::Bank)][int(Command::RD)];
}

void HBM_AB::init_rowopen() // @NOTE to access rows of rest of banks, we could keep the row id and run through the sibling banks and BGs
{                           // However, if we update correctly, this should not be needed
    // RD
    rowopen[int(Level::Bank)][int(Command::RD)] = [] (DRAM<HBM_AB>* node, Command cmd, int id) {
        switch (int(node->state)) {
            case int(State::Closed): return false;
            case int(State::Opened): return true;
            default: assert(false);
        }};
    //     for (auto bg : node->parent->parent->children)  // Look for open rows at all banks at every BG
    //         for (auto bank: bg->children) {
    //             switch (int(node->state)) {
    //                 case int(State::Closed): return false;
    //                 case int(State::Opened): continue;
    //                 default: assert(false);
    //             }
    //         }
    //     return true;
    // };

    // WR
    rowopen[int(Level::Bank)][int(Command::WR)] = rowopen[int(Level::Bank)][int(Command::RD)];
}

void HBM_AB::init_lambda()  // @NOTE to access rows of rest of banks, we keep the row id and run through the sibling banks and BGs
{
    lambda[int(Level::Bank)][int(Command::ACT)] = [] (DRAM<HBM_AB>* node, int id) {
        for (auto bg : node->parent->parent->children)  // Open row at all banks at every BG
            for (auto bank: bg->children) {
                bank->state = State::Opened;
                bank->row_state[id] = State::Opened;}};
    lambda[int(Level::Bank)][int(Command::PRE)] = [] (DRAM<HBM_AB>* node, int id) {
        for (auto bg : node->parent->parent->children)  // Close row at all banks at every BG
            for (auto bank: bg->children) {
                bank->state = State::Closed;
                bank->row_state.clear();}};
    lambda[int(Level::Rank)][int(Command::PREA)] = [] (DRAM<HBM_AB>* node, int id) {
        for (auto bg : node->children)
            for (auto bank : bg->children) {
                bank->state = State::Closed;
                bank->row_state.clear();
            }};
    lambda[int(Level::Rank)][int(Command::REF)] = [] (DRAM<HBM_AB>* node, int id) {};
    lambda[int(Level::Bank)][int(Command::RD)] = [] (DRAM<HBM_AB>* node, int id) {};
    lambda[int(Level::Bank)][int(Command::WR)] = [] (DRAM<HBM_AB>* node, int id) {};
    lambda[int(Level::Bank)][int(Command::RDA)] = [] (DRAM<HBM_AB>* node, int id) {
        for (auto bg : node->parent->parent->children)  // Close row at all banks at every BG
            for (auto bank: bg->children) {
                bank->state = State::Closed;
                bank->row_state.clear();}};
    lambda[int(Level::Bank)][int(Command::WRA)] = [] (DRAM<HBM_AB>* node, int id) {
        for (auto bg : node->parent->parent->children)  // Close row at all banks at every BG
            for (auto bank: bg->children) {
                bank->state = State::Closed;
                bank->row_state.clear();}};
    lambda[int(Level::Rank)][int(Command::PDE)] = [] (DRAM<HBM_AB>* node, int id) {
        for (auto bg : node->children)
            for (auto bank : bg->children) {
                if (bank->state == State::Closed)
                    continue;
                node->state = State::ActPowerDown;
                return;
            }
        node->state = State::PrePowerDown;};
    lambda[int(Level::Rank)][int(Command::PDX)] = [] (DRAM<HBM_AB>* node, int id) {
        node->state = State::PowerUp;};
    lambda[int(Level::Rank)][int(Command::SRE)] = [] (DRAM<HBM_AB>* node, int id) {
        node->state = State::SelfRefresh;};
    lambda[int(Level::Rank)][int(Command::SRX)] = [] (DRAM<HBM_AB>* node, int id) {
        node->state = State::PowerUp;};
}


void HBM_AB::init_timing()  // @NOTE all banks of all BGs in a rank are accessed, so timings of BGs and banks at the rank level
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

    // CAS <-> CAS  // In AllBanks mode, we never have nCCDS or nWTRS, only nCCDL and nWTRL
    t[int(Command::RD)].push_back({Command::RD, 1, s.nCCDL});
    t[int(Command::RD)].push_back({Command::RDA, 1, s.nCCDL});
    t[int(Command::RDA)].push_back({Command::RD, 1, s.nCCDL});
    t[int(Command::RDA)].push_back({Command::RDA, 1, s.nCCDL});
    t[int(Command::WR)].push_back({Command::WR, 1, s.nCCDL});
    t[int(Command::WR)].push_back({Command::WRA, 1, s.nCCDL});
    t[int(Command::WRA)].push_back({Command::WR, 1, s.nCCDL});
    t[int(Command::WRA)].push_back({Command::WRA, 1, s.nCCDL});
    t[int(Command::RD)].push_back({Command::WR, 1, s.nCL + s.nCCDL + 2 - s.nCWL});
    t[int(Command::RD)].push_back({Command::WRA, 1, s.nCL + s.nCCDL + 2 - s.nCWL});
    t[int(Command::RDA)].push_back({Command::WR, 1, s.nCL + s.nCCDL + 2 - s.nCWL});
    t[int(Command::RDA)].push_back({Command::WRA, 1, s.nCL + s.nCCDL + 2 - s.nCWL});
    t[int(Command::WR)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nWTRL});
    t[int(Command::WR)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nWTRL});
    t[int(Command::WRA)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nWTRL});
    t[int(Command::WRA)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nWTRL});

    t[int(Command::RD)].push_back({Command::PREA, 1, s.nRTP});
    t[int(Command::WR)].push_back({Command::PREA, 1, s.nCWL + s.nBL + s.nWR});

    // CAS <-> RAS  // Upgraded from Bank level to Rank level
    t[int(Command::ACT)].push_back({Command::RD, 1, s.nRCDR});
    t[int(Command::ACT)].push_back({Command::RDA, 1, s.nRCDR});
    t[int(Command::ACT)].push_back({Command::WR, 1, s.nRCDW});
    t[int(Command::ACT)].push_back({Command::WRA, 1, s.nRCDW});

    t[int(Command::RD)].push_back({Command::PRE, 1, s.nRTP});
    t[int(Command::WR)].push_back({Command::PRE, 1, s.nCWL + s.nBL + s.nWR});

    t[int(Command::RDA)].push_back({Command::ACT, 1, s.nRTP + s.nRP});
    t[int(Command::WRA)].push_back({Command::ACT, 1, s.nCWL + s.nBL + s.nWR + s.nRP});

    // CAS <-> PD
    t[int(Command::RD)].push_back({Command::PDE, 1, s.nCL + s.nBL + 1});
    t[int(Command::RDA)].push_back({Command::PDE, 1, s.nCL + s.nBL + 1});
    t[int(Command::WR)].push_back({Command::PDE, 1, s.nCWL + s.nBL + s.nWR});
    t[int(Command::WRA)].push_back({Command::PDE, 1, s.nCWL + s.nBL + s.nWR + 1}); // +1 for pre
    t[int(Command::PDX)].push_back({Command::RD, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::RDA, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::WR, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::WRA, 1, s.nXP});

    // CAS <-> SR: none (all banks have to be precharged)

    // RAS <-> RAS  // In AllBanks mode, we never have nRRDS, only nRRDL
                    // Some upgrades from Bank level to Rank level
    t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRRDL});
    t[int(Command::ACT)].push_back({Command::ACT, 4, s.nFAW});
    t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRC});
    t[int(Command::ACT)].push_back({Command::PREA, 1, s.nRAS});
    t[int(Command::ACT)].push_back({Command::PRE, 1, s.nRAS});
    t[int(Command::PREA)].push_back({Command::ACT, 1, s.nRP});
    t[int(Command::PRE)].push_back({Command::ACT, 1, s.nRP});

    // RAS <-> REF
    t[int(Command::PRE)].push_back({Command::REF, 1, s.nRP});
    t[int(Command::PREA)].push_back({Command::REF, 1, s.nRP});
    t[int(Command::REF)].push_back({Command::ACT, 1, s.nRFC});

    // RAS <-> PD
    t[int(Command::ACT)].push_back({Command::PDE, 1, 1});
    t[int(Command::PDX)].push_back({Command::ACT, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::PRE, 1, s.nXP});
    t[int(Command::PDX)].push_back({Command::PREA, 1, s.nXP});

    // RAS <-> SR
    t[int(Command::PRE)].push_back({Command::SRE, 1, s.nRP});
    t[int(Command::PREA)].push_back({Command::SRE, 1, s.nRP});
    t[int(Command::SRX)].push_back({Command::ACT, 1, s.nXS});

    // REF <-> REF
    t[int(Command::REF)].push_back({Command::REF, 1, s.nRFC});

    // REF <-> PD
    t[int(Command::REF)].push_back({Command::PDE, 1, 1});
    t[int(Command::PDX)].push_back({Command::REF, 1, s.nXP});

    // REF <-> SR
    t[int(Command::SRX)].push_back({Command::REF, 1, s.nXS});

    // REFSB    // Upgraded from Bank level to Rank level
    t[int(Command::PRE)].push_back({Command::REFSB, 1, s.nRP});
    t[int(Command::REFSB)].push_back({Command::REFSB, 1, s.nRFC});
    t[int(Command::REFSB)].push_back({Command::ACT, 1, s.nRFC});

    // PD <-> PD
    t[int(Command::PDE)].push_back({Command::PDX, 1, s.nPD});
    t[int(Command::PDX)].push_back({Command::PDE, 1, s.nXP});

    // PD <-> SR
    t[int(Command::PDX)].push_back({Command::SRE, 1, s.nXP});
    t[int(Command::SRX)].push_back({Command::PDE, 1, s.nXS});

    // SR <-> SR
    t[int(Command::SRE)].push_back({Command::SRX, 1, s.nCKESR});
    t[int(Command::SRX)].push_back({Command::SRE, 1, s.nXS});

    // /*** Bank Group ***/
    // t = timing[int(Level::BankGroup)];
    // // CAS <-> CAS
    // t[int(Command::RD)].push_back({Command::RD, 1, s.nCCDL});
    // t[int(Command::RD)].push_back({Command::RDA, 1, s.nCCDL});
    // t[int(Command::RDA)].push_back({Command::RD, 1, s.nCCDL});
    // t[int(Command::RDA)].push_back({Command::RDA, 1, s.nCCDL});
    // t[int(Command::WR)].push_back({Command::WR, 1, s.nCCDL});
    // t[int(Command::WR)].push_back({Command::WRA, 1, s.nCCDL});
    // t[int(Command::WRA)].push_back({Command::WR, 1, s.nCCDL});
    // t[int(Command::WRA)].push_back({Command::WRA, 1, s.nCCDL});
    // t[int(Command::WR)].push_back({Command::WR, 1, s.nCCDL});
    // t[int(Command::WR)].push_back({Command::WRA, 1, s.nCCDL});
    // t[int(Command::WRA)].push_back({Command::WR, 1, s.nCCDL});
    // t[int(Command::WRA)].push_back({Command::WRA, 1, s.nCCDL});
    // t[int(Command::WR)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nWTRL});
    // t[int(Command::WR)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nWTRL});
    // t[int(Command::WRA)].push_back({Command::RD, 1, s.nCWL + s.nBL + s.nWTRL});
    // t[int(Command::WRA)].push_back({Command::RDA, 1, s.nCWL + s.nBL + s.nWTRL});

    // // RAS <-> RAS
    // t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRRDL});

    // /*** Bank ***/
    // t = timing[int(Level::Bank)];

    // // CAS <-> RAS
    // t[int(Command::ACT)].push_back({Command::RD, 1, s.nRCDR});
    // t[int(Command::ACT)].push_back({Command::RDA, 1, s.nRCDR});
    // t[int(Command::ACT)].push_back({Command::WR, 1, s.nRCDW});
    // t[int(Command::ACT)].push_back({Command::WRA, 1, s.nRCDW});

    // t[int(Command::RD)].push_back({Command::PRE, 1, s.nRTP});
    // t[int(Command::WR)].push_back({Command::PRE, 1, s.nCWL + s.nBL + s.nWR});

    // t[int(Command::RDA)].push_back({Command::ACT, 1, s.nRTP + s.nRP});
    // t[int(Command::WRA)].push_back({Command::ACT, 1, s.nCWL + s.nBL + s.nWR + s.nRP});

    // // RAS <-> RAS
    // t[int(Command::ACT)].push_back({Command::ACT, 1, s.nRC});
    // t[int(Command::ACT)].push_back({Command::PRE, 1, s.nRAS});
    // t[int(Command::PRE)].push_back({Command::ACT, 1, s.nRP});

    // // REFSB
    // t[int(Command::PRE)].push_back({Command::REFSB, 1, s.nRP});
    // t[int(Command::REFSB)].push_back({Command::REFSB, 1, s.nRFC});
    // t[int(Command::REFSB)].push_back({Command::ACT, 1, s.nRFC});
}
