# This Makefile is for the primitive library: zynq_ultra

# Set this variable to any other primitive libraries that this library depends on.
# If they are remote from this project, use slashes in the name (relative or absolute)
# If they are in this project, they must be compiled first, and this requires that the
# PrimitiveLibraries variable be set in the hdl/primitives/Makefile such that the
# libraries are in dependency order.
#Libraries=

# Set this variable to the list of source files in dependency order
# If it is not set, all .vhd and .v files will be compiled in wildcard/random order,
# except that any *_pkg.vhd files will be compiled first
#SourceFiles=

# Remember two rules for OpenCPI primitive libraries, in order to be usable with all tools:
# 1. Any entity (VHDL) or module (verilog) must have a VHDL component declaration in zynq_ultra_pkg.vhd
# 2. Entities or modules to be used from outside the library must have the file name
#    be the same as the entity/module name, and one entity/module per file.

OnlyTargets=zynq_ultra xsim isim modelsim
HdlLibraries=fixed_float ocpi platform axi

# See the READMEfor more information regarding the source files listed below
IPGEN_DIR=vivado_zynq_ultra
SourceFiles=zynq_ultra_pkg.vhd $(IPGEN_DIR)/zynq_ultra_ps_e_v3_2_1.v $(IPGEN_DIR)/zynq_ultra_ps_e_0.vhd $(IPGEN_DIR)/zynq_ultra_ps_e_0_pkg.vhd zynq_ultra_ps_e.vhd

include $(OCPI_CDK_DIR)/include/hdl/hdl-library.mk

# Here, we configure and generate the the Vivado IP for the ZynqMP UltraScale+ PS, extract the Verilog interface to it
# (zynq_ultra_ps_e_v3_2_1.v), configure it as it is needed for OpenCPI (choose the AXI ports, disable unneeded features),
# extract the VHDL wrapper that reflects this particular configuration of the PS (zynq_ultra_ps_e_0.vhd), and also extract
# the VHDL component declaration template for the resulting entity. This template is then used to construct
# zynq_ultra_ps_e_0_pkg.vhd.
$(IPGEN_DIR)/zynq_ultra_ps_e_v3_2_1.v $(IPGEN_DIR)/zynq_ultra_ps_e_0.vhd $(IPGEN_DIR)/zynq_ultra_ps_e_0_pkg.vhd:
	$(AT)rm -rf $(IPGEN_DIR)/tmp; \
	     mkdir $(IPGEN_DIR)/tmp; \
	     cd $(IPGEN_DIR)/tmp; \
	     $(call OcpiXilinxVivadoInit); \
	     vivado -mode batch -source ../zynq_ultra_ps_e_gen.tcl > ../zynq_ultra_ps_e_gen.log; \
	     cd ..; \
	     cp tmp/managed_ip_project/managed_ip_project.srcs/sources_1/ip/zynq_ultra_ps_e_0/hdl/zynq_ultra_ps_e_v3_2_1.v \
                tmp/managed_ip_project/managed_ip_project.srcs/sources_1/ip/zynq_ultra_ps_e_0/synth/zynq_ultra_ps_e_0.vhd .; \
	     echo "library IEEE; use IEEE.std_logic_1164.all; use ieee.numeric_std.all;" >> zynq_ultra_ps_e_0_pkg.vhd; \
	     echo "package zynq_ultra_ps_e_0_pkg is" >> zynq_ultra_ps_e_0_pkg.vhd; \
	     awk '/-- COMP_TAG_END ------ End COMPONENT Declaration ------------/{found=0} {if(found) print} /------------- Begin Cut here for COMPONENT Declaration ------ COMP_TAG/{found=1}' tmp/managed_ip_project/managed_ip_project.srcs/sources_1/ip/zynq_ultra_ps_e_0/zynq_ultra_ps_e_0.vho >> zynq_ultra_ps_e_0_pkg.vhd; \
	     echo "end package zynq_ultra_ps_e_0_pkg;" >> zynq_ultra_ps_e_0_pkg.vhd; \
	     rm -rf tmp;


# Do not remove zynq_ultra_ps_e top level verilog on clean
# This will save time and prevent the Tcl IP generate from being run unnecessarily
clean::
	$(AT)rm -rf $(IPGEN_DIR)/tmp; \
	     rm -f $(IPGEN_DIR)/zynq_ultra_ps_e_gen.log;

cleanall:: clean
	$(AT)rm $(IPGEN_DIR)/zynq_ultra_ps_e_v3_2_1.v $(IPGEN_DIR)/zynq_ultra_ps_e_0.vhd; $(IPGEN_DIR)/zynq_ultra_ps_e_0_pkg.vhd
