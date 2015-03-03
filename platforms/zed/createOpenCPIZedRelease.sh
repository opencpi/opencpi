# See the "usage" message below
set -e
if test "$1" = "" -o "$1" = "--help" -o "$1" = "-help" -o "$2" = ""; then
  cat <<EOF
The purpose of this script is to create the the necessary artifacts for an
OpenCPI Zed '"release"' based on a specific Xilinx release.
It does these things:
- Checks out the git source repo for the tag associated with the Xilinx release.
- Applies some OpenCPI patches to the kernel configuration and device tree
- Builds u-boot and the linux kernel (and the device tree binary).
- Creates a sparse "kernel-headers" subset of the built linux source tree.
- Patches the root file system from the Xilinx release (separate from the linux build).
The result of this script is a release directory with everything that is needed to:
- Build the OpenCPI linux kernel driver.
- Populate an OpenCPI SD card directory tree based on this Xilinx release.
After running this script, OpenCPI, including its driver, can be built for zed.
EOF
  exit 1
fi
if test \
  "$OCPI_XILINX_EDK_DIR" = "" -o \
  "$OCPI_KERNEL_DIR" = "" -o \
  "$OCPI_XILINX_DIR" = "" -o \
  "$OCPI_XILINX_VERSION" = "" -o \
  ! -d "$OCPI_XILINX_EDK_DIR" -o \
  ! -d "$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION"; then
  echo Error: the OpenCPI build environment for Xilinx Zynq is not set up.
  exit 1
fi
if test `basename $(pwd)` != zed; then
  echo This script should be run from the platforms/zed directory.
  exit 1
fi
rel=$1
tag=$2
if test ! -d release-$1; then
  echo The release directory for the $1 release \(release-$1\) does not exist.
  exit 1
fi
if test ! -d git/linux-xlnx; then
  echo The source directory git/linux-xlnx does not exist. Run getXilinxLinuxSources.sh\?
  exit 1
fi
if test ! -r release-$1/SD-release; then
  echo The Xilinx release is not present in the release-$1/SD-release directory.
  echo Use getXilinxRelease.sh
  exit 1
fi
# Protect against sym links for the git subdir for case sensitivity
RELDIR=`pwd`/release-$1
cd git/u-boot-xlnx
echo ==============================================================================
echo Using the tag '"'$tag'"' for the Xilinx u-boot source repository.
make clean CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-
make distclean CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-
echo Checking out the Xilinx u-boot using the repository label '"'$tag'"'.
git checkout -f $tag
echo ==============================================================================
echo Building u-boot to get the mkimage command.
make zynq_zed_config CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-
make CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-
echo ==============================================================================
echo Using the tag '"'$tag'"' for the Xilinx linux kernel source repository.
cd ../linux-xlnx
make clean
make distclean
echo Checking out the Xilinx Linux kernel using the repository label '"'$tag'"'.
git checkout -f $tag
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
echo ==============================================================================
echo Configuring the Xilinx linux kernel using their default configuration for zynq....
make ARCH=arm xilinx_zynq_defconfig
echo ==============================================================================
echo Building the Xilinx linux kernel for zynq to create the kernel-headers tree.
# To only build for driver building and device tree:
#make ARCH=arm CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-
# To build a kernel that we would use, we would do:
PATH="$PATH:`pwd`/../u-boot-xlnx/tools" \
   make ARCH=arm CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST- \
        UIMAGE_LOADADDR=0x8000 uImage dtbs
echo ==============================================================================
cd $RELDIR
echo Capturing the built Linux uImage file and the zed device tree into our release directory.
cp ../git/linux-xlnx/arch/arm/boot/dts/zynq-zed.dtb .
cp ../git/linux-xlnx/arch/arm/boot/uImage .
echo ==============================================================================
echo Preparing the kernel-headers tree based on the built kernel.
rm -r -f kernel-headers-$tag kernel-headers
ln -s kernel-headers-$tag kernel-headers
mkdir kernel-headers-$tag
# Ugly copy that avoids errors when caseinsensitive file systems are used (MacOS...)
#cp -R ../git/linux-xlnx/{Makefile,Module.symvers,include,scripts,arch} kernel-headers-$tag
(cd ../git/linux-xlnx; for f in Makefile Module.symvers include scripts arch; do
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
echo ==============================================================================
echo The kernel-headers-$tag directory has been populated and is ready to use for building drivers
echo ==============================================================================
echo Patching the root file system.
FROM=`pwd`
SOURCE=`pwd`/SD-release/uramdisk.image.gz
DEST=`pwd`/uramdisk.image.gz
T=/tmp/ocpi-patch-SD-$$
set -e
mkdir $T
cd $T
dd if=$SOURCE bs=64 skip=1 | gunzip > in.root.image
mkdir root
cd root
fakeroot cpio -i -d -H newc -F ../in.root.image --no-absolute-filenames
ed etc/fstab<<EOF
/mmcblk0p1/
s/^ *# *//
s/,noauto//p
wq
EOF
ed etc/network/interfaces<<EOF
/auto eth0/
s/eth0/eth0 eth1/p
wq
EOF
find . ! -name '\.' | fakeroot cpio -o -H newc | gzip > ../out.root.image.gz
$FROM/../git/u-boot-xlnx/tools/mkimage \
  -A arm \
  -O linux \
  -T ramdisk \
  -C gzip \
  -a 8000 \
  -e 8000 \
  -n "" \
  -d ../out.root.image.gz \
  $DEST
rm -r -f $T
echo A new patched root file system has been created.

