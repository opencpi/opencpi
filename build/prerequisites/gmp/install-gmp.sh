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

version=6.1.2
dir=gmp-$version
# This server is unavailable:       https://ftp.gnu.org/gnu/gmp
# Since we don't look at multiple URLs/mirrors (yet)
# The one below is one of the advertised mirrors
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-prerequisite.sh \
       "$1" \
       gmp \
       "Extended Precision Numeric library" \
       https://mirror.csclub.uwaterloo.ca/gnu/gmp \
       $dir.tar.xz \
       $dir \
       1
../configure ${OcpiCrossHost:+--host=$OcpiCrossHost} \
  --prefix=$OcpiInstallDir --exec-prefix=$OcpiInstallExecDir \
  --enable-fat=yes --enable-cxx=yes --with-pic
#=gmp \
#  CFLAGS='-g -fPIC' CXXFLAGS='-g -fPIC' # why doesn't --with-pic do this?
make && make install
rm -f $OcpiInstallExecDir/lib/*.la
