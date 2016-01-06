# These first two should be in the ocpi build already, but FIXME: CDK?
#OCPI_CROSS_BUILD_BIN_DIR=/home/vsi-user/CodeSourcery/Sourcery_CodeBench_Lite_for_Xilinx_GNU_Linux/bin
#OCPI_CROSS_HOST=arm-xilinx-linux-gnueabi
Gc_linux-xp-mic=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-gcc -std=c99
Gc_LINK_linux-xp-mic=$(Gc_linux-xp-mic)
Gc++_linux-xp-mic=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-g++
Gc++_LINK_linux-xp-mic=$(Gc++_linux-xp-mic)


