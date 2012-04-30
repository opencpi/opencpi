ifdef NOT
This example shows a simple xml-based application with a command line option to
specify the implementation selection expression for the middle "bias" worker.
To run the HDL version of bias you must:
2. run "make runhdl2", which chooses the HDL bias worker since it is first in the search path
3. run "make runhdl1 ARGS='model=="hdl"', which forces the use of hdl even though it is not
   first in the search path. (note the double == in the correct syntax).
4. run "make runhdl1" will enable the HDL container, but use it since it is not first in the path
   i.e. an rcc implementation of bias is found first

As the bitstream loading is not yet completely automated, before doing this you must use:
". ocpi/ocpisetup.sh" to set up paths to access utilities
"sudo ocfrp_check" to find the FPGA boards on PCI Express - e.g. 0000:05:00.0
"probeXilinxUsb all" to probe for the jtag port, usually says usb21
"loadBitStream hdl/applications/testbias/target-ml605/testbias-ml605.bit.gz 0000:05:00.0 usb21 xc6vlx240t"
  to load the bitstream on the device
"cp hdl/applications/testbias/target-ml605/testbias-ml605.bit.gz components/lib/hdl"
  to put the bitstream file in the path used by this makefile

endif
ifndef OCPI_CDK_DIR
# if not set, assume we are either in the development tree or in an exported cdk
OCPI_CDK_DIR=$(or $(wildcard ../../export),../..)
endif

include $(OCPI_CDK_DIR)/ocpisetup.mk

all: biasFile

test.input:
	g++ -Wall mkTestFile.cxx
	./a.out > test.input

biasFile: biasFile.cxx
	g++ -Wall -g -o $@ -I$(OCPI_INC_DIR) $^ $(OCPI_LD_FLAGS)


run: all test.input
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc ./biasFile $(ARGS) > /dev/null	
	od -X test.output

runhdl1: all test.input
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/rcc:$(OCPI_CDK_DIR)/lib/components/hdl sudo -E ./biasFile '$(ARGS)'
	od -X test.output

runhdl2: all test.input
	OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR)/lib/components/hdl:$(OCPI_CDK_DIR)/lib/components/rcc sudo -E ./biasFile '$(ARGS)'
	od -X test.output

clean::
	rm -f -r biasFile biasFile.dSYM test.output test.input a.out
