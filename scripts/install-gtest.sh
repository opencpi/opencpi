#!/bin/bash
OCPI_GTEST_VERSION=1.7.0
source ./scripts/setup-install.sh \
       "$1" \
       gtest \
       release-$OCPI_GTEST_VERSION.zip \
       https://github.com/google/googletest/archive \
       googletest-release-$OCPI_GTEST_VERSION \
       1

$CXX -fPIC -I../include -I.. -c ../src/gtest-all.cc
$AR -rs libgtest.a gtest-all.o
dname=libgtest.$OCPI_TARGET_DYNAMIC_SUFFIX
iname=$OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_TARGET_DIR/lib/$dname
[ "$OCPI_TARGET_OS" = macos ] && install_name="-install_name $iname"
$CXX $OCPI_TARGET_DYNAMIC_FLAGS $install_name -o $dname gtest-all.o
mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_TARGET_DIR/lib
ln -f -s `pwd`/libgtest.a $OCPI_PREREQUISITES_INSTALL_DIR/gtest/$OCPI_TARGET_DIR/lib
ln -f -s `pwd`/$dname $iname
ln -f -s `cd ..;pwd`/include $OCPI_PREREQUISITES_INSTALL_DIR/gtest/include
echo ============= gtest for $OCPI_TARGET_PLATFORM built and installed
