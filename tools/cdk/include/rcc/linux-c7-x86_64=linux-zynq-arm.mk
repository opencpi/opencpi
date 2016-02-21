zynq_bin_dir=$(OCPI_XILINX_EDK_DIR)/gnu/arm/lin/bin
zynq_cross_host=arm-xilinx-linux-gnueabi
Gc_linux-zynq-arm=$(zynq_bin_dir)/$(zynq_cross_host)-gcc -std=c99
Gc_LINK_linux-zynq-arm=$(Gc_linux-zynq-arm)
Gc++_linux-zynq-arm=$(zynq_bin_dir)/$(zynq_cross_host)-g++
Gc++_LINK_linux-zynq-arm=$(Gc++_linux-zynq-arm)


