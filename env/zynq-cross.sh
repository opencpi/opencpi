# Definitions common to all zynq CROSS_COMPILATION platforms.
. $OCPI_CDK_DIR/scripts/xilinx-edk.sh
#binary for the compiler 
export OCPI_CROSS_BUILD_BIN_DIR=$OCPI_XILINX_EDK_DIR/gnu/arm/lin/bin
export OCPI_CROSS_HOST=arm-xilinx-linux-gnueabi
export OCPI_TARGET_OS=linux
export OCPI_TARGET_OS_VERSION=zynq
export OCPI_TARGET_ARCH=arm
export OCPI_TARGET_HOST=linux-zynq-arm
# Previous definitions of OCPI_ARCH / OCPI_TARGET_CFLAGS / OCPI_TARGET_CXXFLAGS are now in platforms/XXX/XXX-target.sh
export OCPI_LDFLAGS=
export OCPI_SHARED_LIBRARIES_FLAGS=
