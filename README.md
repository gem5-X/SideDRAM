# SideDRAM
 This repository contains the files needed to model the bank-level Compute-near-Memory architecture SideDRAM.

The initial release is described in the following paper:
>R. Medina et al.
>"[**SideDRAM: Integrating SoftSIMD Datapaths near DRAM Banks for Energy-Efficient Variable Precision Computation**](https://infoscience.epfl.ch/handle/20.500.14299/251422)".
>In ESWEEK - CODES+ISSS, October 2025.

## Dependencies

- GCC compiler supporting C++17
- CMake >= 3.5

## Execution

The base exploration can be executed using the [run.sh](./run.sh) script, which will set the environment variables, install the SystemC and ramulator dependencies, compiler the programming interface and the SystemC model, and run an exploration for the execution of Matrix-Vector Multiplications with different sizes and initial quantizations. A containerized execution can also be run with [run_docker.sh](./run_docker.sh).

Additional experiments to explore more SideDRAM configurations and GEMMs can be executed with the additional scripts in [scripts](./scripts/)

The environment variables can be modified at [export_paths.sh](./scripts/export_paths.sh).

The obtained cycle results are found at the [stats](./stats/) folder, and more detailed execution metrics at [inputs/recording](./inputs/recording/) if `EN_MODEL` is defined as `1` at [defs.h](./src/defs.h). In this case, `VCD_TRACE` should be defined as `0`.

## Project structure

- 📁 [**build**:](./build/) build folder.
- 📁 [**inputs**:](./inputs/) files and traces employed for the programming interface.
    - 📁 [**recording**:](./inputs/recording/) detailed simulation statistics. 
    - 📁 [**src**:](./inputs/src/) source files of the programming interface. 
- 📁 [**ramulator_files**:](./ramulator_files/) patched files to support all-bank DRAM mode.
- 📁 [**scripts**:](./scripts/) bash scripts to run parameterized explorations.
- 📁 [**src**:](./src/) source files of SystemC template.
- 📁 [**stats**:](./stats/) simulation statistics.
- 📁 [**waveforms**:](./waveforms/) GTKWave format files to analyze VCD waveforms.