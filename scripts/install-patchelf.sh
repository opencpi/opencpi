#!/bin/sh
# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_HOST is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_TARGET is the gnu cross target
set -e
if test "$OCPI_CROSS_HOST" != ""; then
 echo We do not use patchelf in embedded environments.
 exit 0
fi
OCPI_PATCHELF_VERSION=0.8
. ./scripts/setup-install.sh
mkdir -p patchelf
cd patchelf
SUBDIR=patchelf-$OCPI_PATCHELF_VERSION
TARBALL=$SUBDIR.tar.gz
if test -f $TARBALL; then
  echo The distribution file for patchelf, $TARBALL, exists and is being used.
  echo Remove `pwd`/$TARBALL if you want to download it again.
else
  echo Downloading the distribution file: $TARBALL
  curl -O -L http://nixos.org/releases/patchelf/patchelf-0.8/patchelf-0.8.tar.gz
  echo Download complete.  Removing any existing build directories.
  rm -r -f $SUBDIR
fi
if test -d $SUBDIR; then
  echo The source directory $SUBDIR exists, using it for this target: $OCPI_TARGET_HOST.
else
  echo Unpacking download file $TARBALL into $SUBDIR.
  tar xzf $TARBALL
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
echo Building patchelf in `pwd` for $OCPI_TARGET_HOST
../configure  \
  $crossConfig \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/patchelf \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/patchelf/$OCPI_TARGET_HOST \
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= patchelf for $OCPI_TARGET_HOST built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/patchelf/$OCPI_TARGET_HOST
