# Setup to build this target
OcpiThisFile=$(lastword $(MAKEFILE_LIST))
include $(dir $(OcpiThisFile))/target-xilinx.mk
f:=$(OCPI_XILINX_EDK_DIR)/gnu/arm/lin/bin
ifeq ($(wildcard $f),)
  $(error When setting up to build for zed, OCPI_XILINX_EDK_DIR is "$(OCPI_XILINX_EDK_DIR)". Cannot find $f. Perhaps the EDK was not installed when Xilinx tools were installed?).
endif
export OCPI_CROSS_BUILD_BIN_DIR:=$f
export OCPI_CROSS_HOST:=arm-xilinx-linux-gnueabi
export OCPI_ARCH=arm
export OCPI_TARGET_CFLAGS:=-mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9 -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wconversion -std=c99
export OCPI_TARGET_CXXFLAGS:=-mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9 -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wconversion -std=c++0x
export OCPI_LDFLAGS=
export OCPI_SHARED_LIBRARIES_FLAGS=
CC:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc
CXX:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
LD:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
AR:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-ar
OCPI_EXPORT_DYNAMIC:=-rdynamic
