#!/bin/bash --noprofile
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

liquid_version=v1.3.1
dir=liquid-dsp
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       liquid \
       "DSP Math Library" \
       https://github.com/jgaeddert/liquid-dsp.git \
       $liquid_version \
       $dir \
       1

echo Performing '"./bootstrap.sh"' on git repo
(cd ..; ./bootstrap.sh)
base=$(basename `pwd`)
echo Copying git repo for building in `pwd`
(cd ..; cp -R $(ls . | grep -v ocpi-build-) $base)
# patches to ./configure to not run afoul of macos stronger error checking
ed configure <<-EOF
	g/char malloc, realloc, free, memset,/s//char malloc(), realloc(), free(), memset(),/
	g/char sinf, cosf, expf, cargf, cexpf, crealf, cimagf,/s//char sinf(), cosf(), expf(), cargf(), cexpf(), crealf(), cimagf(),/
	g/rpl_malloc/d
	g/rpl_realloc/d
	w
EOF
./configure  \
  ${cross_host+--host=$cross_host} \
  --prefix=$install_dir --exec-prefix=$install_exec_dir \
  --includedir=$install_dir/include \
  CFLAGS=-g CXXFLAGS=-g
make
make install
exit 0
