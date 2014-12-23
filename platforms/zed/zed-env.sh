. ./env/xilinx.sh
. ./env/zynq-cross.sh

export OCPI_TARGET_PLATFORM=zed

# Build static libraries for zynq so we don't have to install a directory
# full of shared libraries.  In particular (as of 12/22/14), some of the
# prerequisite libraries are only built statically when they are cross compiled.
export OCPI_BUILD_SHARED_LIBRARIES=0

if test "$OCPI_KERNEL_DIR" = ""; then
  # When we build the driver the kernel should be cloned, checked out
  # with the label consistent with the ISE version, and build there
  export OCPI_KERNEL_DIR=$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/git/linux-xlnx
fi
