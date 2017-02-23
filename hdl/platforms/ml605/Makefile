# The ml605 platform.
Libraries=virtex6 xilinx
Cores=pcie_4243_trn_v6_gtx_x4_250
#Cores=pcie_4243_trn_v6_gtx_x4_250 ./chipscope_icon ./chipscope_ila
ExportFiles=jtagSupport_ml605 loadFlash_ml605 \
            ml605.ucf ml605.ut ml605.mk testbias_ml605_base.bitz
# We specify these to get the order we want
SourceFiles= ml605_pkg.vhd ml605.vhd pci_ml605.v 
#SourceFiles=chipscope_pkg.vhd ml605_pkg.vhd ml605.vhd pci_ml605.v 
Configurations=base ml605_flash ml605_lime_tx \
               ml605_zipper_fmc_lpc_rx_tx \
               ml605_zipper_fmc_lpc_rx \
               ml605_zipper_fmc_lpc_tx \
               ml605_zipper_fmc_hpc_rx_tx \
               ml605_zipper_fmc_hpc_rx \
               ml605_zipper_fmc_hpc_tx
include $(OCPI_CDK_DIR)/include/hdl/hdl-platform.mk
