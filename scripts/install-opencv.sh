#!/bin/bash --noprofile
. setup_install.sh
CMAKE=$OCPI_PREREQUISITES_INSTALL_DIR/cmake/$OCPI_TOOL_HOST/bin/cmake
if test ! -x $CMAKE ; then
  echo No cmake found at: $CMAKE
  echo Installing opencv requires cmake, and it is not installed as an opencpi prerequisite.
  echo Either use the install-cmake.mk script to do that, of modify this script to
  echo point to an existing cmake.  We don\'t use a system-installed cmake unless you ask for it.
  exit 1
fi
if test ! -d opencv ; then
  mkdir -p opencv
fi
cd opencv
OPENCV_MAJOR=2.4.3
OPENCV_MINOR=
OPENCV_VERSION=$OPENCV_MAJOR$OPENCV_MINOR
TarFile=OpenCV-$OPENCV_VERSION.tar
BZFile=$TarFile.bz2
if test ! -f $BZFile ; then
  curl -O http://softlayer.dl.sourceforge.net/project/opencvlibrary/opencv-unix/$OPENCV_MAJOR/$BZFile
#  if test ! -f $TarFile ; then
#    echo You must download the source tar file: $BZFile from:
#    echo     http://sourceforge.net/projects/opencvlibrary/files/opencv-unix/2.3.1/
#    echo It has no download URL, so you must do it yourself and put it in: `pwd`
#    echo Then you can run this $0 script again.
#    exit 1
#  fi
fi
rm -f $TarFile
OPENCV_DIR=OpenCV-$OPENCV_MAJOR
OPENCV_INSTALL_DIR=$OCPI_PREREQUISITES_INSTALL_DIR/opencv/$OCPI_TARGET_HOST
rm -f -r $OPENCV_DIR
rm -f -r $OPENCV_INSTALL_DIR
bunzip2 -d $BZFile
tar xf $TarFile
cd OpenCV-$OPENCV_MAJOR
mkdir build-$OCPI_TARGET_HOST
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
cd build-$OCPI_TARGET_HOST
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
