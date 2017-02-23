#!/bin/bash
# See the "usage" message below
set -e
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help" -o "$2" = ""; then
  cat <<EOF
The purpose of this script is to put an appropriately patched root file system
into the OpenCPI Zynq Linux "release" based on a specific Xilinx binary release.
It assumes:
- a (partially complete) OpenCPI Zynq release has been created using:
      createLinuxKernelHeaders.sh <rel-name> <repo-tag> <work-dir>
- a Xilinx Zynq binary release has been downloaded using:
      getXilinxLinuxBinaryRelease.sh <rel-name> <download-url> <work-dir>

It copies the root file system from the Xilinx linux binary release and patches it as needed.
The result of this script is a release directory with everything that is needed to create
a bootable SD card for a zynq platform.

Usage is: createLinuxRootFS.sh <release-name> <work-dir>

EOF
  exit 1
fi
case $2 in (/*) gdir=$2 ;; (*) gdir=`pwd`/$2;; esac
rel=$1
rdir=$gdir/xilinx-zynq-binary-release-for-$rel
if test ! -d $rdir; then
  echo The release directory for the $1 release \($rdir\) does not exist.
  echo Run getXilinxLinuxBinaryRelease.sh to download it for release $rel
  exit 1
fi
# Protect against sym links for the git subdir for case sensitivity
cd opencpi-zynq-linux-release-$rel
echo Patching the root file system from the Xilinx binary release to the OpenCPI release
FROM=`pwd`
SOURCE=$rdir/uramdisk.image.gz
DEST=$FROM/uramdisk.image.gz
T=/tmp/ocpi-patch-rootfs-$$
set -e
rm -r -f $T
mkdir $T
cd $T
dd if=$SOURCE bs=64 skip=1 | gunzip > in.root.image
mkdir root
cd root
fakeroot cpio -i --quiet -d -H newc -F ../in.root.image --no-absolute-filenames
ed -s etc/fstab<<EOF
/mmcblk0p1/
s/^ *# *//
s/,noauto//p
wq
EOF
ed -s etc/network/interfaces<<EOF
/auto eth0/
s/eth0/eth0 eth1/p
wq
EOF
# Put the C++ runtime library on the system
cp $FROM/lib/libstdc++.so* lib
# Record in the rootfs, which release we are actually running.
ocpi_kernel_release=$(< $FROM/kernel-headers/ocpi-release)
echo xilinx$rel linux-x$rel-arm $ocpi_kernel_release > etc/opencpi-release
# This is for backward compatibility
echo $ocpi_kernel_release > etc/ocpi-release
find . ! -name '\.' | fakeroot cpio -o -H newc | gzip > ../out.root.image.gz
$FROM/mkimage \
  -A arm \
  -O linux \
  -T ramdisk \
  -C gzip \
  -a 8000 \
  -e 8000 \
  -n "" \
  -d ../out.root.image.gz \
  $DEST
cd $FROM
rm -r -f $T
echo A new patched root file system has been created, and placed in $(basename $FROM)
echo Copying some ancillary files from the binary release into the opencpi zynq release
# This is where we unfortunately need per-platform knowledge for now, otherwise we need a
# whole copy of the binary release in our release dir.  We do for all platforms we know.
for i in $rdir/*; do
 if [ -d $i ]; then
     case $(basename $i) in
	 (zed)
	     mkdir -p zed
	     cp $rdir/zed/boot.bin zed
	     ;;
     esac
 fi
done
echo ============================================================================================
echo That directory is now complete: ready for creating a bootable SD card.
echo ============================================================================================
