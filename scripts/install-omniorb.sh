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

# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_DIR is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_TARGET is the gnu cross target
set -e
OCPI_OMNIORB_VERSION=4.2.0
. ./scripts/setup-install.sh
mkdir -p omniorb
cd omniorb
echo Building omniorb in `pwd` for $OCPI_TARGET_DIR
sudo rm -r -f omniorb* $OCPI_TARGET_DIR omniORB* include lib share etc
curl -O -L http://downloads.sourceforge.net/project/omniorb/omniORB/omniORB-$OCPI_OMNIORB_VERSION/omniORB-$OCPI_OMNIORB_VERSION.tar.bz2
#cp ../omniORB-$OCPI_OMNIORB_VERSION.tar.bz2 .
tar xf omniORB-$OCPI_OMNIORB_VERSION.tar.bz2
cd omniORB-$OCPI_OMNIORB_VERSION
# From here on is per target
mkdir build-$OCPI_TARGET_DIR
cd build-$OCPI_TARGET_DIR
if test "$OCPI_CROSS_HOST" != ""; then
 export PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
 crossConfig="CC=$OCPI_CROSS_HOST-gcc CXX=$OCPI_CROSS_HOST-g++ --host=$OCPI_CROSS_HOST"
fi
../configure  \
  $crossConfig \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_DIR \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_DIR \
  --with-omniORB-config=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc/omniORB.cfg \
  --with-omniNames-logdir=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/logs \
  --without-openssl \
  --oldincludedir=/tt \
  CFLAGS=-g CXXFLAGS=-g
export LD_RUN_PATH=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_DIR/lib
if test "$OCPI_CROSS_HOST" != ""; then
  make CC=gcc -C src/tool/omniidl/cxx/cccp
  make CXX=g++ -C src/tool/omniidl/cxx
  make CC=gcc -C src/tool/omkdepend
  unset LD_RUN_PATH
fi
make
make install
if test $OCPI_PREREQUISITES_INSTALL_DIR != $OCPI_PREREQUISITES_BUILD_DIR; then
   rm -r -f $OCPI_PREREQUISITES_INSTALL_DIR/omniorb
fi
#mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_DIR
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/logs
make install
sed < ../sample.cfg 's/^#.*InitRef\(.*\)my.host.name.*/InitRef\1localhost/' > omniORB.cfg
cp omniORB.cfg $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc
echo ============= omniorb for $OCPI_TARGET_PLATFORM built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_DIR
