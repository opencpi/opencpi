HdlLibraries=bsv fixed_float ocpi util
# We can't successfully elaborate this since it references the metadata stuff
HdlNoElaboration=yes
# FIXME FIXME: The low level primitives (FIFO etc.) are COPIED here due to an XST bug
# that doesn't seem to find verilog modules in libraries when the elaboration is vhdl->verilog->verilog...
# mkOCCP.v occp_rv.vhd
SourceFiles=\
platform_pkg.vhd platform_body.vhd \
time_client_rv.vhd \
mkTLPSerializer.v unoc_cp_adapter.vhd metadata_rv.vhd sim_dcp.vhd sim_clk.vhd \
wci_master.vhd ocscp_rv.vhd \
mkPktFork.v mkPktMerge.v mkTLPSM.v unoc_node_rv.vhd unoc_terminator.vhd \
mkSimIO.v mkSimDCP.v \
FIFO1.v FIFO2.v MakeResetA.v SyncResetA.v SyncHandshake.v arSRLFIFOD.v
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk
