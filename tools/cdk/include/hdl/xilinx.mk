ifndef _XILINX_MK
_XILINX_MK=1
# Common definition for both isim and xst
# Order: OCPI_XILINX_TOOLS_DIR -> OCPI_XILINX_DIR -> OCPI_XILINX_VERSION -> self-discover
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
ifndef OCPI_XILINX_DIR
  export OCPI_XILINX_DIR=/opt/Xilinx
endif
ifndef OCPI_XILINX_VERSION
  XilinxVersions:=$(shell echo $(wildcard $(OCPI_XILINX_DIR)/*/ISE_DS) | tr ' ' '\n' | sort -r)
  export OCPI_XILINX_VERSION:=$(notdir $(patsubst %/,%,$(dir $(firstword $(XilinxVersions)))))
endif
ifndef OCPI_XILINX_LICENSE_FILE
 export OCPI_XILINX_LICENSE_FILE=$(OCPI_XILINX_DIR)/Xilinx-License.lic
endif
ifndef OCPI_XILINX_TOOLS_DIR
  export OCPI_XILINX_TOOLS_DIR=$(OCPI_XILINX_DIR)/$(OCPI_XILINX_VERSION)/ISE_DS
  $(info Warning: Setting OCPI_XILINX_TOOLS_DIR=$(OCPI_XILINX_TOOLS_DIR))
endif
ifeq ($(wildcard $(OCPI_XILINX_TOOLS_DIR)),)
  export OCPI_XILINX_TOOLS_DIR=$(OCPI_XILINX_DIR)/$(OCPI_XILINX_VERSION)/LabTools
endif
ifndef OCPI_VIVADO_TOOLS_DIR
  XilinxVersions:=$(shell echo $(wildcard $(OCPI_XILINX_DIR)/Vivado/*) | tr ' ' '\n' | sort -r)
  export OCPI_VIVADO_VERSION:=$(notdir $(firstword $(XilinxVersions)))
  export OCPI_VIVADO_TOOLS_DIR=$(OCPI_XILINX_DIR)/Vivado/$(OCPI_VIVADO_VERSION)
endif
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
