#!/bin/sh
set -ev
. `dirname $0`/set_target.sh
echo Target: $HostTarget
mkdir -p /opt/opencpi/prerequisites
cd /opt/opencpi/prerequisites
git clone http://git.veripool.org/git/verilator
cd verilator
git checkout verilator_3_830
autoconf
PREFIX=`pwd`/$HostTarget
./configure --prefix=$PREFIX
make
make test
make install
ln -s /opt/opencpi/prerequisites/verilator/$HostTarget /opt/opencpi/$HostTarget/prerequisites/verilator

