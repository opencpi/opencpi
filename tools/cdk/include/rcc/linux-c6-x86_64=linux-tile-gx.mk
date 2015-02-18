# These first two should be in the ocpi build already, but FIXME: CDK?
#OCPI_CROSS_BUILD_BIN_DIR=/home/vsi-user/CodeSourcery/Sourcery_CodeBench_Lite_for_Xilinx_GNU_Linux/bin
#OCPI_CROSS_HOST=arm-xilinx-linux-gnueabi
Gc_linux-tile-gx=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc -std=c99
Gc_LINK_linux-tile-gx=$(Gc_linux-tile-gx)
Gc++_linux-tile-gx=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-g++
Gc++_LINK_linux-tile-gx=$(Gc++_linux-tile-gx)




