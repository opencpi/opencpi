
#binary for the compiler 
export OCPI_CROSS_BUILD_BIN_DIR=$OCPI_XILINX_EDK_DIR/gnu/arm/lin/bin
export OCPI_CROSS_HOST=arm-xilinx-linux-gnueabi
export OCPI_TARGET_HOST=linux-zynq-arm
export OCPI_TARGET_ARCH=arm
export OCPI_TARGET_OS_VERSION=zed
export OCPI_ARCH=arm
# export OCPI_CFLAGS=
# export OCPI_CXXFLAGS=
export OCPI_LDFLAGS=
export OCPI_SHARED_LIBRARIES_FLAGS=
# When we build the driver the kernel should be cloned, checked out
# with the label consistent with the ISE version, and build there
export OCPI_KERNEL_DIR=$OCPI_XILINX_DIR/$OCPI_XILINX_VERSION/git/linux-xlnx
