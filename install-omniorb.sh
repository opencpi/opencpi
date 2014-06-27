#!/bin/sh
# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_HOST is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_TARGET is the gnu cross target
set -e
OCPI_OMNIORB_VERSION=4.2.0
. ./setup_install.sh
mkdir -p omniorb
cd omniorb
echo Building omniorb in `pwd` for $OCPI_TARGET_HOST
sudo rm -r -f omniorb* $OCPI_TARGET_HOST omniORB* include lib share etc
curl -O -L http://downloads.sourceforge.net/project/omniorb/omniORB/omniORB-$OCPI_OMNIORB_VERSION/omniORB-$OCPI_OMNIORB_VERSION.tar.bz2
#cp ../omniORB-$OCPI_OMNIORB_VERSION.tar.bz2 .
tar xf omniORB-$OCPI_OMNIORB_VERSION.tar.bz2
cd omniORB-$OCPI_OMNIORB_VERSION
# From here on is per target
mkdir build-$OCPI_TARGET_HOST
cd build-$OCPI_TARGET_HOST
if test "$OCPI_CROSS_HOST" != ""; then
 export PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
 crossConfig="CC=$OCPI_CROSS_HOST-gcc CXX=$OCPI_CROSS_HOST-g++ --host=$OCPI_CROSS_HOST"
fi
../configure  \
  $crossConfig \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_HOST \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_HOST \
  --with-omniORB-config=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc/omniORB.cfg \
  --with-omniNames-logdir=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/logs \
  --without-openssl \
  --oldincludedir=/tt \
  CFLAGS=-g CXXFLAGS=-g
export LD_RUN_PATH=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_HOST/lib
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
#mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_HOST
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/logs
make install
sed < ../sample.cfg 's/^#.*InitRef\(.*\)my.host.name.*/InitRef\1localhost/' > omniORB.cfg
cp omniORB.cfg $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc
echo ============= omniorb for $OCPI_TARGET_HOST built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_TARGET_HOST
