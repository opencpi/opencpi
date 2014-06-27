#!/bin/sh
set -e
OCPI_VERILATOR_VERSION=3_830
. setup_install.sh
mkdir -p verilator
cd verilator
rm -r -f verilator* $OCPI_BUILD_HOST 
mkdir verilator-$OCPI_VERILATOR_VERSION
cd verilator-$OCPI_VERILATOR_VERSION
git clone http://git.veripool.org/git/verilator
cd verilator
git checkout verilator_$OCPI_VERILATOR_VERSION
autoconf
./configure --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/verilator/$OCPI_BUILD_HOST
make
make test
make install
