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

# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_DIR is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_HOST is the gnu cross target
set -e
OCPI_LIQUID_VERSION=v1.3.1
dir=liquid-dsp
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-install.sh \
       "$1" \
       liquid \
       "DSP Math Library" \
       $OCPI_LIQUID_VERSION \
       https://github.com/jgaeddert/liquid-dsp.git \
       $dir \
       1
SHARED=yes
SEDINPLACE="sed --in-place"
if test "$OCPI_TARGET_OS" = macos; then
 crossConfig="CC=cc CXX=c++"
 SEDINPLACE='sed -i'
elif test "$OCPI_CROSS_HOST" != ""; then
 export PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
 crossConfig="CC=$OCPI_CROSS_HOST-gcc CXX=$OCPI_CROSS_HOST-g++ --host=$OCPI_CROSS_HOST"
# SHARED=no
fi
#generators="\
#./src/fec/gentab/reverse_byte_gentab \
#./src/utility/gentab/count_ones_gentab \
#"
# [ $OCPI_TOOL_PLATFORM != $OCPI_TARGET_PLATFORM -a ! -f $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TOOL_DIR/bin/reverse_byte_gentab ] && {
#   echo It appears that you have not built the liquid library for $OCPI_TOOL_PLATFORM yet.
#   echo This is required before trying to build this library for $OCPI_TARGET_PLATFORM, on $OCPI_TOOL_PLATFORM.
#   exit 1
# }
(echo Performing '"./bootstrap.sh"' on git repo; cd ..; ./bootstrap.sh)
base=$(basename `pwd`)
echo Copying git repo for building in `pwd`
(cd ..; cp -R $(ls . | grep -v ocpi-build-) $base)
[ $OCPI_TOOL_PLATFORM != $OCPI_TARGET_PLATFORM ] && {
  for g in $generators; do
   cp $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TOOL_DIR/bin/$(basename $g) $(dirname $g)
  done
}
# patches to ./configure to not run afoul of macos stronger error checking
$SEDINPLACE -e 's/char malloc, realloc, free, memset,/char malloc(), realloc(), free(), memset(),/' ./configure
$SEDINPLACE -e 's/char sinf, cosf, expf, cargf, cexpf, crealf, cimagf,/char sinf(), cosf(), expf(), cargf(), cexpf(), crealf(), cimagf(),/' ./configure
$SEDINPLACE -e '/rpl_malloc/d' ./configure
$SEDINPLACE -e '/rpl_realloc/d' ./configure
./configure  \
  $crossConfig \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/liquid \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TARGET_DIR \
  --includedir=$OCPI_PREREQUISITES_INSTALL_DIR/liquid/include \
  CFLAGS=-g CXXFLAGS=-g
make
make install
# the recommendations from liquidsdr.org is to use #include "liquid/liquid.h" code per Aaron
# mv $OCPI_PREREQUISITES_INSTALL_DIR/liquid/include/liquid/* $OCPI_PREREQUISITES_INSTALL_DIR/liquid/include
# rmdir $OCPI_PREREQUISITES_INSTALL_DIR/liquid/include/liquid
[ $OCPI_TOOL_PLATFORM = $OCPI_TARGET_PLATFORM ] && {
  mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TARGET_DIR/bin
  for g in $generators; do
    cp $g $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TARGET_DIR/bin
  done
}
exit 0
