#!/bin/bash

cd $SIDEDRAM_HOME

git clone https://github.com/CMU-SAFARI/ramulator.git
cp ramulator_files/src/* ramulator/src/
cp ramulator_files/configs/* ramulator/configs/
cd ramulator
make CXX=g++ -j $(nproc)

cd ..