#!/bin/sh
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

echo "Icarus needs autoconf version > 2.59 and < 2.68, both of which don't work."
echo "It will work successfully with 2.61 (what comes on MacOS 10.6)."
echo "Icarus also needs gperf. 3.0.3 works (what comes on MacOs 10.6)"
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


