ifndef _XILINX_VIVADO_MK
_XILINX_VIVADO_MK=1
# Common definition for both xsim and vivado
# While the environment may be set up, it may not be and we should do proper defaulting
# right here.
# Note we don't set anything in the environment except what is needed:
# OCPI_XILINX_VIVAO_TOOLS_DIR
# OCPI_XILINX_LICENSE_FILE

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  # use the script to find ISE that is shared with other tools
  $(and $(call DoShell,$(OCPI_CDK_DIR)/scripts/xilinx-vivado.sh,XilinxDir),$(error $(XilinxDir)))
  export OCPI_XILINX_VIVADO_TOOLS_DIR:=$(XilinxDir)
  # Find a license file
  ifndef OCPI_XILINX_VIVADO_LICENSE_FILE
   ifndef OCPI_XILINX_DIR
    OCPI_XILINX_DIR=/opt/Xilinx
   endif
   export OCPI_XILINX_VIVADO_LICENSE_FILE:=$(OCPI_XILINX_DIR)/Xilinx-License.lic
  endif
  ifeq ($(call OcpiExists,$(OCPI_XILINX_VIVADO_LICENSE_FILE)),)
   $(error Xilinx Vivado license file, not found at $(OCPI_XILINX_VIVADO_LICENSE_FILE))
  endif
endif

XilinxVivadoInit=\
  set -evx ; \
  . $(OCPI_XILINX_VIVADO_TOOLS_DIR)/settings64.sh $(OCPI_XILINX_VIVADO_TOOLS_DIR)

endif
