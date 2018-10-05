#!/bin/bash --noprofile
# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

source scripts/setup-install.sh
CMAKE=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TOOL_DIR/bin/cmake
if test ! -x $CMAKE ; then
  echo No cmake found at: $CMAKE
  echo Installing opencv requires cmake, and it is not installed as an opencpi prerequisite.
  echo Either use the install-cmake.mk script to do that, of modify this script to
  echo point to an existing cmake.  We don\'t use a system-installed cmake unless you ask for it.
  exit 1
fi
mkdir -p opencv
cd opencv
rm -r -f OpenCV* opencv*
OPENCV_MAJOR=2.4.10
OPENCV_MINOR=
OPENCV_VERSION=$OPENCV_MAJOR$OPENCV_MINOR
TarFile=opencv-$OPENCV_VERSION
ZIPFile=$TarFile.zip
if test ! -f $ZIPFile ; then
  curl -O -L http://downloads.sourceforge.net/project/opencvlibrary/opencv-unix/$OPENCV_MAJOR/$ZIPFile
#  if test ! -f $TarFile ; then
#    echo You must download the source tar file: $BZFile from:
#    echo     http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.3.1/
#    echo It has no download URL, so you must do it yourself and put it in: `pwd`
#    echo Then you can run this $0 script again.
#    exit 1
#  fi
fi
# rm -f $ZIPFile
OPENCV_DIR=opencv-$OPENCV_MAJOR
OPENCV_INSTALL_DIR=$OCPI_PREREQUISITES_INSTALL_DIR/opencv/$OCPI_TARGET_DIR
rm -f -r $OPENCV_DIR
rm -f -r $OPENCV_INSTALL_DIR
unzip $ZIPFile
#tar xf $TarFile
cd opencv-$OPENCV_MAJOR
mkdir build-$OCPI_TARGET_DIR
if test `uname` == XXXDarwin; then
  # Force the rpath in the various targets to be @rpath
  for sd in modules/{gpu,highgui,stitching}/CMakeLists.txt OpenCVModule.cmake; do
    echo editing $sd to force @rpath as install_name
    ed $sd <<EOF    
g/INSTALL_NAME_DIR/s/lib/@rpath/p
w
EOF
  done
fi
cd build-$OCPI_TARGET_DIR
#if test ! -d release ; then
#mkdir release
#fi
#cd release
PKG_CONFIG_PATH=/usr/local/lib/pkgconfig \
  $CMAKE \
  -D CMAKE_BUILD_TYPE=RELEASE \
  -D CMAKE_INSTALL_PREFIX=$OPENCV_INSTALL_DIR \
  ..
make
make install
