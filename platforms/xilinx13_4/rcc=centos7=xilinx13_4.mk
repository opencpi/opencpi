# This file is not version-specific and just relies on its name for OS version
OcpiThisFile:=$(lastword $(MAKEFILE_LIST))
include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
zynq_bin_dir:=$(OcpiXilinxEdkDir)/gnu/arm/lin/bin
zynq_cross_host:=arm-xilinx-linux-gnueabi
file:=$(basename $(notdir $(OcpiThisFile)))
zynq_host:=$(RccTarget_$(word 2,$(subst =, ,$(subst rcc=,,$(file)))))
Gc_$(zynq_host):=$(zynq_bin_dir)/$(zynq_cross_host)-gcc -std=c99
Gc_LINK_$(zynq_host):=$(Gc_$(zynq_host))
Gc++_$(zynq_host):=$(zynq_bin_dir)/$(zynq_cross_host)-g++ -std=c++0x
Gc++_LINK_$(zynq_host):=$(Gc++_$(zynq_host))
