#!/bin/bash
# See the "usage" message below
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

Usage is: createOpenCPIZedRelease.sh <release-name> <repo-tag> <repo-dir>

The release name can be the same as the tag, but it is usually the associated
Xilinx tool release associated with the tag unless it is between releases etc.
E.g. if the Xilinx release is 2013.4, the repo tag is xilinx-v2013.4, and the 
OpenCPI Zynq release name is 13_4.
EOF
  exit 1
fi
# Enable xilinx variables
source $OCPI_CDK_DIR/scripts/util.sh
setVarsFromMake $OCPI_CDK_DIR/include/hdl/xilinx.mk ShellIseVars=1 $verbose

if test \
  "$OcpiXilinxEdkDir" = "" -o \
  ! -d "$OcpiXilinxEdkDir" ; then \
  echo Error: the OpenCPI build environment for Xilinx Zynq is not set up.
  exit 1
fi
OCPI_CROSS_BUILD_BIN_DIR=$OcpiXilinxEdkDir/gnu/arm/lin/bin
OCPI_CROSS_HOST=arm-xilinx-linux-gnueabi
CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/${OCPI_CROSS_HOST}-
rel=$1
tag=$2
case $3 in (/*) gdir=$3 ;; (*) gdir=`pwd`/$3;; esac
if test ! -d $gdir/linux-xlnx; then
  echo The source directory $3/linux-xlnx does not exist. Run getXilinxLinuxSources.sh\?
  exit 1
fi
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
make CROSS_COMPILE=$CROSS_COMPILE
cp tools/mkimage $RELDIR
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
/CONFIG_USB_USBNET/d
/CONFIG_USB_ZYNQ_PHY/d
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
make ARCH=arm xilinx_zynq_defconfig
echo ============================================================================================
echo Building the Xilinx linux kernel for zynq to create the kernel-headers tree.
# To build a kernel that we would use, we would do:
PATH="$PATH:`pwd`/../u-boot-xlnx/tools" \
   make ARCH=arm CROSS_COMPILE=$CROSS_COMPILE \
        UIMAGE_LOADADDR=0x8000 uImage dtbs
ocpi_kernel_release=$(< include/config/kernel.release)-$(echo $tag | sed 's/^xilinx-//')
echo ============================================================================================
cd $RELDIR
echo Capturing the built Linux uImage file and the zynq device trees in release directory: $(basename $RELDIR)
mkdir dts lib
cp $gdir/linux-xlnx/arch/arm/boot/dts/zynq-*.dt* dts
cp $gdir/linux-xlnx/arch/arm/boot/uImage .
cp -P $OCPI_CROSS_BUILD_BIN_DIR/../$OCPI_CROSS_HOST/libc/usr/lib/libstdc++.so* lib
echo ============================================================================================
echo Preparing the kernel-headers tree based on the built kernel.
rm -r -f kernel-headers-$tag kernel-headers
mkdir kernel-headers
# copy that avoids errors when caseinsensitive file systems are used (MacOS...)
#cp -R ../git/linux-xlnx/{Makefile,Module.symvers,include,scripts,arch} kernel-headers
(cd $gdir/linux-xlnx; for f in Makefile Module.symvers include scripts arch; do
  find $f -type d -exec mkdir -p $RELDIR/kernel-headers/{} \;
  find $f -type f -exec sh -c \
     "if test -e $RELDIR/kernel-headers/{}; then
        echo File {} has a case sensitive duplicate which will be overwritten.
        rm -f $RELDIR/kernel-headers/{}
      fi 
      cp {} $RELDIR/kernel-headers/{}" \;
  done
)
(cd kernel-headers/arch; for i in *; do if test $i != arm; then rm -r -f $i; fi; done)
rm -r -f kernel-headers/arch/arm/boot
find kernel-headers -name "*.[csSo]" -exec rm {} \;
rm kernel-headers/scripts/{basic,mod}/.gitignore
# Record the kernel release AND the repo tag used.
echo $ocpi_kernel_release > kernel-headers/ocpi-release
echo ============================================================================================
echo The kernel-headers directory/package has been created in $(basename $RELDIR)
echo It is now ready for building the OpenCPI linux kernel driver for zynq
echo ============================================================================================
