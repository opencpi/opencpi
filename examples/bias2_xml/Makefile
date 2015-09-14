ifdef NOT
This example shows a simple xml-based application with a command line option to
specify the implementation selection expression for the middle "bias" workers.
To run the HDL version of bias you must:
2. run "make runhdl2", which chooses the HDL bias workers since it is first in the search path
3. run "make runhdl1 ARGS='model==\"hdl\"', which forces the use of hdl even though it is not
   first in the search path. (note the double == is the correct syntax).
4. run "make runhdl1" will enable the HDL container, but won't use it since it is not first in the path
   i.e. an rcc implementation of bias is found first
5. run "make runhdl1 ARGS='model==\"hdl\" model==\"rcc\"' to force hdl or rcc etc.
As the bitstream loading is not yet completely automated, before doing this you must use:
". ocpi/ocpisetup.sh" to set up paths to access utilities
"sudo ocfrp_check" to find the FPGA boards on PCI Express - e.g. 0000:05:00.0
"probeXilinxUsb all" to probe for the jtag port, usually says usb21
"loadBitStream hdl/applications/testbias/target-ml605/testbias-ml605.bit.gz 0000:05:00.0 usb21 xc6vlx240t"
  to load the bitstream on the device
"cp hdl/applications/testbias/target-ml605/testbias-ml605.bit.gz components/lib/hdl"
"cp hdl/applications/testbias2/target-ml605/testbias2-ml605.bit.gz components/lib/hdl"
  to put the bitstreams files in the path used by this makefile

endif
APP=biasFile
include ../setup.mk

good.output test.input: mkTestFile.cxx
	g++ -Wall -o $(DIR)/a.out mkTestFile.cxx
	$(DIR)/a.out $(FILE_SIZE) > test.input 2> good.output

run: all test.input
	$(VG) $(PROG) $(ARGS) > /dev/null	
	cmp test.output good.output

runxml: all test.input
	$(VG) ocpirun -d -v $(ARGS) bias2.xml > /dev/null
	cmp test.output good.output

runhdl1: all test.input
	$(PROGS) $(ARGS)
	cmp test.output good.output

runhdl2: all test.input
	$(PROGS) $(ARGS)
	cmp test.output good.output

clean::
	rm -f -r test.output test.input good.output
