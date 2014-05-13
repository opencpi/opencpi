#!/bin/sh
# This script fills an SD card from the directory created using makeSD.sh
if test "$OCPI_XILINX_VERSION" = ""; then
  echo Error: the environment is not set up properly - OCPI_XILINX_VERSION not set.
  exit 1
fi
dir=`dirname $0`/SD-$OCPI_XILINX_VERSION
if test ! -d $dir; then
  echo Error: The SD directory, $dir, does not exist. Run makeSD.sh before this.
  exit 1
fi
if test "$1" = ""; then
  echo This script fills an SD card file system with the files needed to use OpenCPI.
  echo It will copy from $dir onto the mounted SD file system supplied as an argument.
  exit 1
fi
if test ! -d $1; then
  echo The supplied destination SD card file system, $1, is not a directory.
  exit 1
fi
echo Copying files from $dir to $1
cp -R -p $dir/* $1
