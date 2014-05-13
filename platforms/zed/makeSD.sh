#!/bin/sh
# This script prepares OpenCPI to use a Zed board by creating and filling a directory
# to be written to an SD card for booting the zed board.
# It assumes you:
# 1. already have a version of Xilinx tools installed.
# 2. have set up the OpenCPI build for linux-zynq-arm based on Xilinx cross tools
# 3. have successfully build the OpenCPI software tree with those tools
# 4. have successfully built the OpenCPI Linux kernel driver
#
# This script can be repeatedly run; it first removes all the files it has created
# so it can be re-run repeatedly
#
# It assumes you are running it where it lives in the OpenCPI tree, in platforms/zed
set -e
if test "$1" = ""; then
  echo 'This script takes a single argument (anything) to run.'
  echo "Don't forget to copy defaultsetup.sh to mysetup.sh and customize mysetup.sh before doing this."
  exit 1
fi
echo "Don't forget to copy defaultsetup.sh to mysetup.sh and customize mysetup.sh before doing this."
echo "If not, do it and run this script again."
rel=$OCPI_XILINX_VERSION-release
rf=$rel.tar.xz
rurl=http://www.wiki.xilinx.com/file/view/$rf
sd=SD-$OCPI_XILINX_VERSION
tmp=tmp-$OCPI_XILINX_VERSION
rm -r -f $sd $tmp
mkdir $tmp
cd $tmp
echo Downloading the Xilinx release file $rf form $rurl...
curl -O $rurl
echo Creating subdirectories and unpacking the release file...
mkdir $rel
mkdir ../$sd
cd $rel # now in $tmp/$rel
tar xvJf ../$rf
echo Populating the SD image directory from the release files.
cp uImage uramdisk.image.gz zed/boot.bin zed/devicetree.dtb ../../$sd
cd ../.. # now in $tmp
echo Adding OpenCPI setup scripts to the SD image directory
cp ocpizedsetup.sh $sd
echo You should have already customized the mysetup.sh script for your environment
cp mysetup.sh $sd
rm -r -f $tmp
