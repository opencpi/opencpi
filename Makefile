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

OnlyTargets=zynq_u9eg zynq_u28dr xsim isim modelsim
HdlLibraries=fixed_float ocpi platform axi

SourceFiles=zynq_ultra_pkg.vhd vivado_zynq_ultra/zynq_ultra_ps_e_v3_2_1.v zynq_ultra_ps_e.vhd

include $(OCPI_CDK_DIR)/include/hdl/hdl-library.mk

# In order to generate the zynq_ultra_ps_e top verilog file with Vivado,
# we need to run a series of TCL commands located in vivado_zynq_ultra/tcl
vivado_zynq_ultra/zynq_ultra_ps_e_v3_2_1.v:
	$(AT)rm -rf vivado_zynq_ultra/tmp; \
	     mkdir vivado_zynq_ultra/tmp; \
	     cd vivado_zynq_ultra/tmp; \
	     $(call OcpiXilinxVivadoInit); \
	     vivado -mode batch -source ../zynq_ultra_ps_e_gen.tcl > ../zynq_ultra_ps_e_gen.log; \
	     cd ..; \
	     cp tmp/managed_ip_project/managed_ip_project.srcs/sources_1/ip/zynq_ultra_ps_e_0/hdl/zynq_ultra_ps_e_v3_2_1.v . ;\
	     rm -rf tmp;

# Do not remove zynq_ultra_ps_e top level verilog on clean
# This will save time
clean::
	$(AT)rm -rf vivado_zynq_ultra/tmp; \
	     rm -f vivado_zynq_ultra/zynq_ultra_ps_e_gen.log;
