#!/bin/sh
OCPI_GMP_VERSION=6.1.1
dir=gmp-$OCPI_GMP_VERSION
source ./scripts/setup-install.sh \
       "$1" \
       gmp \
       $dir.tar.xz \
       https://ftp.gnu.org/gnu/gmp \
       $dir \
       1

../configure  \
  ${OCPI_CROSS_HOST+--host=${OCPI_CROSS_HOST}} \
  --enable-cxx=yes \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gmp \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/gmp/$OCPI_TARGET_DIR \
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= gmp for $OCPI_TARGET_PLATFORM built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/gmp/$OCPI_TARGET_DIR
