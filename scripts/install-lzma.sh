#!/bin/bash
OCPI_LZMA_VERSION=5.2.2
dir=xz-$OCPI_LZMA_VERSION
source ./scripts/setup-install.sh \
       "$1" \
       lzma \
       $dir.tar.gz \
       https://github.com/xz-mirror/xz/releases/download/v$OCPI_LZMA_VERSION \
       $dir \
       1

../configure  \
  ${OCPI_CROSS_HOST+--host=${OCPI_CROSS_HOST}} \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/lzma \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/lzma/$OCPI_TARGET_DIR \
  --enable-shared=yes --enable-static --disable-symbol-versions \
  --disable-xz --disable-xzdec --disable-lzmadec --disable-lzmainfo --disable-lzma-links \
  --disable-scripts --disable-doc \
  CFLAGS=-g CXXFLAGS=-g
make
make install
echo ============= lzma for $OCPI_TARGET_PLATFORM built and installed in $OCPI_PREREQUISITES_INSTALL_DIR/lzma/$OCPI_TARGET_DIR
