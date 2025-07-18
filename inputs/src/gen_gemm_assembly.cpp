#include "gen_gemm_assembly.h"

/*** This program generates assembly files for the SoftSIMD-near-DRAM system
 *   that correspond with the execution of GEMM operation of configurable dimensions.
 *   A is an mxn matrix, B is an nxq matrix, and C is an mxq matrix.
 */

/** 
 * Function to handle parsing of the command line arguments
 * @param argc Number of arguments
 * @param argv Array of arguments
 * @param m Vertical dimension of the A matrix (--m=)
 * @param n Horizontal dimension of the A matrix and vertical dimension of the B matrix (--n=)
 * @param q Horizontal dimension of the B matrix (--q=)
 * @param sw_bw Initial bitwidth of the subword operands (--sw_bw=)
 * @param csd_len Length of the CSD operands (--csd_len=)
 * @param dram_intvl Minimum interval between accesses to a DRAM column
 * @param output Output files name (--output=)
 * @param verbose Flag to enable verbose output (-v)
 */
void parse_args (int argc, char **argv, int &m, int &n, int &q, int &sw_bw, int &csd_len, int &dram_intvl,
                std::string &output, bool &verbose) {
    // Check if the number of arguments is correct
    if (argc < 7) {
        std::cerr << "Usage: " << argv[0];
        std::cerr << " --m=<m> --n=<n> --q=<q> --sw_bw=<sw_bw> --csd_len=<csd_len> --dram_intvl=<dram_intvl> --output=<output> [-v]" << std::endl;
        exit(1);
    }

    // Default values
    m = 128;
    n = 128;
    q = 128;
    sw_bw = 8;
    csd_len = 8;
    dram_intvl = 5;
    output = "output";

    // Parse the arguments, removing the -- and - prefixes
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.find("--m=") == 0) {
            m = std::stoi(arg.substr(4));
        } else if (arg.find("--n=") == 0) {
            n = std::stoi(arg.substr(4));
        } else if (arg.find("--q=") == 0) {
            q = std::stoi(arg.substr(4));
        } else if (arg.find("--sw_bw=") == 0) {
            sw_bw = std::stoi(arg.substr(8));
        } else if (arg.find("--csd_len=") == 0) {
            csd_len = std::stoi(arg.substr(10));
        } else if (arg.find("--dram_intvl=") == 0) {
            dram_intvl = std::stoi(arg.substr(13));
        } else if (arg.find("--output=") == 0) {
            output = arg.substr(9);
        } else if (arg == "-v") {
            verbose = true;
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            exit(1);
        }
    }
}

/** 
 * Function to generate a random CSD operand
 * @param csd_len Length of the CSD operand
 * @param gen Random number generator
 * @return Random CSD operand
 */
uint64_t generate_random_csd(uint csd_len, std::mt19937 &gen) {
    std::uniform_int_distribution<uint64_t> dis(0, 2);
    uint64_t csd = 0;
    uint64_t new_bit = 0;
    uint64_t prev_bit = 0;
    for (uint i = 0; i < csd_len; i++) {
        if (prev_bit == 0) {
            new_bit = dis(gen);
        
        } else {
            new_bit = 0;
        }
        prev_bit = new_bit;    
        csd |= new_bit << 2*i;
    }
    return csd;
}

/**
 * Function to write a random VWR to the data file
 * @param sw_bw Bitwidth of the subword operands
 * @param dataFile Output stream for the data file
 * @param gen Random number generator
 */
void write_random_vwr (uint sw_bw, std::ofstream &dataFile, std::mt19937 &gen) {
    uint64_t vwr[VWR_64B];
    uint64_t subword;

    if (!sw_bw) {
        std::cerr << "Subword bitwidth cannot be zero" << std::endl;
        exit(1);
    }

    std::uniform_int_distribution<uint64_t> dis(0, (1 << (sw_bw-1)) - 1);
    for (uint c = 0; c < CORES_PER_PCH; c++) {
        for (uint i = 0; i < VWR_64B; i++) {
            vwr[i] = 0;
        }
        for (uint i = 0; i < WORDS_PER_VWR*WORD_BITS/sw_bw; i++) {
            subword = dis(gen);
            subword |= (subword & (1 << (sw_bw-2))) << 1;   // Duplicate the sign bit
            if (((i + 1) * sw_bw - 1) % 64 >= sw_bw) {  // Complete subword in 64-bit word
                vwr[(i*sw_bw)/64] |= subword << ((i*sw_bw) % 64);
            } else {    // Subword crosses 64-bit word boundary
                vwr[(i*sw_bw)/64] |= subword << ((i*sw_bw) % 64);
                vwr[(i*sw_bw)/64 + 1] |= subword >> (64 - ((i*sw_bw) % 64));
            }
        }
        for (uint i = 0; i < VWR_64B; i++) {    // @NOTE check if this is the correct write order, or if it should be reversed
            dataFile << std::showbase << std::hex << vwr[i] << "\t";
        }
    }
    dataFile << std::endl;
}

/**
 * 
 */
uint64_t nxt_addr(uint64_t chIdx, uint64_t &roIdx, uint64_t &coIdx)
 {
    // Considering rank, bank group and bank as 0
    uint64_t raIdx = 0;
    uint64_t bgIdx = 0;
    uint64_t baIdx = 0;

    if (++coIdx >= NUM_COL) {
        coIdx = 0;
        if (++roIdx >= (NUM_ROW/2)) {    // Half the number of rows, as the next half is to program CnM
            roIdx = 0;
        }
    }

    return ((chIdx << SHIFT_CH) + 
            (raIdx << SHIFT_RANK) + 
            (bgIdx << SHIFT_BG) + 
            (baIdx << SHIFT_BANK) + 
            (roIdx << SHIFT_ROW) + 
            (coIdx << SHIFT_COL));
 }

/** 
 * Function to initialize the add tree vector with the necessary layers
 * @param addTreeVector Vector to store the add tree layers
 * @param addTreeLayerTotal Total number of layers in the add tree
 * @param addTreeInputsPerDP Number of inputs to the add tree to compute dot product
 * @param sw_bw_0 Initial bitwidth of the subword operands
 * @param q Horizontal dimension of the B matrix
 * @param verbose Flag to enable verbose output
 */
void initiallizeAddTreeVector (std::vector<addTreeLayer> &addTreeVector, uint addTreeLayerTotal,
                                uint addTreeInputsPerDP, uint sw_bw_0, uint q, bool verbose) {
    // Initialize the add tree vector
    for (uint i = 0; i < addTreeLayerTotal; i++) {
        addTreeLayer addTreeLayer;
        addTreeLayer.idx = i + 1;
        addTreeLayer.inputs = (i == 0) ? addTreeInputsPerDP : addTreeVector[i-1].outputs;
        addTreeLayer.outputs = div_ceil(addTreeLayer.inputs, 2);
        addTreeLayer.additions = addTreeLayer.inputs / 2;
        addTreeLayer.ADDOps = (i == 0) ? 
                            div_ceil(q, (WORD_BITS * CORES_PER_PCH / sw_bw_0)) * addTreeLayer.additions : 
                            div_ceil(q, (WORD_BITS * CORES_PER_PCH / swsize_to_uint(addTreeVector[i-1].sw_out))) * addTreeLayer.additions;
        if (i == 0) {
            addTreeLayer.sw_in = uint_to_swsize(sw_bw_0);
            addTreeLayer.sw_out = next_sw(addTreeLayer.sw_in);
        } else {
            addTreeLayer.sw_in = addTreeVector[i-1].sw_out;
            uint sw_diff = swsize_to_uint(addTreeLayer.sw_in) - swsize_to_uint(prev_sw(addTreeLayer.sw_in));
            // Verify if the subwords would overflow in the next addition. If so, increase the subword size
            if (sw_diff > i) {
                if (swsize_to_uint(addTreeLayer.sw_in) > sw_bw_0) { // If difference is larger, compare to initial subword size
                    addTreeLayer.sw_out = addTreeVector[i-1].sw_out;
                } else {
                    addTreeLayer.sw_out = next_sw(addTreeLayer.sw_in);
                    if (addTreeLayer.sw_in == SWSIZE::B24)  std::cout << "WARNING: Subword size overflow" << std::endl;
                }
            } else {
                if (swsize_to_uint(addTreeLayer.sw_in) > swsize_to_uint(addTreeVector[i-sw_diff].sw_out)) {
                    addTreeLayer.sw_out = addTreeVector[i-1].sw_out;
                } else {
                    addTreeLayer.sw_out = next_sw(addTreeLayer.sw_in);
                    if (addTreeLayer.sw_in == SWSIZE::B24)  std::cout << "WARNING: Subword size overflow" << std::endl;
                }
            }
        }
        addTreeLayer.sw_change = repack_sw(addTreeLayer.sw_in, addTreeLayer.sw_out);
        addTreeLayer.PACKOps = (addTreeLayer.sw_change == SWREPACK::INV) ? 
                                0 :
                                div_ceil(q, (WORD_BITS * CORES_PER_PCH / swsize_to_uint(addTreeLayer.sw_out)) * addTreeLayer.outputs);

        addTreeVector.push_back(addTreeLayer);

        if (verbose) {
            std::cout << "Layer " << addTreeLayer.idx << ": Inputs = " << addTreeLayer.inputs << ", Outputs = " << addTreeLayer.outputs;
            std::cout << ", Additions = " << addTreeLayer.additions << ", ADDOps = " << addTreeLayer.ADDOps;
            std::cout << ", SW in = " << swsize_to_uint(addTreeLayer.sw_in) << ", SW out = " << swsize_to_uint(addTreeLayer.sw_out);
            std::cout << ", SW change = " << REPACK_SEL_STRING.at(addTreeLayer.sw_change) << ", PACKOps = " << addTreeLayer.PACKOps << std::endl;
            std::cout << "------------------------------------------------" << std::endl;
        }
    }
}

/**
 * Function to generate the assembly code for the ADD reduction operation within a code chunk.
 * @param curAddLayer Current layer of the ADD tree
 * @param curInstrCount Current instruction count
 * @param numSw Total number of subwords in the reduction layer at the VWR
 * @param originalSwPerWord Initial number of subwords per word before reduction
 * @param addTreeLayerTotal Total number of layers in the add tree
 * @param addTreeVector Vector to store the add tree layers
 * @param assembly Output stream for the assembly code
 * @param limitLayer Layer to stop the reduction (if 0, reduction continues until possible)
 * @param verbose Flag to enable verbose output
 */
void generate_add_reduction_assembly (uint &curAddLayer, uint &curInstrCount, uint &numSw, uint originalSwPerWord, uint addTreeLayerTotal,
                                    std::vector<addTreeLayer> &addTreeVector, std::ofstream &assembly, uint limitLayer, bool verbose){
    for (;;) {

        // Check if more layers needs to be computed
        if ((curAddLayer >= addTreeLayerTotal) || (limitLayer && curAddLayer >= limitLayer)) {
            if (verbose) {
                std::cout << "Stopping reduction after layer " << addTreeVector[curAddLayer-1].idx << ", current instruction count is " << curInstrCount << std::endl;
            }
            return;
        }

        uint swProcessed = 0;   // Keep track of the number of subwords processed in the current layer not to do useless PACK operations
        uint swInPerWord = WORD_BITS / swsize_to_uint(addTreeVector[curAddLayer].sw_in);
        uint swOutPerWord = WORD_BITS / swsize_to_uint(addTreeVector[curAddLayer].sw_out);
        uint originalWordPkt = lcm(swInPerWord, originalSwPerWord) / swInPerWord;   // How many words to hold the an entire original packet of subwords
        uint wordsPerLayer = div_ceil(numSw, swInPerWord);                         // How many words are needed to hold the entire layer of subwords

        // Compute the number of operations per reduced layer
        uint VMVOpsPerReducLayer = (wordsPerLayer / (2*originalWordPkt)) * originalWordPkt; // Number of VMV operations per reduced layer
        uint ADDOpsPerReducLayer = VMVOpsPerReducLayer;
        uint wordsNotReduced = wordsPerLayer % (2*originalWordPkt);     // Number of words that are not reduced in this layer
        uint SHIFTOpsPerReducLayer = 0;                                 // Number of SHIFT operations per reduced layer in order to repack non-reduced words
        uint PACKOpsPerReducLayer = 0;                                  // Number of PACK operations per reduced layer in order to repack reduced and non-reduced words

        // Start computing number of subwords for next layer in case
        uint reducedSw = ADDOpsPerReducLayer * swInPerWord;             // Number of subwords results of addition in this layer
        uint nonReducedSw = numSw - reducedSw*2;                        // Number of subwords that are not reduced in this layer

        if (addTreeVector[curAddLayer].sw_change != SWREPACK::INV) {   
            SHIFTOpsPerReducLayer = wordsNotReduced;
            PACKOpsPerReducLayer = div_ceil(reducedSw + nonReducedSw, swOutPerWord);
        }

        // Check if the next layer can be added
        if ((curInstrCount + VMVOpsPerReducLayer + ADDOpsPerReducLayer + SHIFTOpsPerReducLayer + PACKOpsPerReducLayer + 2 > IB_ENTRIES) ||
            (ADDOpsPerReducLayer == 0)) {
            if (verbose) {
                std::cout << "Stopping reduction before layer " << addTreeVector[curAddLayer].idx << ", current instruction count is " << curInstrCount << std::endl;
            }
            return;
        }
        if (verbose) {
            std::cout << "------------------------------------------------" << std::endl;
            std::cout << "In the code chunk, one more reduction at layer " << addTreeVector[curAddLayer].idx << std::endl;
            std::cout << "VMV operations: " << VMVOpsPerReducLayer << ", ADD operations: " << ADDOpsPerReducLayer;
            std::cout << ", SHIFT operations: " << SHIFTOpsPerReducLayer << ", PACK operations: " << PACKOpsPerReducLayer << std::endl;
        }

        // Generate the assembly code for the next layer
        uint packVWR = 0;
        uint packVWRIdx = 0;

        uint wordPkt = lcm(swInPerWord, swOutPerWord) / swInPerWord;
        uint curWordPkt = 0;
        uint swInPerR2R1 = 2 * swInPerWord;
        uint curSwR1R2 = 0;

        // Generate the assembly code for the ADD operations @NOTE the VWR indeces don't align with behavior, as packing shuffles data around
        for (uint i = 0; i < VMVOpsPerReducLayer; i++) {
            assembly << "VMV VWR_0[" << i << "]" << std::endl;
            assembly << "VFUX ADD LEN_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << " SRC_VWR OUT_";
            // Different ADD destination depending on whether PACK operations are needed
            if (PACKOpsPerReducLayer) { 
                assembly << "R" << (i % 2 ? "2" : "1") << " VWR_1[" << i << "]" << std::endl; 
            } else { 
                assembly << "VWR VWR_1[" << i << "] VWR_0" << "[" << i << "]" << std::endl;     // Avoid using the same VWR for source and destination
            }
            if (PACKOpsPerReducLayer) {
                curWordPkt++;
                assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << "_" << swsize_to_uint(addTreeVector[curAddLayer].sw_out) << " "\
                        << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                swProcessed += swOutPerWord;
                curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                    packVWR = 0;
                    packVWRIdx++;
                }
                if (curWordPkt == wordPkt && swProcessed < reducedSw) {
                    assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << "_" << swsize_to_uint(addTreeVector[curAddLayer].sw_out) << " "\
                            << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                    swProcessed += swOutPerWord;
                    curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                    if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                        packVWR = 0;
                        packVWRIdx++;
                    }
                    curWordPkt = 0; // Word packet is complete, reset the counter
                }
            }
        }

        if (PACKOpsPerReducLayer) {
            swProcessed = SHIFTOpsPerReducLayer ? 0 : swProcessed;  // Reset the number of subwords processed to measure only non-reduced subwords
            for (uint i = 0; i < SHIFTOpsPerReducLayer; i++) {
                assembly << "VFUX SHIFT 0 LEN_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << " SRC_VWR OUT_R"\
                        << ((i + VMVOpsPerReducLayer) % 2 ? "2" : "1") << " VWR_1[" << i+VMVOpsPerReducLayer << "]" << std::endl; 
                curWordPkt++;
                assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << "_" << swsize_to_uint(addTreeVector[curAddLayer].sw_out) << " "\
                        << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                swProcessed += swOutPerWord;
                curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                    packVWR = 0;
                    packVWRIdx++;
                }
                if (curWordPkt == wordPkt && swProcessed < nonReducedSw) {
                    assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << "_" << swsize_to_uint(addTreeVector[curAddLayer].sw_out) << " "\
                            << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                    swProcessed += swOutPerWord;
                    curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                    if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                        packVWR = 0;
                        packVWRIdx++;
                    }
                    curWordPkt = 0; // Word packet is complete, reset the counter
                }
            }
            // In case there are subwords left to pack in R2|R1 when there are no shifts, pack them
            if (curWordPkt && !SHIFTOpsPerReducLayer && swProcessed < reducedSw) {
                assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[curAddLayer].sw_in) << "_" << swsize_to_uint(addTreeVector[curAddLayer].sw_out) << " "\
                        << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                swProcessed += swOutPerWord;
            }
        }

        curInstrCount += VMVOpsPerReducLayer + ADDOpsPerReducLayer + SHIFTOpsPerReducLayer + PACKOpsPerReducLayer;
        curAddLayer++;
        numSw = reducedSw + nonReducedSw;
        if (verbose) {
            std::cout << "Current instruction count: " << curInstrCount << ", number of subwords: " << numSw << std::endl;
        }
    }
}

/** 
 * Function to generate the assembly code for the GEMM operation.
 * @param assembly Output stream for the assembly code
 * @param dataFile Output stream for the data file
 * @param addrFile Output stream for the address file
 * @param m Vertical dimension of the A matrix
 * @param n Horizontal dimension of the A matrix and vertical dimension of the B matrix
 * @param q Horizontal dimension of the B matrix
 * @param sw_bw Initial bitwidth of the subword operands
 * @param csd_len Length of the CSD operands
 * @param dram_intvl Minimum interval between accesses to a DRAM column
 * @param verbose Flag to enable verbose output
 * @param gen Random number generator
 */
void map_gemm (std::ofstream &assembly, std::ofstream &dataFile, std::ofstream &addrFile, 
                int m, int n, int q, int sw_bw, int csd_len, int dram_intvl, bool verbose, std::mt19937 &gen) {

    MAPMODE mapMode = MAPMODE::UNCONSTRAINED;   // Mapping mode, UNCONSTRAINED by default
    uint64_t channel = 0;               // Assume channel 0
    uint64_t inputRow = 0;              // Assume input row 0
    uint64_t inputCol = 0;              // Assume input column 0
    uint64_t outputRow = (NUM_ROW/4);   // Assume output row is at the middle of the Computing DRAM
    uint64_t outputCol = 0;             // Assume output column 0

    // Compute the count of necessary operations
    uint MULOpsPerBRow = div_ceil(q, (WORD_BITS * CORES_PER_PCH) / sw_bw);  // Number of VFUX MUL per row of B
    uint addTreeInputsPerDP = n;                                            // Number of inputs to the add tree to compute dot product
    uint addTreeLayerTotal = uint(ceil(log2(addTreeInputsPerDP)));          // Number of layers in the add tree

    if (verbose) {
        std::cout << "Number of VFUX MUL operations per row of B: " << MULOpsPerBRow << std::endl;
        std::cout << "Number of inputs to the add tree to compute dot product: " << addTreeInputsPerDP << std::endl;
        std::cout << "Number of layers in the add tree: " << addTreeLayerTotal << std::endl;
    }

    std::vector<addTreeLayer> addTreeVector;    // Vector to store the add tree layers
    initiallizeAddTreeVector(addTreeVector, addTreeLayerTotal, addTreeInputsPerDP, sw_bw, q, verbose);   // Initialize the add tree vector

    // Compute how many NOP cycles are needed after a VFUX MUL operation according to the CSD operand length and the DRAM standard
    uint maxCyclesPerMUL = (csd_len / 2) + 1 + 1;  // Worst case for multiplication is alternating 01010101... (csd_len/2+1), and writing to R0/1/2 (+1)
    uint NOPCyclesPerMUL = (maxCyclesPerMUL > dram_intvl) ? maxCyclesPerMUL - dram_intvl : 0;   // @TODO if NOPCyclesPerMUL > dram_intvl, need to handle multiple NOPs
    if (verbose) {
        std::cout << "Maximum cycles per MUL operation: " << maxCyclesPerMUL << std::endl;
        std::cout << "NOP cycles per MUL operation: " << NOPCyclesPerMUL << std::endl;
    }
    if (NOPCyclesPerMUL > dram_intvl) {
        std::cout << "Warning: NOP cycles (" << NOPCyclesPerMUL << ") per MUL operation exceed the minimum interval between DRAM commands" << std::endl;
    }

    // Write into the assembler the description of the workload
    assembly << "; GEMM operation with dimensions " << m << "x" << n << " and " << n << "x" << q;
    assembly << ", with initial subword bitwidth = " << sw_bw << ", CSD length = " << csd_len;
    assembly << " and DRAM interval = " << dram_intvl << " cycles " << std::endl;
    assembly << "; Hardware parameters: IB_ENTRIES = " << IB_ENTRIES << ", CSD_ENTRIES = " << CSD_ENTRIES << ", WORD_BITS = " << WORD_BITS;
    assembly << ", VWR_BITS = " << VWR_BITS << ", CORES_PER_PCH = " << CORES_PER_PCH << std::endl << std::endl;
    
    // Divide the matrix dimensions into tiles according to the hardware parameters
    // Initial check: can we multiply and do one complete addition layer given the IB / CSDRF sizes?
    uint MULOpsPerMulChunk = std::min(2 * WORDS_PER_VWR, (n/2)*2);      // MULs needed per multiplication chunk (if odd, round down to even)
    uint NOPOpsPerMulChunk = NOPCyclesPerMUL ? MULOpsPerMulChunk : 0;   // NOPs needed per multiplication chunk
    uint VMVOpsPerMulChunk = MULOpsPerMulChunk / 2;                     // VMVs needed per multiplication chunk
    uint ADDOpsPerMulChunk = MULOpsPerMulChunk / 2;                     // ADDs needed per multiplication chunk
    // PACK operations needed after first addition of the multiplication chunk
    uint PACKOpsPerMulChunk = div_ceil (ADDOpsPerMulChunk * WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in),
                                            WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out));
    PACKOpsPerMulChunk = (addTreeVector[0].sw_in == SWSIZE::B24) ? 0 : PACKOpsPerMulChunk; // No PACK operations if the initial subword size is 24 bits
    // Number of instructions per multiplication chunk
    uint instrPerMulChunk = 2 + MULOpsPerMulChunk + NOPOpsPerMulChunk +
                            VMVOpsPerMulChunk + ADDOpsPerMulChunk + PACKOpsPerMulChunk + 2;

    // Assess if we are limited by the CSDRF size. If so, update the parameters accordingly
    if (MULOpsPerMulChunk > CSD_ENTRIES) {
        mapMode = MAPMODE::CSDRF_CONSTRAINED;
        MULOpsPerMulChunk = CSD_ENTRIES;
        NOPOpsPerMulChunk = NOPCyclesPerMUL ? CSD_ENTRIES : 0;
        VMVOpsPerMulChunk = CSD_ENTRIES / 2;
        ADDOpsPerMulChunk = CSD_ENTRIES / 2;
        PACKOpsPerMulChunk = div_ceil (ADDOpsPerMulChunk * WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in),
                                            WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out));
        PACKOpsPerMulChunk = (addTreeVector[0].sw_in == SWSIZE::B24) ? 0 : PACKOpsPerMulChunk; // No PACK operations if the initial subword size is 24 bits
        instrPerMulChunk = 2 + MULOpsPerMulChunk + NOPOpsPerMulChunk +
                            VMVOpsPerMulChunk + ADDOpsPerMulChunk + PACKOpsPerMulChunk + 2;
    }

    // Assess if we are limited by the IB size (more constraining than the CSDRF size)
    // If so, update the parameters accordingly
    if (instrPerMulChunk > IB_ENTRIES) {
        mapMode = MAPMODE::IB_CONSTRAINED;
        // Tile according to the IB size, making sure we can fit x MULs and the corresponding ADDs in the IB
        MULOpsPerMulChunk = (IB_ENTRIES - 4) / 
                                ((NOPCyclesPerMUL ? 3 : 2) + div_ceil(WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in),
                                                                        WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out)));
        MULOpsPerMulChunk = (MULOpsPerMulChunk / 2) * 2;    // Make sure it is even
        NOPOpsPerMulChunk = NOPCyclesPerMUL ? MULOpsPerMulChunk : 0;
        VMVOpsPerMulChunk = MULOpsPerMulChunk / 2;
        ADDOpsPerMulChunk = MULOpsPerMulChunk / 2;
        PACKOpsPerMulChunk = div_ceil (ADDOpsPerMulChunk * WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in),
                                            WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out));
        PACKOpsPerMulChunk = (addTreeVector[0].sw_in == SWSIZE::B24) ? 0 : PACKOpsPerMulChunk; // No PACK operations if the initial subword size is 24 bits
        instrPerMulChunk = 2 + MULOpsPerMulChunk + NOPOpsPerMulChunk +
                            VMVOpsPerMulChunk + ADDOpsPerMulChunk + PACKOpsPerMulChunk + 2;
    }

    if (verbose) {
        std::cout << "IB size: " << IB_ENTRIES << ", CSDRF size: " << CSD_ENTRIES << std::endl;
        std::cout << "Mapping mode: " << MAPMODE_STRING.at(mapMode) << std::endl;
        std::cout << "Number of MUL operations per multiplication chunk: " << MULOpsPerMulChunk << std::endl;
        std::cout << "Number of NOP operations per multiplication chunk: " << NOPOpsPerMulChunk << std::endl;
        std::cout << "Number of VMV operations per multiplication chunk: " << VMVOpsPerMulChunk << std::endl;
        std::cout << "Number of ADD operations per multiplication chunk: " << ADDOpsPerMulChunk << std::endl;
        std::cout << "Number of PACK operations per multiplication chunk: " << PACKOpsPerMulChunk << std::endl;
        std::cout << "Number of instructions per multiplication chunk: " << instrPerMulChunk << std::endl;
    }

    // Compute loops
    uint ext_loops = n / MULOpsPerMulChunk;   // Number of external loops to cover n
    uint loops = MULOpsPerBRow;                // Number of loops to cover q
    uint ext_peeling = n % MULOpsPerMulChunk; // Number of peeling operations

    if (verbose) {
        std::cout << "Number of external loops to cover n: " << ext_loops << std::endl;
        std::cout << "Number of loops to cover q: " << loops << std::endl;
        std::cout << "Number of peeling operations: " << ext_peeling << std::endl;
    }

    // Variables to keep track of the current state of the assembly code generation
    uint curAddLayer = 0;       // Current layer of the adder tree reached
    uint curInstrCount = 0;     // Current instruction count
    uint addLayerReached = 0;   // Keeps track of the last layer reached in the external loop
    uint inputNextLayer = 0;    // Number of inputs to the next layer of the adder tree
    uint VMVOpsPerReducLayer;   // Number of VMV operations per reduced layer
    uint ADDOpsPerReducLayer;   // Number of ADD operations per reduced layer
    uint PACKOpsPerReducLayer;  // Number of PACK operations per reduced layer
    
    // Variables to help packing operations
    uint numSw;             // Number of subwords in the current layer
    uint swInPerWord;       // Number of input subwords per word
    uint swOutPerWord;      // Number of output subwords per word
    uint wordPkt;           // Number of words needed for an entire number of input subwords to be packed into an entire number of output subwords
    uint curWordPkt;        // Current index of the word being added
    uint swInPerR2R1;       // Number of input subwords per joint R2/R1 register
    uint curSwR1R2;         // Current index of the joint R2/R1 subword
    uint packVWR;           // Current VWR to pack into
    uint packVWRIdx;        // Current index of the VWR to pack into

    // Generate assembly code for multiplications, with the hardware loop covering q, then an external loop covering n, and all for m times

    assembly << "WRF CSD_LEN " << csd_len << std::endl << std::endl;

    if (ext_loops) {
        swInPerWord = WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in);
        swOutPerWord = WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out);
        wordPkt = lcm(swInPerWord, swOutPerWord) / swInPerWord;
        curWordPkt = 0;
        swInPerR2R1 = 2 * swInPerWord;
        curSwR1R2 = 0;

        packVWR = 0;
        packVWRIdx = 0;

        assembly << "WRF IB0" << std::endl;
        assembly << "RLB AddrFile VWR_0 ALL_WORDS DataFile" << std::endl;
        assembly << "RLB AddrFile VWR_1 ALL_WORDS DataFile" << std::endl;
        for (uint i = 0; i < MULOpsPerMulChunk/2; i++) {
            assembly << "VFUX MUL LEN_" << swsize_to_uint(addTreeVector[0].sw_in) << " CSD" << i << " OUT_VWR VWR_0[" << i << "] VWR_0[" << i << "]" << std::endl;
            if (NOPCyclesPerMUL) { assembly << "NOP " << NOPCyclesPerMUL << std::endl; }
        }
        for (uint i = 0; i < MULOpsPerMulChunk/2; i++) {
            assembly << "VFUX MUL LEN_" << swsize_to_uint(addTreeVector[0].sw_in) << " CSD" << i+MULOpsPerMulChunk/2 << " OUT_R3 VWR_1[" << i << "]" << std::endl;
            if (NOPCyclesPerMUL) { assembly << "NOP " << NOPCyclesPerMUL << std::endl; }
            assembly << "VMV VWR_0[" << i << "]" << std::endl;
            assembly << "VFUX ADD LEN_" << swsize_to_uint(addTreeVector[0].sw_in) << " SRC_R3 OUT_R" << (i % 2 ? "2" : "1") << std::endl;   // Alternate between R1 and R2
            if (PACKOpsPerMulChunk) {
                curWordPkt++;
                assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[0].sw_in) << "_" << swsize_to_uint(addTreeVector[0].sw_out) << " "\
                        << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                    packVWR = 0;
                    packVWRIdx++;
                }
                if (curWordPkt == wordPkt) { // After this addition, that completes the wordPkt two packs are needed
                    assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[0].sw_in) << "_" << swsize_to_uint(addTreeVector[0].sw_out) << " "\
                            << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                    curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                    if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                        packVWR = 0;
                        packVWRIdx++;
                    }
                    curWordPkt = 0; // Word packet is complete, reset the counter
                } 
            }
        }
        // In case there are subwords left to pack in R2|R1, pack them
        if (curWordPkt && PACKOpsPerMulChunk) {
            assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[0].sw_in) << "_" << swsize_to_uint(addTreeVector[0].sw_out) << " "\
                    << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
        }

        curInstrCount = instrPerMulChunk - 2;
        curAddLayer++;
        addLayerReached = curAddLayer;
        ADDOpsPerReducLayer = ADDOpsPerMulChunk;
        PACKOpsPerReducLayer = PACKOpsPerMulChunk;

        numSw = swInPerWord * ADDOpsPerMulChunk;
        
        // If not constrained by the IB or the CSDRF sizes, continue the adder tree until possible
        if (mapMode == MAPMODE::UNCONSTRAINED) {    
            generate_add_reduction_assembly(curAddLayer, curInstrCount, numSw, swInPerWord,
                                            addTreeLayerTotal, addTreeVector, assembly, 0, verbose);
            addLayerReached = curAddLayer;
        }
        inputNextLayer = (numSw / swInPerWord) * ext_loops; // Track the output of the adder tree for the next layer

        // Add WLBs, and NOP 0 if needed
        assembly << "WLB AddrFile VWR_0 ALL_WORDS" << std::endl;
        assembly << "WLB AddrFile VWR_1 ALL_WORDS" << std::endl;
        curInstrCount += 2;
        if (curInstrCount < IB_ENTRIES) {
            assembly << "NOP 0" << std::endl;
        }

        // Program hardware loop, CSD registers and trigger executions
        assembly << std::endl << "LOOP 0 " << curInstrCount-1 << " " << loops << std::endl << std::endl;
        for (uint i = 0; i < m*ext_loops; i++) {
            for (uint j = 0; j < MULOpsPerMulChunk; j++) {
                assembly << "WRF CSD" << j << " DataFile" << std::endl; 
                dataFile << std::hex << std::showbase << generate_random_csd(csd_len, gen) << std::endl;
            }
            assembly << "EXEC" << std::endl << std::endl;
            for (uint j = 0; j < loops; j++) {
                write_random_vwr(swsize_to_uint(addTreeVector[0].sw_in), dataFile, gen);
                write_random_vwr(swsize_to_uint(addTreeVector[0].sw_in), dataFile, gen);
                addrFile << std::hex << std::showbase << nxt_addr(channel, inputRow, inputCol) << std::endl << nxt_addr(channel, inputRow, inputCol) << std::endl;
                addrFile << std::hex << std::showbase << nxt_addr(channel, outputRow, outputCol) << std::endl << nxt_addr(channel, outputRow, outputCol) << std::endl;
            }
        }

    }
    
    if (ext_peeling) {  // External peeling
        assembly << std::endl << "; External loop complete, starting loop peeling" << std::endl;

        uint MULOpsPeeling = 2 * (ext_peeling / 2);                 // MULs needed per peeling chunk
        uint NOPOpsPeeling = NOPCyclesPerMUL ? MULOpsPeeling : 0;   // NOPs needed per peeling chunk
        uint VMVOpsPeeling = ext_peeling / 2;                       // VMVs needed per peeling chunk
        uint ADDOpsPeeling = ext_peeling / 2;                       // ADDs needed per peeling chunk
        // PACK operations needed after first addition of the peeling chunk
        uint PACKOpsPeeling = div_ceil ((ext_peeling / 2) * WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in),
                                                WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out));
        PACKOpsPeeling = (addTreeVector[0].sw_in == SWSIZE::B24) ? 0 : PACKOpsPeeling; // No PACK operations if the initial subword size is 24 bits
        // Number of instructions per peeling chunk
        uint instrPeeling = 2 + MULOpsPeeling + NOPOpsPeeling
                                + VMVOpsPeeling + ADDOpsPeeling + PACKOpsPeeling + 2;

        swInPerWord = WORD_BITS / swsize_to_uint(addTreeVector[0].sw_in);
        swOutPerWord = WORD_BITS / swsize_to_uint(addTreeVector[0].sw_out);
        wordPkt = lcm(swInPerWord, swOutPerWord) / swInPerWord;
        curWordPkt = 0;
        swInPerR2R1 = 2 * swInPerWord;
        curSwR1R2 = 0;

        packVWR = 0;
        packVWRIdx = 0;

        assembly << "WRF IB0" << std::endl;
        assembly << "RLB AddrFile VWR_0 ALL_WORDS DataFile" << std::endl;
        assembly << "RLB AddrFile VWR_1 ALL_WORDS DataFile" << std::endl;
        for (uint i = 0; i < ext_peeling/2; i++) {
            assembly << "VFUX MUL LEN_" << swsize_to_uint(addTreeVector[0].sw_in) << " CSD" << i << " OUT_VWR VWR_0[" << i << "] VWR_0[" << i << "]" << std::endl;
            if (NOPCyclesPerMUL) { assembly << "NOP " << NOPCyclesPerMUL << std::endl; }
        }
        for (uint i = 0; i < ext_peeling/2; i++) {
            assembly << "VFUX MUL LEN_" << swsize_to_uint(addTreeVector[0].sw_in) << " CSD" << i+ext_peeling/2 << " OUT_R3 VWR_1[" << i << "]" << std::endl;
            if (NOPCyclesPerMUL) { assembly << "NOP " << NOPCyclesPerMUL << std::endl; }
            assembly << "VMV VWR_0[" << i << "]" << std::endl;
            assembly << "VFUX ADD LEN_" << swsize_to_uint(addTreeVector[0].sw_in) << " SRC_R3 OUT_R" << (i % 2 ? "2" : "1") << std::endl;   // Alternate between R1 and R2
            if (PACKOpsPeeling) {
                curWordPkt++;
                assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[0].sw_in) << "_" << swsize_to_uint(addTreeVector[0].sw_out) << " "\
                        << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                    packVWR = 0;
                    packVWRIdx++;
                }
                if (curWordPkt == wordPkt) { // After this addition, that completes the wordPkt two packs are needed
                    assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[0].sw_in) << "_" << swsize_to_uint(addTreeVector[0].sw_out) << " "\
                            << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
                    curSwR1R2 = (curSwR1R2 + swOutPerWord) % swInPerR2R1;   // Next R1/R2 subword index
                    if (++packVWR >= 2) { // Store alternate VWRs, then increment the index
                        packVWR = 0;
                        packVWRIdx++;
                    }
                    curWordPkt = 0; // Word packet is complete, reset the counter
                } 
            }
        }
        // In case there are subwords left to pack in R2|R1, pack them
        if (curWordPkt && PACKOpsPeeling) {
            assembly << "PACK CHANGE_" << swsize_to_uint(addTreeVector[0].sw_in) << "_" << swsize_to_uint(addTreeVector[0].sw_out) << " "\
                    << curSwR1R2 << " OUT_VWR VWR_" << packVWR << "[" << packVWRIdx << "]" << std::endl;
        }

        curInstrCount = instrPeeling - 2;
        curAddLayer = 1;
        ADDOpsPerReducLayer = ADDOpsPeeling;
        PACKOpsPerReducLayer = PACKOpsPeeling;

        numSw = swInPerWord * ADDOpsPeeling;

        if (mapMode == MAPMODE::UNCONSTRAINED) {
            generate_add_reduction_assembly(curAddLayer, curInstrCount, numSw, swInPerWord,
                                            addTreeLayerTotal, addTreeVector, assembly, addLayerReached, verbose);
        }
        inputNextLayer += numSw / swInPerWord;  // Track the output of the adder tree for the next layer

        // Add WLBs, and NOP 0 if needed
        assembly << "WLB AddrFile VWR_0 ALL_WORDS" << std::endl;
        assembly << "WLB AddrFile VWR_1 ALL_WORDS" << std::endl;
        curInstrCount += 2;
        if (curInstrCount < IB_ENTRIES) {
            assembly << "NOP 0" << std::endl;
        }

        // Program hardware loop, CSD registers and trigger executions
        assembly << std::endl << "LOOP 0 " << curInstrCount-1 << " " << loops << std::endl << std::endl;
        for (uint i = 0; i < m; i++) {
            for (uint j = 0; j < MULOpsPeeling; j++) {
                assembly << "WRF CSD" << j << " DataFile" << std::endl; 
                dataFile << std::hex << std::showbase << generate_random_csd(csd_len, gen) << std::endl;
            }
            assembly << "EXEC" << std::endl << std::endl;
            for (uint j = 0; j < loops; j++) {
                write_random_vwr(swsize_to_uint(addTreeVector[0].sw_in), dataFile, gen);
                write_random_vwr(swsize_to_uint(addTreeVector[0].sw_in), dataFile, gen);
                addrFile << std::hex << std::showbase << nxt_addr(channel, inputRow, inputCol) << std::endl << nxt_addr(channel, inputRow, inputCol) << std::endl;
                addrFile << std::hex << std::showbase << nxt_addr(channel, outputRow, outputCol) << std::endl << nxt_addr(channel, outputRow, outputCol) << std::endl;
            }
        }
    }

    // Continue the adder tree until the last layer is reached. An internal loop covers q, then an external loop covers m, and a while loop until the last layer is reached
    assembly << "; Multiplication complete, starting ADD reduction" << std::endl;
    while (addLayerReached < addTreeLayerTotal) {

        swInPerWord = WORD_BITS / swsize_to_uint(addTreeVector[addLayerReached].sw_in);

        loops = div_ceil(q, swInPerWord * CORES_PER_PCH); // Number of loops to cover q

        assembly << "WRF IB0" << std::endl;
        assembly << "RLB AddrFile VWR_0 ALL_WORDS DataFile" << std::endl;
        assembly << "RLB AddrFile VWR_1 ALL_WORDS DataFile" << std::endl;

        curInstrCount = 2;
        curAddLayer = addLayerReached;
        if (addTreeVector[curAddLayer].sw_change != SWREPACK::INV) {
            VMVOpsPerReducLayer = std::min(WORDS_PER_VWR,
                                            (IB_ENTRIES - 4) / (int) (2 + div_ceil(WORD_BITS / swsize_to_uint(addTreeVector[curAddLayer].sw_in),
                                                                                    WORD_BITS / swsize_to_uint(addTreeVector[curAddLayer].sw_out))));
        } else {
            VMVOpsPerReducLayer = std::min(WORDS_PER_VWR, (IB_ENTRIES - 4) / 2);
        }
        VMVOpsPerReducLayer = std::min(VMVOpsPerReducLayer, inputNextLayer / 2);  // Limit to the number of additions
        VMVOpsPerReducLayer = std::max(VMVOpsPerReducLayer, 1U); // At least one addition
        ADDOpsPerReducLayer = VMVOpsPerReducLayer;
        PACKOpsPerReducLayer = 0;
        numSw = swInPerWord * VMVOpsPerReducLayer * 2;  // Number of subwords in the current layer
        uint loopsReducLayer = div_ceil(inputNextLayer, 2 * VMVOpsPerReducLayer);

        // Generate assembly code for the next layer
        generate_add_reduction_assembly(curAddLayer, curInstrCount, numSw, swInPerWord,
                                        addTreeLayerTotal, addTreeVector, assembly, 0, verbose);
        inputNextLayer = (numSw / swInPerWord) * loopsReducLayer;   // Track the output of the adder tree for the next layer

        assembly << "WLB AddrFile VWR_0 ALL_WORDS" << std::endl;
        assembly << "WLB AddrFile VWR_1 ALL_WORDS" << std::endl;
        curInstrCount += 2;
        if (curInstrCount < IB_ENTRIES) {
            assembly << "NOP 0" << std::endl;
        }

        // Program hardware loop and trigger executions
        assembly << std::endl << "LOOP 0 " << curInstrCount-1 << " " << loops << std::endl << std::endl;
        for (uint i = 0; i < m*loopsReducLayer; i++) {
            assembly << "EXEC" << std::endl;
            for (uint j = 0; j < loops; j++) {
                write_random_vwr(swsize_to_uint(addTreeVector[addLayerReached].sw_in), dataFile, gen);
                write_random_vwr(swsize_to_uint(addTreeVector[addLayerReached].sw_in), dataFile, gen);
                addrFile << std::hex << std::showbase << nxt_addr(channel, outputRow, outputCol) << std::endl << nxt_addr(channel, outputCol, outputCol) << std::endl;
                addrFile << std::hex << std::showbase << nxt_addr(channel, outputRow, outputCol) << std::endl << nxt_addr(channel, outputRow, outputCol) << std::endl;
            }
        }
        assembly << std::endl;
        addLayerReached = curAddLayer;  // Update after data is written to employ the correct adder tree layer
    }

    // Predicted adder tree layers are completed, but they are not enough to cover the entire output
    if (inputNextLayer > 1) {
        assembly << "; Final ADD reduction, exceeding predicted number of tree layers" << std::endl;
        swInPerWord = WORD_BITS / swsize_to_uint(addTreeVector[addLayerReached-1].sw_out);
        loops = div_ceil(q, swInPerWord * CORES_PER_PCH); // Number of loops to cover q

        assembly << "WRF IB0" << std::endl;
        assembly << "RLB AddrFile VWR_0 ALL_WORDS DataFile" << std::endl;
        assembly << "RLB AddrFile VWR_1 ALL_WORDS DataFile" << std::endl;

        curInstrCount = 2;
        VMVOpsPerReducLayer = inputNextLayer / 2;  // Limit to the number of additions
        ADDOpsPerReducLayer = VMVOpsPerReducLayer;
        numSw = swInPerWord * VMVOpsPerReducLayer * 2;  // Number of subwords in the current layer

        while (inputNextLayer > 1) {
            VMVOpsPerReducLayer = inputNextLayer / 2;  // Limit to the number of additions
            ADDOpsPerReducLayer = VMVOpsPerReducLayer;
            numSw = swInPerWord * VMVOpsPerReducLayer * 2;  // Number of subwords in the current layer

            for (uint i = 0; i < VMVOpsPerReducLayer; i++) {
                assembly << "VMV VWR_0[" << i << "]" << std::endl;
                assembly << "VFUX ADD LEN_" << swsize_to_uint(addTreeVector[curAddLayer-1].sw_out) << " SRC_VWR OUT_";
                assembly << "VWR VWR_1[" << i << "] VWR_0" << "[" << i << "]" << std::endl;
            }

            curInstrCount += VMVOpsPerReducLayer + ADDOpsPerReducLayer;
            inputNextLayer = div_ceil(inputNextLayer, 2);

            if (verbose) {
                std::cout << "Current instruction count: " << curInstrCount << ", inputNextLayer: " << inputNextLayer << std::endl;
            }
        }

        assembly << "WLB AddrFile VWR_0 ALL_WORDS" << std::endl;
        curInstrCount += 1;
        if (curInstrCount < IB_ENTRIES) {
            assembly << "NOP 0" << std::endl;
        }

        // Program hardware loop and trigger executions
        assembly << std::endl << "LOOP 0 " << curInstrCount-1 << " " << loops << std::endl << std::endl;
        for (uint i = 0; i < m; i++) {
            assembly << "EXEC" << std::endl;
            for (uint j = 0; j < loops; j++) {
                write_random_vwr(swsize_to_uint(addTreeVector[addLayerReached-1].sw_out), dataFile, gen);
                write_random_vwr(swsize_to_uint(addTreeVector[addLayerReached-1].sw_out), dataFile, gen);
                addrFile << std::hex << std::showbase << nxt_addr(channel, outputRow, outputCol) << std::endl << nxt_addr(channel, outputCol, outputCol) << std::endl;
                addrFile << std::hex << std::showbase << nxt_addr(channel, outputRow, outputCol) << std::endl;
            }
        }
    }

    return;
}

int main (int argc, char **argv) {
    // Parse the command line arguments
    int m, n, q, sw_bw, csd_len, dram_intvl;
    std::string output;
    bool verbose = false;
    parse_args(argc, argv, m, n, q, sw_bw, csd_len, dram_intvl, output, verbose);

    // Open the output files
    std::ofstream assembly("INPUTS_DIR/assembly-input/" + output + ".asm");    // Assembly file
    std::ofstream dataFile("INPUTS_DIR/data-input/" + output + ".data");       // Data file
    std::ofstream addrFile("INPUTS_DIR/address-input/" + output + ".addr");    // Address file
    
    std::mt19937 gen(0);    // Standard mersenne_twister_engine seeded

    std::cout << "Generating assembly code for GEMM operation with dimensions: " << m << "x" << n << " and " << n << "x" << q;
    std::cout << ", with initial subword bitwidth of " << sw_bw;
    std::cout << ", CSD operand length of " << csd_len;
    std::cout << ", and minimum interval between consecutive DRAM commands at the column of " << dram_intvl << std::endl;

    // Generate the assembly code
    map_gemm(assembly, dataFile, addrFile, m, n, q, sw_bw, csd_len, dram_intvl, verbose, gen);

    // Close the output files
    assembly.close();
    dataFile.close();
    addrFile.close();

    return 0;
}