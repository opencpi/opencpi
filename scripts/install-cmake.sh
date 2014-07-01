#!/bin/bash --noprofile
. setup_install.sh
if test ! -d cmake ; then
  mkdir -p cmake
fi
cd cmake
CMAKE_MAJOR=2.8
CMAKE_MINOR=8
CMAKE_VERSION=$CMAKE_MAJOR.$CMAKE_MINOR
CMAKE_DIR=cmake-$CMAKE_VERSION
CMAKE_INSTALL_DIR=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TOOL_HOST
TARFILE=cmake-$CMAKE_VERSION.tar.gz
if test ! -f $TARFILE ; then
  curl -O http://www.cmake.org/files/v$CMAKE_MAJOR/$TARFILE
fi
rm -f -r $CMAKE_INSTALL_DIR
rm -f -r $CMAKE_DIR
rm -f -r $OCPI_TOOL_HOST
tar xzf $TARFILE
cd $CMAKE_DIR
./bootstrap --prefix=$CMAKE_INSTALL_DIR
make
make install









