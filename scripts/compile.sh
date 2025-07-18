#!/bin/bash

cd $SIDEDRAM_HOME

cd inputs
./compile_all.sh
make -C ../Debug/ all -j32
cd ..