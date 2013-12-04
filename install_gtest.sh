#!/bin/sh
set -e
OCPI_GTEST_VERSION=1.7.0
. ./setup_install.sh
mkdir -p gtest
cd gtest
rm -r -f gtest*
curl -O http://googletest.googlecode.com/files/gtest-$OCPI_GTEST_VERSION.zip
unzip gtest-$OCPI_GTEST_VERSION.zip
# From here on is per target
rm -r -f build-$OCPI_TARGET_HOST
mkdir build-$OCPI_TARGET_HOST
cd build-$OCPI_TARGET_HOST
if test "$OCPI_CROSS_HOST" = ""; then
CC=gcc
#macos only CXX="c++ -stdlib=libstdc++"
CXX=c++
LD=c++
AR=ar
else
CC=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-gcc
CXX=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-c++
LD=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-c++
AR=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-ar
fi
dir=../gtest-$OCPI_GTEST_VERSION
$CXX -fPIC -I$dir/include -I$dir -c $dir/src/gtest-all.cc
$AR -rs libgtest.a gtest-all.o
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_TARGET_HOST
ln -f -s `pwd` $OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_TARGET_HOST/lib
ln -f -s `cd $dir;pwd`/include $OCPI_PREREQUISITES_INSTALL_DIR/gtest/include
echo ============= gtest for $OCPI_TARGET_HOST built and installed
