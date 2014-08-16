#!/bin/bash --noprofile
. setup_install.sh
CMAKE=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TOOL_HOST/bin/cmake
if test ! -d cilk ; then
  mkdir -p cilk
fi
cd cilk

BZEXT=bz2
GccTarBZFile=cilkplus-4_8-install.tar_0.$BZEXT
GccTarFile=cilkplus-4_8-install.tar_0
if test ! -f $GccTarBZFile ; then
    curl -O http://www.cilkplus.org/sites/default/files/cilk-gcc-compiler/$GccTarBZFile
fi
bunzip2 -d $GccTarBZFile
tar xvf $GccTarFile
ln -sf cilkplus-4_8-install cilkplus
rm -f $GccTarFile




