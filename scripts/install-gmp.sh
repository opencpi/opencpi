#!/bin/sh
# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_HOST is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_TARGET is the gnu cross target
set -e
OCPI_GMP_VERSION=6.1.1
. ./scripts/setup-install.sh
mkdir -p gmp
cd gmp
SUBDIR=gmp-$OCPI_GMP_VERSION
TARBALL=$SUBDIR.tar.xz
if test -f $TARBALL; then
  echo The distribution file for gmp, $TARBALL, exists and is being used.
  echo Remove `pwd`/$TARBALL if you want to download it again.
else
  echo Downloading the distribution file: $TARBALL
#  curl -O -L http://tukaani.org/xz/$TARBALL
# https://github.com/xz-mirror/xz/releases/download/v5.0.8/xz-5.0.8.tar.gz
  curl -O -L https://ftp.gnu.org/gnu/gmp/$TARBALL
  echo Download complete.  Removing any existing build directories.
  rm -r -f $SUBDIR
fi
if test -d $SUBDIR; then
  echo The source directory $SUBDIR exists, using it for this target: $OCPI_TARGET_HOST.
else
  echo Unpacking download file $TARBALL into $SUBDIR.
  tar -x --xz -f $TARBALL
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
if test "$OCPI_DYNAMIC" != 1; then
  DYNAMIC=--enable-static=yes
else
  DYNAMIC=--enable-shared=yes
fi
SHARED=yes
echo Building gmp in `pwd` for $OCPI_TARGET_HOST
if test "$OCPI_CROSS_HOST" != ""; then
 export PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
 crossConfig="CC=$OCPI_CROSS_HOST-gcc CXX=$OCPI_CROSS_HOST-g++ --host=$OCPI_CROSS_HOST"
# SHARED=no
fi

../configure  \
  $crossConfig \
  --enable-cxx=yes \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gmp \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gmp/$OCPI_TARGET_HOST \
  $DYNAMIC \
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= gmp for $OCPI_TARGET_HOST built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/lzma/$OCPI_TARGET_HOST
