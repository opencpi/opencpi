# ML605 make variable settings
PART = xc6vlx240t-1-ff1156
CSIN = ml605_1102.cdc
PROMARGS = -x xcf128x -data_width 16 -u 00000000 
# PLATFORM_CORES=dram_mig33_v6 pcie_v6_x4_250 temac_v6
# PLATFORM_CORES= pcie_v6_x4_250 temac_v6
#PLATFORM_RTLS= \
# $(PlatformSpecDir)/fpgaTop_v6.v \
# $(PlatformSpecDir)/mkFTopV6.v \

# Platform devices are optional elements
#PLATFORM_DEVICES= \
#  $(OCPI_HDL_IMPORTS_DIR)/rtl/mkGbeWorker.v

# dram left out for the moment
# should flash go here?
