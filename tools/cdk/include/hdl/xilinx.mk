# Common definition for both isim and xst
ifndef OCPI_XILINX_TOOLS_DIR
XilinxVersions=$(shell echo $(wildcard /opt/Xilinx/*/ISE_DS) | tr ' ' '\n' | sort -r)
OCPI_XILINX_TOOLS_DIR=$(firstword $(XilinxVersions))
endif
# The "set -e" is to force an error if the settings64.sh has problems
XilinxInit=\
  set -e ; \
  . $(OCPI_XILINX_TOOLS_DIR)/settings64.sh $(and $1,> $1); \
  export LM_LICENSE_FILE=$(OCPI_XILINX_LICENSE_FILE)

VivadoXilinx=\
  set -e ; \
  . $(OCPI_VIVADO_TOOLS_DIR)/.settings64.sh $(OCPI_VIVADO_TOOLS_DIR) \
