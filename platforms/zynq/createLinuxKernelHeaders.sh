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
# "make" will run in parallel unless you set MAKE_PARALLEL to blank or null string (but defined)
# We set the default to 4 since using "-j" by itself blows up (infinite forks) on some systems.
export MAKE_PARALLEL=${MAKE_PARALLEL--j4}
set -e

if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help" -o "$2" = ""; then
  cat <<EOF
The purpose of this script is to create a "kernel headers" package with the necessary artifacts
for an OpenCPI Zynq Linux "release" based on a specific Xilinx release.
It assumes:
- a git repo for Xilinx linux kernel and u-boot has been established using:
      getXilinxLinuxSources.sh <git-repo-dir>
- a Xilinx EDK installation for OpenCPI exists in the environment
- the OCPI_CDK_DIR environment variable points to an OpenCPI installation

It does these things:
- Checks out the previously downloaded git source repo with the tag for this release
- Builds u-boot and the linux kernel (and the device tree binaries).
- Creates a sparse "kernel-headers" subset of the built linux source tree.
- Captures several other files from the build process in the resulting release directory

The result of this script is a release directory with everything that is needed to:
- Build the OpenCPI linux kernel driver (this completing the framework build for zynq)
- Create a bootable SD card (after running createLinuxRoosFS.sh).

The name of the resulting release directory is: opencpi-zynq-release-<release-name>
The name of the resulting release directory is: opencpi-zynq-release-<release-name>
After this step, the git repo directory can be removed to save space.

Usage is: createLinuxKernelHeaders.sh <release-name> <repo-tag> <repo-dir>

The release name can be the same as the tag, but it is usually the associated
Xilinx tool release associated with the tag unless it is between releases etc.
E.g. if the Xilinx release is 2013.4, the repo tag is xilinx-v2013.4, and the
OpenCPI Zynq release name is 13_4.
EOF
  exit 1
fi
rel=$1
tag=$2
case $3 in (/*) gdir=$3 ;; (*) gdir=`pwd`/$3;; esac
if test ! -d $gdir/linux-xlnx; then
  echo The source directory $3/linux-xlnx does not exist. Run getXilinxLinuxSources.sh\?
  exit 1
fi
source $OCPI_CDK_DIR/scripts/ocpitarget.sh xilinx$rel
CROSS_COMPILE=$OCPI_TARGET_CROSS_COMPILE
echo CROSS COMPILE prefix is: $CROSS_COMPILE
# Protect against sym links for the git subdir for case sensitivity
RELDIR=`pwd`/opencpi-zynq-linux-release-$rel
rm -r -f $RELDIR
mkdir $RELDIR
cd $gdir/u-boot-xlnx
git reset --hard origin/master
git clean -ffdx
echo ==============================================================================
echo Using the tag '"'$tag'"' for the Xilinx u-boot source repository.
make clean CROSS_COMPILE=$CROSS_COMPILE
make distclean CROSS_COMPILE=$CROSS_COMPILE
echo Checking out the Xilinx u-boot using the repository label '"'$tag'"'.
git checkout -f tags/$tag
echo ==============================================================================
echo Building u-boot to get the mkimage command.
make zynq_zed_config CROSS_COMPILE=$CROSS_COMPILE
make CROSS_COMPILE=$CROSS_COMPILE ${MAKE_PARALLEL}
cp tools/mkimage tools/fit_info $RELDIR
echo ==============================================================================
echo The u-boot build is complete.  Starting linux build.
echo ==============================================================================
echo Using the tag '"'$tag'"' for the Xilinx linux kernel source repository.
cd ../linux-xlnx
git reset --hard origin/master
git clean -ffdx
echo Checking out the Xilinx Linux kernel using the repository label '"'$tag'"'.
git checkout -f tags/$tag
if test $tag = xilinx-v14.7; then
    git checkout xilinx-v2013.4 drivers/usb/phy/phy-zynq-usb.c
    echo Patching zed device tree for ethernet phy issue.
    ed arch/arm/boot/dts/zynq-zed.dts <<EOF
   134
   s/phy@7/phy@0/p
    137
    s/<7>/<0>/p
    305
    s/host/otg/p
    w
EOF
fi
echo Adding support for USB Ethernet Dongles
ed arch/arm/configs/xilinx_zynq_defconfig <<EOF
g/CONFIG_USB_USBNET/d
g/CONFIG_USB_ZYNQ_PHY/d
$
a
CONFIG_USB_ZYNQ_PHY=y
CONFIG_USB_USBNET=y
CONFIG_USB_NET_AX8817X=y
.
w
EOF
echo ============================================================================================
echo Configuring the Xilinx linux kernel using their default configuration for zynq....
make V=2 ARCH=arm xilinx_zynq_defconfig
echo ============================================================================================
echo Building the Xilinx linux kernel for zynq to create the kernel-headers tree.
# To build a kernel that we would use, we would do:
PATH="$PATH:`pwd`/../u-boot-xlnx/tools" \
   make V=2 ARCH=arm CROSS_COMPILE=$CROSS_COMPILE \
        UIMAGE_LOADADDR=0x8000 ${MAKE_PARALLEL} uImage dtbs modules
ocpi_kernel_release=$(< include/config/kernel.release)-$(echo $tag | sed 's/^xilinx-//')
echo ============================================================================================
cd $RELDIR
echo Capturing the built Linux uImage file and the zynq device trees in release directory: $(basename $RELDIR)
mkdir dts lib
cp $gdir/linux-xlnx/arch/arm/boot/dts/zynq-*.dt* dts
cp $gdir/linux-xlnx/arch/arm/boot/uImage .
libstdc=$(dirname $CROSS_COMPILE)/../$(basename $CROSS_COMPILE|sed 's/-$//')/libc/
if [ -n "$(shopt -s nullglob; echo $libstdc/usr/lib/libstdc++.so*)" ]; then
  libstdc+=usr/
elif [ -z "$(shopt -s nullglob; echo $libstdc/lib/libstdc++.so*)" ]; then
  echo "WARNING: Cannot locate libstdc++.so in $libstdc/usr/lib or $libstdc/lib, so it will not be in the release"
  libstdc=
fi
[ -n "$libstdc" ] && cp -P $libstdc/lib/libstdc++.so* lib
echo ============================================================================================
echo Preparing the kernel-headers tree based on the built kernel.
rm -r -f kernel-headers-$tag kernel-headers
mkdir kernel-headers
# copy that avoids errors when caseinsensitive file systems are used (MacOS...)
#cp -R ../git/linux-xlnx/{Makefile,Module.symvers,include,scripts,arch} kernel-headers
(cd $gdir/linux-xlnx;
  for f in Makefile Module.symvers include scripts arch/arm .config; do
    find $f -type d -exec mkdir -p $RELDIR/kernel-headers/{} \;
    find $f -type f -exec sh -c \
     "if test -e $RELDIR/kernel-headers/{}; then
        echo File {} has a case sensitive duplicate which will be overwritten.
        rm -f $RELDIR/kernel-headers/{}
      fi
      cp {} $RELDIR/kernel-headers/{}" \;
  done
  for i in $(find -name 'Kconfig*'); do
    mkdir -p $RELDIR/kernel-headers/$(dirname $i)
    cp $i $RELDIR/kernel-headers/$(dirname $i)
  done
)
rm -r -f kernel-headers/arch/arm/boot
find kernel-headers -name "*.[csSo]" -exec rm {} \;
rm kernel-headers/scripts/{basic,mod}/.gitignore
# Record the kernel release AND the repo tag used.
echo $ocpi_kernel_release > kernel-headers/ocpi-release
echo ============================================================================================
echo The kernel-headers directory/package has been created in $(basename $RELDIR)
echo It is now ready for building the OpenCPI linux kernel driver for zynq
echo ============================================================================================
echo Removing *.cmd
find kernel-headers -name '*.cmd' -delete
echo Removing x86_64 binaries
find kernel-headers/scripts | xargs file | grep "ELF 64-bit" | cut -f1 -d: | xargs -tr -n1 rm
echo Removing auto.conf
rm kernel-headers/include/config/auto.conf
echo Restoring source to removed binaries
(cd $gdir/linux-xlnx;
  for f in $(find scripts/ -name '*.c' -o -name 'zconf.tab'); do
    mkdir -p $RELDIR/kernel-headers/$(dirname $f)
    cp $f $RELDIR/kernel-headers/$(dirname $f)
  done
  mkdir -p $RELDIR/kernel-headers/tools/include
  cp -R tools/include/tools $RELDIR/kernel-headers/tools/include
)
echo Removing unused large headers
rm -rf kernel-headers/include/linux/{mfd,platform_data}
