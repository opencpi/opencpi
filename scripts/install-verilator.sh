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
