# The zed platform.
# Other aspects are in ml605.mk: those needed for final bitstream
#Cores=pcie_4243_hip_s4gx_gen2_x4_128
XilinxZynqCore=processing_system7_v4_03_a
ifndef OCPI_XILINX_EDK_DIR
OCPI_XILINX_EDK_DIR=$(OCPI_XILINX_TOOLS_DIR)/EDK
endif
XilinxZynq=$(OCPI_XILINX_EDK_DIR)/hw/XilinxProcessorIPLib/pcores/$(XilinxZynqCore)/hdl/verilog
# Since the wrapper file is verilog. we'll provide the verilog PS7
#XilinxPS7=$(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/unisims/PS7.v
XilinxZynqPSFiles=atc aw_atc b_atc processing_system7 trace_buffer w_atc
# system_processing_system7_0_wrapper.v
SourceFiles=$(XilinxPS7) $(XilinxZynqPSFiles:%=$(XilinxZynq)/%.v) zynq_pkg.vhd zed_ps.vhd axi2cp.vhd

Configurations=zed_base
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk
