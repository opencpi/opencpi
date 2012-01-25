#!/bin/sh
set -e
OCPI_OMNIORB_VERSION=4.1.5
. setup_install.sh
mkdir -p omniorb
cd omniorb
sudo rm -r -f omniorb* $OCPI_BUILD_TARGET omniORB* include lib share etc
curl -O http://iweb.dl.sourceforge.net/project/omniorb/omniORB/omniORB-$OCPI_OMNIORB_VERSION/omniORB-$OCPI_OMNIORB_VERSION.tar.bz2
tar xf omniORB-$OCPI_OMNIORB_VERSION.tar.bz2
cd omniORB-$OCPI_OMNIORB_VERSION
# From here on is per target
mkdir build-$OCPI_BUILD_TARGET
cd build-$OCPI_BUILD_TARGET
../configure  \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_BUILD_TARGET \
  --with-omniORB-config=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc/omniORB.cfg \
  --with-omniNames-logdir=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/logs \
  --without-openssl \
  CFLAGS=-g CXXFLAGS=-g
export LD_RUN_PATH=$OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_BUILD_TARGET/lib
make
if test $OCPI_PREREQUISITES_INSTALL_DIR != $OCPI_PREREQUISITES_BUILD_DIR; then
   rm -r -f $OCPI_PREREQUISITES_INSTALL_DIR/omniorb
fi
#mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/$OCPI_BUILD_TARGET
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/logs
make install
sed < ../sample.cfg 's/^#.*InitRef\(.*\)my.host.name.*/InitRef\1localhost/' > omniORB.cfg
cp omniORB.cfg $OCPI_PREREQUISITES_INSTALL_DIR/omniorb/etc
