#ifndef __HBM_AB_H
#define __HBM_AB_H

#include "DRAM.h"
#include "Request.h"
#include <vector>
#include <functional>

using namespace std;

namespace ramulator
{

class HBM_AB
{
public:
    static string standard_name;
    enum class Org;
    enum class Speed;
    HBM_AB(Org org, Speed speed);
    HBM_AB(const string& org_str, const string& speed_str);

    static map<string, enum Org> org_map;
    static map<string, enum Speed> speed_map;

    /* Level */
    enum class Level : int  // @NOTE we keep same levels as in normal HBM
    {
        Channel, Rank, BankGroup, Bank, Row, Column, MAX
    };
    
    static std::string level_str [int(Level::MAX)];

    /* Command */
    enum class Command : int
    {
        ACT, PRE,   PREA,
        RD,  WR,    RDA, WRA,
        REF, REFSB, PDE, PDX,  SRE, SRX,
        MAX
    };

    // REFSB and REF is not compatible, choose one or the other.
    // REFSB can be issued to banks in any order, as long as REFI1B
    // is satisfied for all banks

    string command_name[int(Command::MAX)] = {
        "ACT", "PRE",   "PREA",
        "RD",  "WR",    "RDA",  "WRA",
        "REF", "REFSB", "PDE",  "PDX",  "SRE", "SRX"
    };

    Level scope[int(Command::MAX)] = {
        Level::Row,    Level::Bank,   Level::Rank,
        Level::Column, Level::Column, Level::Column, Level::Column,
        Level::Rank,   Level::Bank,   Level::Rank,   Level::Rank,   Level::Rank,   Level::Rank
    };
    
    bool is_opening(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::ACT):
                return true;
            default:
                return false;
        }
    }

    bool is_accessing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::RD):
            case int(Command::WR):
            case int(Command::RDA):
            case int(Command::WRA):
                return true;
            default:
                return false;
        }
    }

    bool is_closing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::RDA):
            case int(Command::WRA):
            case int(Command::PRE):
            case int(Command::PREA):
                return true;
            default:
                return false;
        }
    }

    bool is_refreshing(Command cmd)
    {
        switch(int(cmd)) {
            case int(Command::REF):
            case int(Command::REFSB):
                return true;
            default:
                return false;
        }
    }

    /* State */
    enum class State : int
    {
        Opened, Closed, PowerUp, ActPowerDown, PrePowerDown, SelfRefresh, MAX
    } start[int(Level::MAX)] = {
        State::MAX, State::PowerUp, State::MAX, State::Closed, State::Closed, State::MAX
    };

    /* Translate */
    Command translate[int(Request::Type::MAX)] = {  // @NOTE this is for checking if it was the target op
        Command::RD,  Command::WR,
        Command::REF, Command::PDE, Command::SRE
    };

    /* Prereq */
    function<Command(DRAM<HBM_AB>*, Command cmd, int)> prereq[int(Level::MAX)][int(Command::MAX)];

    // SAUGATA: added function object container for row hit status
    /* Row hit */
    function<bool(DRAM<HBM_AB>*, Command cmd, int)> rowhit[int(Level::MAX)][int(Command::MAX)];
    function<bool(DRAM<HBM_AB>*, Command cmd, int)> rowopen[int(Level::MAX)][int(Command::MAX)];

    /* Timing */
    struct TimingEntry
    {
        Command cmd;
        int dist;
        int val;
        bool sibling;
    };
    vector<TimingEntry> timing[int(Level::MAX)][int(Command::MAX)];

    /* Lambda */
    function<void(DRAM<HBM_AB>*, int)> lambda[int(Level::MAX)][int(Command::MAX)];

    /* Organization */
    enum class Org : int
    { // per channel density here. Each stack comes with 8 channels
        HBM_1Gb,
        HBM_2Gb,
        HBM_4Gb,
        MAX
    };

    struct OrgEntry {   // @TODO do we change this for matching organization/size?
        int size;
        int dq;
        int count[int(Level::MAX)];
    } org_table[int(Org::MAX)] = {  // @NOTE This organizations start at the BG
        {1<<10, 128, {0, 0, 4, 2, 1<<13, 1<<(6+1)}},
        {2<<10, 128, {0, 0, 4, 2, 1<<14, 1<<(6+1)}},
        {4<<10, 128, {0, 0, 4, 4, 1<<14, 1<<(5+2)}}, // 2 instead of 1 to offset the prefetch
    }, org_entry;

    void set_channel_number(int channel);
    void set_rank_number(int rank);

    /* Speed */
    enum class Speed : int
    {
        HBM_1Gbps,
        HBM_300MHz,
        HBM_300MHz_row_if,
        HBM_300MHz_multiBG_if,
        MAX
    };

    int prefetch_size = 4; // burst length could be 2 and 4 (choose 4 here), 2n prefetch
    int channel_width = 128;

    struct SpeedEntry { // @NOTE we commit to BL = 2, and for computing timings we see if in the datasheet they are specified in nCK
                        // (we leave them the same), in tCK (we leave them the same), or in ns/ps (we compute the new number of cycles they are equal to)
        int rate;
        double freq, tCK;
        int nBL, nCCDS, nCCDL;
        int nCL, nRCDR, nRCDW, nRP, nCWL;
        int nRAS, nRC;		// nRAS = nRCD + nCL + nBL; nRC = nRAS + nRP
        int nRTP, nWTRS, nWTRL, nWR;
        int nRRDS, nRRDL, nFAW;
        int nRFC, nREFI, nREFI1B;
        int nPD, nXP;
        int nCKESR, nXS;
    } speed_table[int(Speed::MAX)] = {
        {1000,
        500, 2.0,
        2, 2, 3,
        7, 7, 6, 7, 4,
        17, 24,
        7, 2, 4, 8,
        4, 5, 20,
        0, 1950, 0,
        5, 5,
        5, 0},
        {600,
        300, 3.333,
        2, 2, 3,
        5, 5, 4, 5, 3,
        11, 15,
        7, 2, 4, 5,
        3, 4, 13,
        0, 1170, 0,
        4, 5,
        5, 0},
        {600,
        300, 3.333,
        2, 2, 3,
        10, 5, 4, 5, 6,
        17, 22,
        7, 2, 4, 5,
        3, 4, 13,
        0, 1170, 0,
        4, 5,
        5, 0},
        {600,
        300, 3.333,
        2, 2, 3,
        20, 5, 4, 5, 12,
        27, 32,
        7, 2, 4, 5,
        3, 4, 13,
        0, 1170, 0,
        4, 5,
        5, 0}
    }, speed_entry;

    int read_latency;

private:
    void init_speed();
    void init_lambda();
    void init_prereq();
    void init_rowhit();  // SAUGATA: added function to check for row hits
    void init_rowopen();
    void init_timing();
};

} /*namespace ramulator*/

#endif /*__HBM_AB_H*/
