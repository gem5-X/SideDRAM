#include "nmc_assembler.h"

// Format of assembly input:    Inst        [Operands]
// Format of raw traces:        Address     R/W         Data

enum class DefaultCmd : int {defaultRD, defaultWR, defaultInherit};

int main(int argc, const char *argv[])
{ 
    if (argc < 3 || argc > 5) {
        cout << "Usage: " << argv[0] << " <assembly-input> <raw-output> [<data-input> <address-input]" << endl;
        return 0;
    }

    string ai = argv[1];        // Input assembly file name
    string ro = argv[2];        // Output raw hexadecimal code
    string ailine;

    // Open input and output files
    ifstream assembly;
    ifstream dataFile;
    ifstream addrFile;
    ofstream rawSeq;
    assembly.open(ai);
    rawSeq.open(ro);

    // Prepare data input if needed
    if (argc == 5) {
        string di = argv[3];    // Input data file name
        string ad = argv[4];    // Input address file name
        dataFile.open(di);
        addrFile.open(ad);
    }

    bool error = false;
    DefaultCmd defaultCmd = DefaultCmd::defaultRD;  // Change here for the defaulr cmd policy

    // Variables for assembly decoding
    uint currLine = 0;
    INSTR instrType;
    RF_SEL storeType;
    string instrTypeString, storeTypeString;
    uint64_t idx;
    string dataString, diline;
    uint64_t dataAux;
    deque<uint64_t> rfData;

    // Variables for writing to IB
    std::array<nmcInst, IB_ENTRIES> ibInstr;
    uint ibIdx;
    bool ibWrMode = false;
    string dstAux, srcAux, addrAux, vwrAux, memAux, maskAux, modeAux, repackAux, vfuxTypeAux, lenAux;
    uint staAux, shiftAux;

    uint rowcol_aux;    // Concatenation of row and column that hold RF selection and address
    uint row_aux, col_aux; 

    // Variables for writing EXEC triggers
    uint8_t execIdx = 0;
    bool execStop;
    string memCmd;
    uint16_t lastRow, lastCol;
    string lastMemCmd;
    uint64_t loopStart, loopEnd, loopNumIter, loopCurrIter;

    // Variables to write the raw sequence
    uint64_t addr;

    // Run through assembly input
    while (getline(assembly, ailine) && !error) {

        currLine++;
        rfData.clear();

        // Don't process comment lines or emptly lines
        if(ailine.length() != 0 && !(ailine.at(0) == ';')){
            
            istringstream aistream(ailine);

            // Check instruction type
            if (!(aistream >> instrTypeString)) {
                cout << "Error when reading instruction type at line " << currLine << endl;
                error = true;
                break;
            }
            try{ instrType = STRING2INSTR.at(instrTypeString); }
            catch (const std::out_of_range& oor) {
                cout << "Error when reading instruction type at line " << currLine << endl;
                error = true;
                break;
            }

            switch (instrType) {

                // If Write to RF (WRF), check target RF
                case (INSTR::WRF):

                    // Split storage ID and index
                    if (!(aistream >> storeTypeString)) {
                        cout << "Error when reading which Register File to write to at line " << currLine << endl;
                        error = true;
                        break;
                    }
                    splitStoreIndex(&storeTypeString, &idx);

                    try{ storeType = STRING2RFSEL.at(storeTypeString); }
                    catch (const std::out_of_range& oor) {
                        cout << "Error when reading which Register File to write to at line " << currLine << endl;
                        error = true;
                        break;
                    }

                    // If WRF to IB, start IB writing mode starting at index n, checking if it's valid
                    if (storeType == RF_SEL::IB) {
                        if (idx < IB_ENTRIES) {
                            ibWrMode = true;
                            ibIdx = idx;
                        } else {
                            cout << "Error, starting to write out of IB range at line " << currLine << endl;
                            error = true;
                            break;
                        }

                    // Else, write to output the correct {A,R/W,D}
                    } else {
                        
                        // Obtain the data to write to the RF
                        if (aistream >> dataString) {
                            // Read from data file
                            if (!dataString.compare(DATAFILE)){
                                do {
                                    if (!getline(dataFile, diline)) {
                                        cout << "Error, data file cannot be read at line " << currLine << endl;
                                        error = true;   // Signal error
                                        break;
                                    }
                                } while (diline.length() == 0  || diline.at(0) == '#');
                                istringstream distream(diline);
                                while (distream >> hex >> dataAux) {
                                    rfData.push_back(dataAux);
                                }
                            // Read from assembly file
                            } else {
                                dataAux = stoull(dataString,0,0);
                                rfData.push_back(dataAux);
                                while (aistream >> dataAux) {
                                    rfData.push_back(dataAux);
                                }
                            }
                        }
                                                    
                        switch (storeType) {
                            case (RF_SEL::SRF):
                                if (rfData.size() != 1 || idx > SRF_ENTRIES) {
                                    cout << "Error trying to write to SRF at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            break;
                            case (RF_SEL::MRF):
                                if (rfData.size() != MASK_64B || idx > MASK_ENTRIES) {
                                    cout << "Error trying to write to MRF at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            break;
                            case (RF_SEL::CSDRF):
                                if (rfData.size() != CSD_64B || idx > CSD_ENTRIES) {
                                    cout << "Error trying to write to CSDRF at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            break;
                            case (RF_SEL::CSD_LEN):
                                if (rfData.size() != 1) {
                                    cout << "Error trying to write to CSD_LEN at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            break;
                            default:
                                cout << "Error trying to write with RF_SEL at line " << currLine << endl;
                                error = true;
                            break;
                        }

                        // Parse index to address
                        rowcol_aux = (uint(storeType) << RF_ADDR_BITS) + idx;
                        row_aux = rowcol_aux >> COL_BITS;
                        col_aux = rowcol_aux & ((1 << COL_BITS) - 1);
                        addr = build_addr({0, 0, 0, 0, row_aux, col_aux}, true);

                        // Write command
                        rawSeq << showbase << hex << addr << "\tWR";
                        while (!rfData.empty()) {
                            rawSeq << "\t" << showbase << hex << rfData.front();
                            rfData.pop_front();
                        }
                        rawSeq << endl;
                    }

                break;

                case (INSTR::LOOP):
                    storeType = RF_SEL::LOOP_REG;
                    if (!(aistream >> loopStart >> loopEnd >> loopNumIter)) {
                        cout << "Error when programming the hardware loop at line " << currLine << endl;
                        error = true;
                        break;
                    }

                    // Parse loop parameters to 64-bit uint
                    dataAux = 0;
                    dataAux |= (loopStart & 0xFFFF) << 32;
                    dataAux |= (loopEnd & 0xFFFF) << 16;
                    dataAux |= (loopNumIter & 0xFFFF);

                    // Parse index to address
                    rowcol_aux = (uint(storeType) << RF_ADDR_BITS);
                    row_aux = rowcol_aux >> COL_BITS;
                    col_aux = rowcol_aux & ((1 << COL_BITS) - 1);
                    addr = build_addr({0, 0, 0, 0, row_aux, col_aux}, true);

                    // Write command
                    rawSeq << showbase << hex << addr << "\tWR\t" << showbase << hex << dataAux << endl;
                break;
                
                // If EXEC, stop IB writing mode and write to output the {A,R/W,D} sequence
                // that will trigger execution starting from index 0, taking into account 
                // the loops (and maybe later the NOPs TODO)
                case (INSTR::EXEC): 

                    execStop = false;
                    lastRow = lastCol = 0;
                    lastMemCmd = "RD";
                    execIdx = 0;
                    loopCurrIter = 0;

                    // Run through array of instructions
                    while (execIdx < IB_ENTRIES && !execStop) {
                        
                        memCmd = "DC";

                        nmcInst execInstr = ibInstr.at(execIdx);

                        // Check if we need a specific address 
                        // and decide if we need a WR or RD command
                        if (execInstr.dst == OPC_STORAGE::DRAM) {
                            if (execInstr.dstAddrFile) {
                                error = getAddrFromFile(addrFile, &addr);
                            } else {
                                addr = execInstr.dstN;
                            }
                            memCmd = "WR";
                        } else if (execInstr.src == OPC_STORAGE::DRAM) {
                            if (execInstr.srcAddrFile) {
                                error = getAddrFromFile(addrFile, &addr);
                            } else {
                                addr = execInstr.srcN;
                            }
                            memCmd = "RD";
                        } else {
                            // If not, generate one.
                            addr = build_addr({0, 0, 0, 0, lastRow, lastCol}, false);
                            switch (defaultCmd){
                                case DefaultCmd::defaultWR:
                                    memCmd = "WR";
                                break;
                                case DefaultCmd::defaultInherit:
                                    memCmd = lastMemCmd;
                                break;
                                case DefaultCmd::defaultRD: 
                                default:
                                    memCmd = "RD";
                                break;
                            }
                        }

                        // Store last row and column used to minimize unnecessary Precharges
                        lastRow = get_row(addr);
                        lastCol = get_col(addr);
                        lastMemCmd = memCmd;

                        // Write command
                        rawSeq << showbase << hex << addr << "\t" << memCmd;

                        // Read from data file if needed 
                        if (execInstr.dataFile) {
                            error = getDataFromFile(dataFile, &execInstr);
                            if (error)  break;
                            while (!execInstr.data.empty()) {
                                rawSeq << "\t" << showbase << hex << execInstr.data.front();
                                execInstr.data.pop_front();   // Pop since next iteration has new data
                            }

                        // Read directly from the structure (data was in assembly file)
                        } else {
                            if (!execInstr.data.empty()) {
                                auto dataItr = execInstr.data.begin();
                                while (dataItr != execInstr.data.end()) {
                                    rawSeq << "\t" << showbase << hex << *dataItr;
                                    ++dataItr;
                                }
                            }
                        }
                        rawSeq << endl;

                        // Check if we stop execution
                        if (execInstr.ittIdx == MACRO_IDX::EXIT) {
                            execStop = true;

                        // Check if we are at the end address of the hardware loop
                        } else if (loopNumIter && execIdx == loopEnd) {
                            if (loopCurrIter < loopNumIter-1) { // Still looping
                                loopCurrIter++;
                                execIdx = loopStart;
                            } else { // End of loop
                                execIdx++;
                            }
                            
                        } else {
                            execIdx++;
                        }
                    }
                break;

                // Else, decode the instruction, write it into the instruction array mimicking the IB,
                // generate the traces for writing to the IB, advance index
                // and check it's not higher than the IB size
                default:
                    if (!ibWrMode || ibIdx >= IB_ENTRIES) {
                        cout << "Error when trying to write NMC instructions at line " << currLine << endl;
                        error = true;
                        break;
                    }
                    nmcInst* currInstr = new nmcInst();
                    switch (instrType) {

                        case INSTR::NOP:    // Exit or NOP
                            if (!(aistream >> currInstr->imm)) {
                                cout << "Error when reading NOP parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, currInstr->imm, "", "", "");
                            ibInstr.at(ibIdx) = *currInstr;
                        break;
                        
                        case INSTR::RLB:    // Read from DRAM to VWR
                            if (!(aistream >> addrAux >> vwrAux >> maskAux)) {
                                cout << "Error when reading RLB parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", "", "");
                            currInstr->srcAddrFile = interpretAddr(addrAux, &(currInstr->srcN));
                            currInstr->src = OPC_STORAGE::DRAM;
                            if (!splitVwrIndeces(vwrAux, &(currInstr->dst), &(currInstr->dstN), false)) {
                                cout << "Error when reading RLB VWR parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->imm = interpretMask(maskAux);
                            if (!error)
                                error = getInstData(aistream, currInstr);

                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        case INSTR::WLB:    // Write from VWR to DRAM
                            if (!(aistream >> addrAux >> vwrAux >> maskAux)) {
                                cout << "Error when reading WLB parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", "", "");
                            currInstr->dstAddrFile = interpretAddr(addrAux, &(currInstr->dstN));
                            currInstr->dst = OPC_STORAGE::DRAM;
                            if (!splitVwrIndeces(vwrAux, &(currInstr->src), &(currInstr->srcN), false)) {
                                cout << "Error when reading WLB VWR parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->imm = interpretMask(maskAux);
                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        case INSTR::VMV:    // Move from VWR to R0
                            if (!(aistream >> vwrAux)) {
                                cout << "Error when reading VMV parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", "", "");
                            if (!splitVwrIndeces(vwrAux, &(currInstr->src), &(currInstr->imm), true)) {
                                cout << "Error when reading VMV VWR parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        case INSTR::RMV:    // Move from R3 to VWR
                            if (!(aistream >> vwrAux)) {
                                cout << "Error when reading RMV parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", "", "");
                            currInstr->src0 = OPC_STORAGE::R3;  // We are bypassing the SA stage 
                            currInstr->swLen = SWSIZE::B24;     // Make it different from INV so that it's a SA instruction
                            if (!splitVwrIndeces(vwrAux, &(currInstr->dst), &(currInstr->dstN), true)) {
                                cout << "Error when reading RMV VWR parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        case INSTR::GLMV:   // Tile shuffling of the words in a VWR
                            if (!(aistream >> memAux)) {
                                cout << "Error when reading GLMV parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            if (memAux != "M2V") {
                                vwrAux = memAux;
                                if (!(aistream >> modeAux >> staAux)) {
                                    cout << "Error when reading GLMV parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            } else {
                                if (!(aistream >> addrAux >> vwrAux >> modeAux >> staAux)) {
                                    cout << "Error when reading GLMV parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            }

                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", "", memAux);
                            if (!splitVwrIndeces(vwrAux, &(currInstr->src), &(currInstr->srcN), false)) {
                                cout << "Error when reading GLMV VWR parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->dst = currInstr->src;
                            if (memAux == "M2V") {
                                currInstr->src = OPC_STORAGE::DRAM;
                                currInstr->srcAddrFile = interpretAddr(addrAux, &(currInstr->srcN));
                                if (!error)
                                    error = getInstData(aistream, currInstr);
                            }
                            
                            currInstr->imm = interpretShuffle(modeAux, staAux);
                            ibInstr.at(ibIdx) = *currInstr;
                        break;
                        
                        case INSTR::PACK:   // Repack format from R1,R2 to R3 or VWR
                            if (!(aistream >> repackAux >> currInstr->packSta >> dstAux)) {
                                cout << "Error when reading PACK parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", dstAux, "");
                            currInstr->repack = STRING2REPACK.at(repackAux);
                            // Read VWR information if that's the destination
                            if (dstAux == "OUT_VWR") {
                                if (!(aistream >> vwrAux)) {
                                    cout << "Error when reading PACK VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                                if (!splitVwrIndeces(vwrAux, &(currInstr->dst), &(currInstr->dstN), true)) {
                                    cout << "Error when reading PACK VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            } else {
                                currInstr->dst = OPC_STORAGE::R3;
                            }
                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        case INSTR::PERM:   // Permute R1,R2 to R3, VWR or both
                            if (!(aistream >> repackAux >> currInstr->packSta >> dstAux)) {
                                cout << "Error when reading PERM parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, 0, "", dstAux, "");
                            currInstr->repack = STRING2REPACK.at(repackAux);
                            // Read VWR information if that's the destination
                            if (dstAux == "OUT_VWR" || dstAux == "OUT_BOTH") {
                                if (!(aistream >> vwrAux)) {
                                    cout << "Error when reading PERM VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                                if (!splitVwrIndeces(vwrAux, &(currInstr->dst), &(currInstr->dstN), true)) {
                                    cout << "Error when reading PERM VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            } else {
                                currInstr->dst = OPC_STORAGE::R3;
                            }
                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        case INSTR::VFUX:   // VFUX operation
                            shiftAux = 0;
                            if (!(aistream >> vfuxTypeAux)) {
                                cout << "Error when reading VFUX parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            if (vfuxTypeAux == "SHIFT" && !((aistream >> shiftAux) || shiftAux > SA_MAX_SHIFT)) {
                                cout << "Error when reading VFUX SHIFT parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            if (!(aistream >> lenAux >> srcAux >> dstAux)) {
                                cout << "Error when reading VFUX parameters at line " << currLine << endl;
                                error = true;
                                break;
                            }
                            currInstr->ittIdx = inst2ittIdx(instrType, shiftAux, vfuxTypeAux, dstAux, srcAux);
                            currInstr->shiftSA = shiftAux;
                            currInstr->swLen = STRING2SWSIZE.at(lenAux);
                            // Process according to VFUX type
                            if (currInstr->ittIdx == MACRO_IDX::VFUX_MUL) {
                                splitStoreIndex(&srcAux, &(currInstr->rfN));
                                if (!(aistream >> vwrAux)) { // Decoding SRC_VWR_SEL and SRC_INDEX
                                    cout << "Error when reading VFUX VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                                if (!splitVwrIndeces(vwrAux, &(currInstr->src0), &(currInstr->src0N), true)) {
                                    cout << "Error when reading VFUX VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            } else {
                                if (srcAux == "SRC_VWR") {
                                    if (!(aistream >> vwrAux)) {
                                        cout << "Error when reading VFUX VWR parameters at line " << currLine << endl;
                                        error = true;
                                        break;
                                    }
                                    if (!splitVwrIndeces(vwrAux, &(currInstr->src0), &(currInstr->src0N), true)) {
                                        cout << "Error when reading VFUX VWR parameters at line " << currLine << endl;
                                        error = true;
                                        break;
                                    }
                                } else {
                                    currInstr->src0 = OPC_STORAGE::R3;
                                }
                            }
                            if (dstAux == "OUT_VWR") {
                                if (!(aistream >> vwrAux)) {
                                    cout << "Error when reading VFUX VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                                if (!splitVwrIndeces(vwrAux, &(currInstr->dst), &(currInstr->dstN), true)) {
                                    cout << "Error when reading VFUX VWR parameters at line " << currLine << endl;
                                    error = true;
                                    break;
                                }
                            } else {
                                currInstr->dst = STRING2VFUXDST.at(dstAux);
                            }
                            
                            ibInstr.at(ibIdx) = *currInstr;
                        break;

                        default:
                            cout << "Error, instruction type not recognized at line " << currLine << endl;
                            error = true;
                        break;
                    }

                    // Parse index to address
                    rowcol_aux = (uint(RF_SEL::IB) << RF_ADDR_BITS) + ibIdx;
                    row_aux = rowcol_aux >> COL_BITS;
                    col_aux = rowcol_aux & ((1 << COL_BITS) - 1);
                    addr = build_addr({0, 0, 0, 0, row_aux, col_aux}, true);

                    // Write command
                    rawSeq << showbase << hex << addr << "\tWR\t";
                    rawSeq << hex << build_macroinstr (uint(currInstr->ittIdx), currInstr->imm, currInstr->dst, currInstr->src, 
                                                        currInstr->swLen, currInstr->shiftSA, currInstr->rfN, currInstr->src0N, currInstr->src0,
                                                        currInstr->repack, currInstr->packSta, currInstr->shiftPM, currInstr->dstN, currInstr->dstN) << endl;

                    // Advance IB index
                    ibIdx++;
                    // cout << "line " << currLine << " decoded" << endl;
                break;
            }
        }
    }

    assembly.close();
    rawSeq.close();
    if (argc == 5) {
        dataFile.close();
        addrFile.close();
    }

    cout << "Raw sequence generated" << endl;

    return 0;
}

uint64_t build_addr(vector<uint64_t> addr_vec, bool rf_write)
{
    uint64_t addr_aux = 0;
    uint64_t offset = 6; // Don't know really why right now, maybe to address groups of 128

    if (rf_write) {
        addr_vec[int(Level::Row)] |= (1 << (addr_bits[int(Level::Row)]-1));
    }

    for (int i = 0; i < int(Level::MAX); i++) {
        if (addr_bits[int(addr_map[i])]) {
            addr_aux |= (addr_vec[int(addr_map[i])] << offset);
            offset += addr_bits[int(addr_map[i])];
        }
    }

    return addr_aux;
}

bool splitStoreIndex(string* storeTypeString, uint64_t *idx)
{
    bool split = false;
    uint8_t charidx = 0;
    string part1, part2;

    string aux = *storeTypeString;

    while (!split && charidx < aux.length()) {
        if (aux[charidx] >= '0' && aux[charidx] <= '9') {
            split = true;
            part1 = aux.substr(0,charidx);
            part2 = aux.substr(charidx);
        }
        ++charidx;
    }

    if (split) {
        *storeTypeString = part1;
        *idx = stoll(part2,0,0);
    }

    return split;
}

bool splitVwrIndeces(string vwrString, OPC_STORAGE *vwr_sel, uint64_t *vwr_idx, bool idxPresent)
{
    bool split = false;
    uint8_t charidx = 0;
    string part1, part2;

    string aux = vwrString;

    // Split according to the underscore location
    while (!split && charidx < aux.length()) {
        if (aux[charidx] == '_') {
            split = true;
            part1 = aux.substr(charidx+1);
        }
        ++charidx;
    }

    // Split the VWR_SEL and VWR_IDX
    if (split && idxPresent) {
        split = false;
        charidx = 0;
        while (!split && charidx < part1.length()) {
            if (part1[charidx] == '[') {
                split = true;
                part2 = part1.substr(charidx+1,part1.length()-charidx-2);
                part1 = part1.substr(0,charidx);
            }
            ++charidx;
        }
    }

    // Store the values
    if (split && stoi(part1) < VWR_NUM) {
        *vwr_sel = OPC_STORAGE(uint(OPC_STORAGE::VWR0) + stoi(part1));
        if (idxPresent) {
            *vwr_idx = stoll(part2,0,0);
        }
    }

    return split;
}

bool interpretAddr(string addrString, uint64_t *addr)
{
    bool addressFromFile = false;
    
    if (addrString.compare(ADDRFILE)) {
        *addr = stoll(addrString,0,0);
    } else {
        addressFromFile = true;
    }

    return addressFromFile;
}

uint interpretMask(string maskString)
{
    uint mask = 0;
#if FLEXIBLE_MASK
    if (maskString == "ALL_WORDS") {
        mask = ((1 << WORDS_PER_VWR) - 1);
    }
    else { // Get the word number and create the mask
        string wordNum = maskString.substr(maskString.find("_") + 1);
        mask = 1 << stoi(wordNum);
    }
#else
    if (maskString == "ALL_WORDS") {
        mask = 0;
    }
    else { // Get the word number and create the mask
        string wordNum = maskString.substr(maskString.find("_") + 1);
        mask = stoi(wordNum) + 1;
    }
#endif
    return mask;
}

uint interpretShuffle(string modeString, uint start_idx)
{
    uint imm = 0;
    TS_MODE mode;
    
    try {
        mode = STRING2TSMODE.at(modeString);
    } catch (const std::out_of_range& oor) {
        cout << "Error when reading GLMV mode" << endl;
        return 0;
    }

    imm |= (((uint) mode) & ((1 << TS_MODE_BITS) - 1));
    imm <<= TS_STA_BITS;
    imm |= (start_idx & ((1 << TS_STA_BITS) - 1));
    
    return imm;
}

uint8_t get_bank(uint64_t addr) {
    uint64_t bank, bankMask;
    uint offset = 6; // Initial offset
    uint lvl = 0;

    addr = addr >> offset;  // Remove initial offset
    // Shif to the right until the bank is aligned
    while (addr_map[lvl] != Level::Bank) {
        addr = addr >> addr_bits[int(addr_map[lvl])];
        lvl++;
    }
    // Obtain the mask
    bankMask = (1 << addr_bits[int(addr_map[lvl])]) - 1;
    // Get the bank index
    bank = addr & bankMask;

    return bank;
}

uint16_t get_row(uint64_t addr) {
    uint64_t row, rowMask;
    uint offset = 6; // Initial offset
    uint lvl = 0;

    addr = addr >> offset;  // Remove initial offset
    // Shif to the right until the row is aligned
    while (addr_map[lvl] != Level::Row) {
        addr = addr >> addr_bits[int(addr_map[lvl])];
        lvl++;
    }
    // Obtain the mask
    rowMask = (1 << addr_bits[int(addr_map[lvl])]) - 1;
    // Get the row index
    row = addr & rowMask;

    return row;
}

uint16_t get_col(uint64_t addr) {
    uint64_t col, colMask;
    uint offset = 6; // Initial offset
    uint lvl = 0;

    addr = addr >> offset;  // Remove initial offset
    // Shif to the right until the column is aligned
    while (addr_map[lvl] != Level::Column) {
        addr = addr >> addr_bits[int(addr_map[lvl])];
        lvl++;
    }
    // Obtain the mask
    colMask = (1 << addr_bits[int(addr_map[lvl])]) - 1;
    // Get the column index
    col = addr & colMask;

    return col;
}

MACRO_IDX inst2ittIdx(INSTR instr, uint imm, string vfuxType, string dst, string src) {
    MACRO_IDX ittIdx = MACRO_IDX::SAFE_STATE;
    switch (instr) {
        case INSTR::NOP:    ittIdx = imm ? MACRO_IDX::NOP : MACRO_IDX::EXIT;    break;
        case INSTR::RLB:    ittIdx = MACRO_IDX::RLB;                            break;
        case INSTR::WLB:    ittIdx = MACRO_IDX::WLB;                            break;
        case INSTR::VMV:    ittIdx = MACRO_IDX::VMV;                            break;
        case INSTR::RMV:    ittIdx = MACRO_IDX::RMV;                            break;
        case INSTR::GLMV:
            ittIdx = (src == "M2V") ? MACRO_IDX::GLMV_FROMDRAM : MACRO_IDX::GLMV;
        break;
        case INSTR::PACK:   
            if (dst == "OUT_VWR") {
                ittIdx = MACRO_IDX::PACK_TOVWR;
            } else if (dst == "OUT_R3") {
                ittIdx = MACRO_IDX::PACK_TOR3;
            }
        break;
        case INSTR::PERM:
            if (dst == "OUT_VWR") {
                ittIdx = MACRO_IDX::PERM_TOVWR;
            } else if (dst == "OUT_R3") {
                ittIdx = MACRO_IDX::PERM_TOR3;
            } else if (dst == "OUT_BOTH") {
                ittIdx = MACRO_IDX::PERM_TOBOTH;
            }
        break;
        case INSTR::VFUX:
            if (vfuxType == "SHIFT") {
#if (INSTR_FORMAT == BASE_FORMAT)
                if (dst == "OUT_VWR") {
                    ittIdx = MACRO_IDX::VFUX_SHIFT_FROMTOVWR;
                } else if (src == "SRC_VWR") {
                    ittIdx = MACRO_IDX::VFUX_SHIFT_FROMVWR;
                } else if (src == "SRC_R3") {
                    ittIdx = MACRO_IDX::VFUX_SHIFT_FROMR3;
                }
#elif (INSTR_FORMAT == ENCODED_SHIFT_FORMAT)
                if (dst == "OUT_VWR") {
                    switch (imm) {
                        case 0: ittIdx = MACRO_IDX::VFUX_SHIFT0_FROMTOVWR; break; 
                        case 1: ittIdx = MACRO_IDX::VFUX_SHIFT1_FROMTOVWR; break; 
                        case 2: ittIdx = MACRO_IDX::VFUX_SHIFT2_FROMTOVWR; break; 
                        case 3: ittIdx = MACRO_IDX::VFUX_SHIFT3_FROMTOVWR; break; 
                    }
                } else if (src == "SRC_VWR") {
                    switch (imm) {
                        case 0: ittIdx = MACRO_IDX::VFUX_SHIFT0_FROMVWR; break; 
                        case 1: ittIdx = MACRO_IDX::VFUX_SHIFT1_FROMVWR; break; 
                        case 2: ittIdx = MACRO_IDX::VFUX_SHIFT2_FROMVWR; break; 
                        case 3: ittIdx = MACRO_IDX::VFUX_SHIFT3_FROMVWR; break; 
                    }
                } else if (src == "SRC_R3") {
                    switch (imm) {
                        case 0: ittIdx = MACRO_IDX::VFUX_SHIFT0_FROMR3; break; 
                        case 1: ittIdx = MACRO_IDX::VFUX_SHIFT1_FROMR3; break; 
                        case 2: ittIdx = MACRO_IDX::VFUX_SHIFT2_FROMR3; break; 
                        case 3: ittIdx = MACRO_IDX::VFUX_SHIFT3_FROMR3; break; 
                    }
                }
#endif
            } else if (vfuxType == "ADD") {
                if (dst == "OUT_VWR") {
                    ittIdx = MACRO_IDX::VFUX_ADD_FROMTOVWR;
                } else if (src == "SRC_VWR") {
                    ittIdx = MACRO_IDX::VFUX_ADD_FROMVWR;
                } else if (src == "SRC_R3") {
                    ittIdx = MACRO_IDX::VFUX_ADD_FROMR3;
                }
            } else if (vfuxType == "SUB") {
                if (dst == "OUT_VWR") {
                    ittIdx = MACRO_IDX::VFUX_SUB_FROMTOVWR;
                } else if (src == "SRC_VWR") {
                    ittIdx = MACRO_IDX::VFUX_SUB_FROMVWR;
                } else if (src == "SRC_R3") {
                    ittIdx = MACRO_IDX::VFUX_SUB_FROMR3;
                }
            } else if (vfuxType == "MUL") {
                ittIdx = MACRO_IDX::VFUX_MUL;
            } 
        break;
        default:            ittIdx = MACRO_IDX::SAFE_STATE;                     break;
    }
    return ittIdx;
}

bool getInstData(istringstream &aistream, nmcInst *currInstr) {
    string dataString;
    uint64_t dataAux;

    // Obtain the data to be moved from a bank if needed 
    if (aistream >> dataString) {
        // Read from data file
        if (!dataString.compare(DATAFILE)){
            currInstr->dataFile = true;
        // Read from assembly file
        } else {
            dataAux = stoull(dataString,0,0);
            currInstr->data.push_back(dataAux);
            while (aistream >> hex >> dataAux) {
                currInstr->data.push_back(dataAux);
            }
            if (currInstr->data.size() % VWR_64B) {
                cout << "Error, not enough 64-bit words to fill a multiple of VWRs" << endl;
                return true; // Error
            }
        }
    }

    return false;
}

bool getDataFromFile (ifstream &dataFile, nmcInst *currInstr) {
    string diline;
    uint64_t dataAux;
    
    do {
        if (!getline(dataFile, diline)) {
            cout << "Error, data file cannot be read" << endl;
            return true;    // Signal error
        }
    } while (diline.length() == 0 || diline.at(0) == '#');

    istringstream distream(diline);

    while (distream >> hex >> dataAux) {
        currInstr->data.push_back(dataAux);
    }
    
    if (currInstr->data.size() % VWR_64B) {
        cout << "Error, not enough 64-bit words to fill a multiple of VWRs" << endl;
        return true; // Error
    }

    return false;
}

bool getAddrFromFile (ifstream &addrFile, uint64_t *addr) {
    string ailine;

    do {
        if (!getline(addrFile, ailine)) {
            cout << "Error, address file cannot be read" << endl;
            return true;    // Signal error
        }
    } while (ailine.length() == 0 || ailine.at(0) == '#');

    istringstream aistream(ailine);
    aistream >> hex >> *addr;

    return false;
}