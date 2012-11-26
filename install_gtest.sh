#!/bin/sh
set -e
OCPI_GTEST_VERSION=1.6.0
. ./setup_install.sh
mkdir -p gtest
cd gtest
rm -r -f gtest*
curl -O http://googletest.googlecode.com/files/gtest-$OCPI_GTEST_VERSION.zip
unzip gtest-$OCPI_GTEST_VERSION.zip
cd gtest-$OCPI_GTEST_VERSION
# From here on is per target
mkdir build-$OCPI_BUILD_TARGET
cd build-$OCPI_BUILD_TARGET
c++ -m64 -fPIC -I../include -I.. -c ../src/gtest-all.cc
ar -rs libgtest.a gtest-all.o
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_BUILD_TARGET
ln -f -s `pwd` $OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_BUILD_TARGET/lib
ln -f -s `cd ..;pwd`/include $OCPI_PREREQUISITES_INSTALL_DIR/gtest/include
