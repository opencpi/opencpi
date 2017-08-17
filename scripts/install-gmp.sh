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

OCPI_GMP_VERSION=6.1.1
dir=gmp-$OCPI_GMP_VERSION
source ./scripts/setup-install.sh \
       "$1" \
       gmp \
       $dir.tar.xz \
       https://ftp.gnu.org/gnu/gmp \
       $dir \
       1

../configure  \
  ${OCPI_CROSS_HOST+--host=${OCPI_CROSS_HOST}} \
  --enable-cxx=yes \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gmp \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gmp/$OCPI_TARGET_DIR \
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= gmp for $OCPI_TARGET_PLATFORM built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/gmp/$OCPI_TARGET_DIR
