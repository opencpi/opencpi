#!/bin/bash
OCPI_PATCHELF_VERSION=0.8
dir=patchelf-$OCPI_PATCHELF_VERSION
source ./scripts/setup-install.sh \
       "$1" \
       patchelf \
       $dir.tar.gz \
       http://nixos.org/releases/patchelf/$dir \
       $dir \
       0

../configure  \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/patchelf \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/patchelf/$OCPI_TARGET_DIR \
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= patchelf for $OCPI_TARGET_PLATFORM built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/patchelf/$OCPI_TARGET_DIR
