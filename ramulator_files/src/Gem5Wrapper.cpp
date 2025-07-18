#include <map>

#include "Gem5Wrapper.h"
#include "Config.h"
#include "Request.h"
#include "MemoryFactory.h"
#include "Memory.h"
#include "DDR3.h"
#include "DDR4.h"
#include "DDR4_AB.h"
#include "LPDDR3.h"
#include "LPDDR4.h"
#include "LPDDR4_AB.h"
#include "GDDR5.h"
#include "GDDR5_AB.h"
#include "WideIO.h"
#include "WideIO2.h"
#include "HBM.h"
#include "HBM_AB.h"
#include "HBM2.h"
#include "HBM2_AB.h"
#include "SALP.h"
#include "PCM_AB.h"
#include "RRAM_AB.h"
#include "STTRAM_AB.h"

using namespace ramulator;

static map<string, function<MemoryBase *(const Config&, int)> > name_to_func = {
    {"DDR3", &MemoryFactory<DDR3>::create}, {"DDR4", &MemoryFactory<DDR4>::create}, {"DDR4_AB", &MemoryFactory<DDR4_AB>::create},
    {"LPDDR3", &MemoryFactory<LPDDR3>::create}, {"LPDDR4", &MemoryFactory<LPDDR4>::create}, {"LPDDR4_AB", &MemoryFactory<LPDDR4_AB>::create},
    {"GDDR5", &MemoryFactory<GDDR5>::create}, {"GDDR5_AB", &MemoryFactory<GDDR5_AB>::create}, 
    {"WideIO", &MemoryFactory<WideIO>::create}, {"WideIO2", &MemoryFactory<WideIO2>::create},
    {"HBM", &MemoryFactory<HBM>::create}, {"HBM_AB", &MemoryFactory<HBM_AB>::create},
    {"HBM2", &MemoryFactory<HBM2>::create}, {"HBM2_AB", &MemoryFactory<HBM2_AB>::create},
    {"SALP-1", &MemoryFactory<SALP>::create}, {"SALP-2", &MemoryFactory<SALP>::create}, {"SALP-MASA", &MemoryFactory<SALP>::create},
    {"PCM_AB", &MemoryFactory<PCM_AB>::create}, {"RRAM_AB", &MemoryFactory<RRAM_AB>::create}, {"STTRAM_AB", &MemoryFactory<STTRAM_AB>::create}
};


Gem5Wrapper::Gem5Wrapper(const Config& configs, int cacheline)
{
    const string& std_name = configs["standard"];
    assert(name_to_func.find(std_name) != name_to_func.end() && "unrecognized standard name");
    mem = name_to_func[std_name](configs, cacheline);
    tCK = mem->clk_ns();
}


Gem5Wrapper::~Gem5Wrapper() {
    delete mem;
}

void Gem5Wrapper::tick()
{
    mem->tick();
}

bool Gem5Wrapper::send(Request req)
{
    return mem->send(req);
}

void Gem5Wrapper::finish(void) {
    mem->finish();
}
