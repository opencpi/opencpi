#!/bin/sh
echo "Icarus needs autoconf version > 2.59 and < 2.68, both of which don't work."
echo "It will work successfully with 2.61 (what comes on MacOS 10.6)."
echo "Icarus also needs gperf. 3.0.3 works (what comes on MacOs 10.6)
echo "Both are available from gnu.org, with: ./configure;make;sudo make install"
set -ev
. `dirname $0`/set_target.sh
echo Target: $HostTarget
mkdir -p /opt/opencpi/prerequisites
cd /opt/opencpi/prerequisites
git clone git://icarus.com/~steve-icarus/verilog
mv verilog icarus
cd icarus
git checkout v0_9_3b
sh autoconf.sh
PREFIX=`pwd`/$HostTarget
./configure --prefix=$PREFIX
make
make install
ln -s /opt/opencpi/prerequisites/icarus/$HostTarget /opt/opencpi/$HostTarget/prerequisites/icarus


