#!/bin/bash

source scripts/export_paths.sh

# Install systemc only if not installed
if [ ! -d "$SYSTEMC_HOME" ]; then
    ./scripts/install_systemc.sh
fi

# Install ramulator only if not installed
if [ ! -f "$RAMULATOR_DIR/ramulator" ]; then
    ./scripts/install_ramulator.sh
fi

./scripts/compile.sh
./scripts/base_exploration.sh