#!/bin/bash --noprofile
OCPI_CMAKE_MAJOR_VERSION=3.7
OCPI_CMAKE_MINOR_VERSION=2
OCPI_CMAKE_VERSION=$OCPI_CMAKE_MAJOR_VERSION.$OCPI_CMAKE_MINOR_VERSION
dir=cmake-$OCPI_CMAKE_VERSION
source ./scripts/setup-install.sh \
       "$1" \
       cmake \
       $dir.tar.gz \
       https://cmake.org/files/v$OCPI_CMAKE_MAJOR_VERSION \
       $dir \
       0
tar xzf ../../$dir.tar.gz
mv $dir/* $dir/.??* .
./bootstrap --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TARGET_DIR
make
make install
