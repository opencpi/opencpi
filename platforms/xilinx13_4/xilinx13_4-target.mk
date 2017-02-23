# Setup to build this target
include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
f:=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin
ifeq ($(wildcard $f),)
  $(error When setting up to build for zynq for $(OCPI_TARGET_PLATFORM), cannot find $f. Perhaps the EDK was not installed\
          when Xilinx tools were installed? The non-default Xilinx environment settings were: \
          $(foreach v,$(filter OCPI_XILINX%,$(.VARIABLES)), $v=$($v)))
endif
export OCPI_CROSS_BUILD_BIN_DIR:=$f
export OCPI_CROSS_HOST:=arm-xilinx-linux-gnueabi
export OCPI_TARGET_CFLAGS:=-mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9 -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wconversion -std=c99
export OCPI_TARGET_CXXFLAGS:=$(OCPI_TARGET_CXXFLAGS) -mfpu=neon-fp16 -mfloat-abi=softfp -march=armv7-a -mtune=cortex-a9 -Wall -Wfloat-equal -Wextra -fno-strict-aliasing -Wconversion -std=c++0x
export OCPI_LDFLAGS=
export OCPI_SHARED_LIBRARIES_FLAGS=
export CC:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc
export CXX:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
export LD:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-c++
export AR:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-ar
export OCPI_EXPORT_DYNAMIC:=-rdynamic
export OCPI_EXTRA_LIBS:=rt dl pthread
ifndef OCPI_TARGET_KERNEL_DIR
  # When we build the driver the kernel should be cloned, checked out
  # with the label consistent with the ISE version, and build there
  export OCPI_TARGET_KERNEL_DIR=$(OCPI_CDK_DIR)/platforms/$(OCPI_TARGET_PLATFORM)/release/kernel-headers
endif
