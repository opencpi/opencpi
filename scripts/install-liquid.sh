#!/bin/sh
# For cross compiling we assume:
# 1. the cross tools are in the path
# 2. OCPI_TARGET_DIR is set properly (our target scheme, not the gnu target scheme)
# 3. OCPI_CROSS_HOST is the gnu cross target
set -e
OCPI_LIQUID_VERSION=v1.2.0
dir=liquid-dsp
source ./scripts/setup-install.sh \
       "$1" \
       liquid \
       $OCPI_LIQUID_VERSION \
       https://github.com/jgaeddert/liquid-dsp.git \
       $dir \
       1
echo PWD `pwd`
SHARED=yes
SEDINPLACE="sed --in-place"
if test "$OCPI_TARGET_OS" = macos; then
 crossConfig="CC=cc CXX=c++"
 SEDINPLACE='sed -i'
elif test "$OCPI_CROSS_HOST" != ""; then
 export PATH=$OCPI_CROSS_BUILD_BIN_DIR:$PATH
 crossConfig="CC=$OCPI_CROSS_HOST-gcc CXX=$OCPI_CROSS_HOST-g++ --host=$OCPI_CROSS_HOST"
# SHARED=no
fi
generators="\
./src/fec/gentab/reverse_byte_gentab \
./src/utility/gentab/count_ones_gentab \
"
if [ $OCPI_TOOL_PLATFORM != $OCPI_TARGET_PLATFORM ]; then
    [ -f $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TOOL_DIR/bin/reverse_byte_gentab ] || {
	echo It appears that you have not built the liquid library for $OCPI_TOOL_PLATFORM yet.
	echo This is required before trying to build this library for $OCPI_TARGET_PLATFORM.
	exit 1
    }
    for g in $generators; do
     cp $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TOOL_DIR/bin/$(basename $g) $(dirname $g)
    done
fi
(echo Performing '"reconf"' on git repo; cd ..; ./reconf)
base=$(basename `pwd`)
echo Copying git repo for building in `pwd`
(cd ..; cp -R $(ls . | grep -v $base) $base)
# patches to ./configure to not run afoul of macos stronger error checking
$SEDINPLACE -e 's/char malloc, realloc, free, memset,/char malloc(), realloc(), free(), memset(),/' ./configure
$SEDINPLACE -e 's/char sinf, cosf, expf, cargf, cexpf, crealf, cimagf,/char sinf(), cosf(), expf(), cargf(), cexpf(), crealf(), cimagf(),/' ./configure
./configure  \
  $crossConfig \
  --prefix=$OCPI_PREREQUISITES_INSTALL_DIR/liquid \
  --exec-prefix=$OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TARGET_DIR \
  --includedir=$OCPI_PREREQUISITES_INSTALL_DIR/liquid/include \
  CFLAGS=-g CXXFLAGS=-g
make
make install
# the recommendations from liquidsdr.org is to use #include "liquid/liquid.h" code per Aaron
# mv $OCPI_PREREQUISITES_INSTALL_DIR/liquid/include/liquid/* $OCPI_PREREQUISITES_INSTALL_DIR/liquid/include
# rmdir $OCPI_PREREQUISITES_INSTALL_DIR/liquid/include/liquid
[ $OCPI_TOOL_PLATFORM = $OCPI_TARGET_PLATFORM ] && {
  mkdir -p $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TARGET_DIR/bin
  for g in $generators; do
    cp $g $OCPI_PREREQUISITES_INSTALL_DIR/liquid/$OCPI_TARGET_DIR/bin
  done
}
exit 0
