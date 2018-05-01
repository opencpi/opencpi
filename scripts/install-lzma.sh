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

OCPI_LZMA_VERSION=5.2.2
dir=xz-$OCPI_LZMA_VERSION
[ -z "$OCPI_CDK_DIR" ] && echo Environment variable OCPI_CDK_DIR not set && exit 1
source $OCPI_CDK_DIR/scripts/setup-install.sh \
       "$1" \
       lzma \
       "LZMA compression library" \
       $dir.tar.gz \
       https://github.com/xz-mirror/xz/releases/download/v$OCPI_LZMA_VERSION \
       $dir \
       1
../configure  \
  ${OCPI_CROSS_HOST+--host=${OCPI_CROSS_HOST}} \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/lzma \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/lzma/$OCPI_TARGET_DIR \
  --enable-shared=yes --enable-static --disable-symbol-versions \
  --disable-xz --disable-xzdec --disable-lzmadec --disable-lzmainfo --disable-lzma-links \
  --disable-scripts --disable-doc \
  --with-pic=liblzma \
  CFLAGS="-g -fPIC" CXXFLAGS="-g -fPIC"
make
make install
