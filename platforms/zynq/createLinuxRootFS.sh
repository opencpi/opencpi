#!/bin/bash
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

# See the "usage" message below
set -e

if ! which fakeroot >/dev/null; then
  echo "Could not find 'fakeroot'. Please install it. (For CentOS 7, you must use the EPEL Repository.)"
  false
fi

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
#T=/tmp/ocpi-patch-rootfs-$$
# We need to preserve the uncompressed root file system for access by gdb
T=$FROM/uramdisk
set -e
rm -r -f $T
mkdir $T
cd $T
if [ -r $SOURCE ]; then
  echo "Found $SOURCE (compressed ramdisk image) in Xilinx binary release."
  dd if=$SOURCE bs=64 skip=1 | gunzip > in.root.image
elif [ -r $rdir/image.ub ]; then
  echo "Found $SOURCE (FIT image combining kernel/devicetree/ramdisk) in Xilinx binary release."
  SOURCE=$rdir/image.ub
  x=(`$FROM/fit_info -f $SOURCE -n /images/ramdisk@1 -p data`)
  if [ "${#x[@]}" != 6 ]; then
      echo "Failed to extract the root FS from $SOURCE (using fit_info tool from u-boot)."
      exit 1
  fi
  (set -o pipefail; dd if=$SOURCE bs=1 skip=${x[5]} count=${x[3]} | gunzip) > in.root.image
  if [ $? != 0 ]; then
      echo Failed to extract and decompress root FS from $SOURCE.
      exit 1
  fi
else
  echo Cannot find expected files in Xilinx linux binary release directory.
fi
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
# Clean up temp files while leaving root fs alone for debugging access by gdb
# The root fs clone could be further pruned to *only* support debugging.
rm ../out.root.image.gz ../in.root.image
cd $FROM
echo "A new patched root file system has been created, and placed in $(basename $FROM)"
echo "Copying some ancillary files from the binary release into the opencpi zynq release"
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
echo "============================================================================================"
echo "That directory is now complete: ready for creating a bootable SD card."
echo "============================================================================================"
