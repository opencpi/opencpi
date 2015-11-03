ifndef _XILINX_MK
_XILINX_MK=1
# Common definition for both isim and xst
# Order: OCPI_XILINX_TOOLS_DIR -> OCPI_XILINX_DIR -> OCPI_XILINX_VERSION -> self-discover
ifndef OCPI_XILINX_TOOLS_DIR
  ifdef OCPI_XILINX_DIR
    XilinxVersions=$(OCPI_XILINX_DIR)/ISE_DS
  else
    ifdef OCPI_XILINX_VERSION
      XilinxVersions=/opt/Xilinx/$(OCPI_XILINX_VERSION)/ISE_DS
    else
      XilinxVersions=$(shell echo $(wildcard /opt/Xilinx/*/ISE_DS) | tr ' ' '\n' | sort -r)
    endif
  endif
  OCPI_XILINX_TOOLS_DIR=$(firstword $(XilinxVersions))
  $(warning Setting OCPI_XILINX_TOOLS_DIR=$(OCPI_XILINX_TOOLS_DIR))
endif

# The "set -e" is to force an error if the settings64.sh has problems
XilinxInit=\
  set -e ; \
  . $(OCPI_XILINX_TOOLS_DIR)/settings64.sh $(and $1,> $1); \
  export LM_LICENSE_FILE=$(OCPI_XILINX_LICENSE_FILE)

VivadoXilinx=\
  set -e ; \
  . $(OCPI_VIVADO_TOOLS_DIR)/.settings64.sh $(OCPI_VIVADO_TOOLS_DIR)

endif
