# The xsim_pf platform.
Libraries=bsv util ocpi
# Configurations=....
SourceFiles=../isim_pf/fpgaTop_isim_pf.v ../isim_pf/mkSimDCP.v ../isim_pf/mkSimIO.v ../isim_pf/main.v
XsimTop=$(CwdName).main
VerilogDefines= TOP=mkTB18 BSV_TIMESCALE=1ns/1ps
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk
