include $(OCPI_CDK_DIR)/include/util.mk
include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
zynq_bin_dir=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin
ifdef OCPI_CROSS_BUILD_BIN_DIR
  ifneq ($(zynq_bin_dir),$(OCPI_CROSS_BUILD_BIN_DIR))
    $(warning Overriding default zynq_bin_dir [$(zynq_bin_dir)] with [$(OCPI_CROSS_BUILD_BIN_DIR)])
    zynq_bin_dir=$(OCPI_CROSS_BUILD_BIN_DIR)
  endif
endif
zynq_cross_host=arm-xilinx-linux-gnueabi
Gc_linux-zynq-arm=$(zynq_bin_dir)/$(zynq_cross_host)-gcc -std=c99
Gc_LINK_linux-zynq-arm=$(Gc_linux-zynq-arm)
Gc++_linux-zynq-arm=$(zynq_bin_dir)/$(zynq_cross_host)-g++
Gc++_LINK_linux-zynq-arm=$(Gc++_linux-zynq-arm)
