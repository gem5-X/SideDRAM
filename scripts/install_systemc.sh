#!/bin/bash

cd $SIDEDRAM_HOME

wget https://github.com/accellera-official/systemc/archive/refs/tags/3.0.0.tar.gz
tar -xzf 3.0.0.tar.gz
cd systemc-3.0.0
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=$SYSTEMC_HOME -DCMAKE_CXX_STANDARD=17 ..
make install -j $(nproc)
cd ../..
rm -rf systemc-3.0.0 3.0.0.tar.gz