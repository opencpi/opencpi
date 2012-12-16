# The ocpiIsim platform.
Libraries=bsv util ocpi
# Configurations=....
SourceFiles=fpgaTop_ocpiIsim.v mkSimDCP.v mkSimIO.v main.v
IsimTop=ocpiIsim.main
VerilogDefines= TOP=mkTB18 BSV_TIMESCALE=1ns/1ps
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk # We are a (compiled) core
