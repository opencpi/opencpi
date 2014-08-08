#!/bin/bash --noprofile
. setup_install.sh
CMAKE=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TOOL_HOST/bin/cmake
if test ! -d cilk ; then
  mkdir -p cilk
fi
cd cilk
CILK_VERSION=004225
TarFile=cilktools-linux-$CILK_VERSION.tgz
ExDir=cilktools-linux-$CILK_VERSION
if test ! -f $TarFile ; then
    echo You must download the source tar file: $TarFile from:
    echo    https://software.intel.com/en-us/articles/download-intel-cilk-sdk
    echo It has no download URL, so you must do it yourself and put it in: `pwd`
    echo Then you can run this $0 script again.
    exit 1
fi
tar xvzf $TarFile
ln -sf cilktools-linux-$CILK_VERSION cilktools

BZEXT=bz2
GccTarBZFile=cilkplus-4_8-install.tar_0.$BZEXT
GccTarFile=cilkplus-4_8-install.tar_0
if test ! -f $GccTarBZFile ; then
    echo t
    curl -O http://www.cilkplus.org/sites/default/files/cilk-gcc-compiler/$GccTarBZFile
#  if test ! -f $TarFile ; then
#    echo You must download the source tar file: $BZFile from:
#    echo     http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.3.1/
#    echo It has no download URL, so you must do it yourself and put it in: `pwd`
#    echo Then you can run this $0 script again.
#    exit 1
#  fi
fi
bunzip2 -d $GccTarBZFile
tar xvf $GccTarBZFile
ln -sf cilkplus-4_8-install cilkplus
#rm -f $GccTarFile



