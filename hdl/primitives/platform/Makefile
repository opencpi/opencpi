HdlLibraries=ocpi bsv
# We can't successfully elaborate this since it references the metadata stuff
HdlNoElaboration=yes
# FIXME FIXME: The low level primitives (FIFO etc.) are COPIED here due to an XST bug
# that doesn't seem to find verilog modules in libraries when the elaboration is vhdl->verilog->verilog...
SourceFiles=\
platform_pkg.vhd mkOCCP.v occp_rv.vhd timeService.v time_server.vhd time_client_rv.vhd \
mkTLPSerializer.v unoc_cp_adapter.vhd metadata_rv.vhd \
mkPktFork.v mkPktMerge.v mkTLPSM.v unoc_node_rv.vhd unoc_terminator.vhd \
mkTimeClient.v \
mkSimIO.v mkSimDCP.v \
FIFO1.v FIFO2.v MakeResetA.v SyncResetA.v SyncRegister.v SyncHandshake.v

include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk
