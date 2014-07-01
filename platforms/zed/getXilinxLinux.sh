# This script obtains and builds the Xilinx Linux kernel for a given version
# of ISE.  It takes two arguments - the location where the Xilinx tools are installed,
# and the release number of that installation.  It does NOT require you to set up
# or source any Xilinx tools scripts.
set -e
if test "$1" = ""; then
  echo This script installs and configures the Xilinx Linux Kernel source tree for Zynq, to
  echo enable the OpenCPI Linux Kernel driver to be built against it.
  echo After this script completes successfully you can do '"make driver"' in
  echo the top level OpenCPI directory.
  echo To actually run this script, give it a single argument which can be anything.
  echo This script needs the environment to be already set up for cross-building for zynq.
  echo If the directory already exists, it will be removed and replaced.
  exit 1
fi
if test \
  "$OCPI_XILINX_EDK_DIR" = "" -o \
  "$OCPI_KERNEL_DIR" = "" -o \
  "$OCPI_XILINX_DIR" = "" -o \
  "$OCPI_XILINX_VERSION" = "" -o \
  ! -d "$OCPI_XILINX_EDK_DIR" -o \
  ! -d "$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION" \
; then
  echo Error: the OpenCPI build environment for Xilinx Zynq is not set up.
  exit 1
fi
echo ==============================================================================
echo Establishing git subdirectory in $OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/git
cd $OCPI_XILINX_DIR/$OCPI_XILINX_VERSION
mkdir -p git; cd git
if test -d linux-xlnx; then
  echo The git/linux-xlnx directory already exists.  It will be removed.
  rm -r -f linux-xlnx
fi
echo Cloning/downloading the Xilinx linux kernel tree from github for ISE version $OCPI_XILINX_VERSION...
git clone git://github.com/Xilinx/linux-xlnx.git
cd linux-xlnx
label=`git tag | grep v$OCPI_XILINX_VERSION | tail -1`
echo Using the git repo label $label, which appears to be the latest one for version $OCPI_XILINX_VERSION.
git checkout -f $label
echo Configuring the Xilinx linux kernel using their default configuration for zynq....
echo ==============================================================================
make ARCH=arm xilinx_zynq_defconfig
#echo ==============================================================================
#echo Building the Xilinx linux kernel for zynq...
#echo ==============================================================================
#make ARCH=arm UIMAGE_LOADADDR=0x8000 uImage CROSS_COMPILE=$OCPI_CROSS_BUILD_BIN_DIR/$OCPI_CROSS_HOST-
echo ==============================================================================
echo The Xilinx Linux Kernel tree for Zynq is available to build the OpenCPI driver
echo ==============================================================================
