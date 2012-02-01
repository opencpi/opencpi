#!/bin/sh
OCPI_HOST_SYSTEM=linux-x86_64
set -e
. setup_install.sh
if test ! -d opencv ; then
mkdir opencv
fi
cd opencv
if test ! -d CMake ; then
mkdir -p CMake
cd CMake
curl -O http://www.cmake.org/files/v2.8/cmake-2.8.7-Linux-i386.tar.gz
gunzip cmake-2.8.7-Linux-i386.tar.gz
tar xf cmake-2.8.7-Linux-i386.tar
rm cmake-2.8.7-Linux-i386.tar
cd ..
fi
OCPI_OPEN_VERSION=2.3.1a
TarFile=OpenCV-$OCPI_OPEN_VERSION.tar
BZFile=$TarFile.bz2
if test ! -f $BZFile ; then
  if test ! -f $TarFile ; then
    echo You must download the source tar file: $BZFile from:
    echo     http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.3.1/
    echo It has no download URL, so you must do it yourself and put it in: `pwd`
    echo Then you can run this $0 script again.
    exit 1
  fi
fi
if test ! -f $TarFile ; then
bzip2 -d $BZFile
fi
tar xf OpenCV-$OCPI_OPEN_VERSION.tar
cd OpenCV-2.3.1
CMAKE=/opt/opencpi/prerequisites/opencv/CMake/cmake-2.8.7-Linux-i386/bin/cmake 
if test ! -d release ; then
mkdir release
fi
cd release
if test ! -d  $OCPI_HOST_SYSTEM ; then
mkdir $OCPI_HOST_SYSTEM
fi
cd $OCPI_HOST_SYSTEM
$CMAKE -D CMAKE_BUILD_TYPE=RELEASE -D CMAKE_INSTALL_PREFIX=release/ -D BUILD_PYTHON_SUPPORT=ON ../..
make
make install











