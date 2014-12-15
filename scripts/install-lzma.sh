#!/bin/sh
# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_HOST is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_TARGET is the gnu cross target
set -e
OCPI_LZMA_VERSION=5.0.5
. ./scripts/setup-install.sh
mkdir -p lzma
cd lzma
SUBDIR=xz-$OCPI_LZMA_VERSION
TARBALL=$SUBDIR.tar.gz
if test -f $TARBALL; then
  echo The distribution file for lzma, $TARBALL, exists and is being used.
  echo Remove `pwd`/$TARBALL if you want to download it again.
else
  echo Downloading the distribution file: $TARBALL
  curl -O -L http://tukaani.org/xz/$TARBALL
  echo Download complete.  Removing any existing build directories.
  rm -r -f $SUBDIR
fi
if test -d $SUBDIR; then
  echo The source directory $SUBDIR exists, using it for this target: $OCPI_TARGET_HOST.
else
  echo Unpacking download file $TARBALL into $SUBDIR.
  tar xzf xz-$OCPI_LZMA_VERSION.tar.gz
fi
cd $SUBDIR
# From here on is per target
if test -d build-$OCPI_TARGET_HOST; then
   echo Removing existing build directory for $OCPI_TARGET_HOST, for rebuilding.
   rm -r -f build-$OCPI_TARGET_HOST
fi
mkdir build-$OCPI_TARGET_HOST
cd build-$OCPI_TARGET_HOST
# We currently say that cross compilation means using static runtime libraries.
SHARED=yes
echo Building lzma in `pwd` for $OCPI_TARGET_HOST
if test "$OCPI_CROSS_HOST" != ""; then
 export PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
 crossConfig="CC=$OCPI_CROSS_HOST-gcc CXX=$OCPI_CROSS_HOST-g++ --host=$OCPI_CROSS_HOST"
 SHARED=no
fi
../configure  \
  $crossConfig \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/lzma \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/lzma/$OCPI_TARGET_HOST \
  --enable-shared=$SHARED --enable-static
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= lzma for $OCPI_TARGET_HOST built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/lzma/$OCPI_TARGET_HOST
