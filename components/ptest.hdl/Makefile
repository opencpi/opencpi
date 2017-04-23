# Put Makefile customizations for worker ptest.hdl here:
#
# XSIM crashes on this:
#ERROR: [XSIM 43-3316] Signal SIGSEGV received.
#Error: xsim failed(1). See target-xsim/ptest-xsim.out.  0:02.25 at 07:20:52
#Printing stacktrace...
#
#[0] /opt/Xilinx/Vivado/2015.4/bin/unwrapped/lnx64.o/xelab() [0x66d73d]
#[1] /opt/Xilinx/Vivado/2015.4/lib/lnx64.o/libxsimverific.so(Verific::VhdlVisitor::TraverseArray(Verific::Array const*)+0x46) [0x7f2a563ad586]
#[2] /opt/Xilinx/Vivado/2015.4/bin/unwrapped/lnx64.o/xelab() [0x66ba50]
#[3] /opt/Xilinx/Vivado/2015.4/bin/unwrapped/lnx64.o/xelab() [0x66cd4a]
#[4] /opt/Xilinx/Vivado/2015.4/lib/lnx64.o/libxsimverific.so(Verific::VhdlVisitor::TraverseArray(Verific::Array const*)+0x46) [0x7f2a563ad586]
#ExcludeTargets=xsim
include $(OCPI_CDK_DIR)/include/worker.mk
