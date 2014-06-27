# These first two should be in the ocpi build already, but FIXME: CDK?
#OCPI_CROSS_BUILD_BIN_DIR=/home/vsi-user/CodeSourcery/Sourcery_CodeBench_Lite_for_Xilinx_GNU_Linux/bin
#OCPI_CROSS_HOST=arm-xilinx-linux-gnueabi
Gc_linux-zynq-arm=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc -std=c99
Gc_LINK_linux-zynq-arm=$(Gc_linux-zynq-arm)
Gc++_linux-zynq-arm=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-g++
Gc++_LINK_linux-zynq-arm=$(G++_linux-zynq-arm)


