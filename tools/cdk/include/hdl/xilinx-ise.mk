ifndef _XILINX_ISE_MK
_XILINX_ISE_MK=1
# Common definition for both isim and xst
# While the environment may be set up, it may not be and we should do proper defaulting
# right here.  The priority should be consistent with the env/xilinx.sh script
# Order: OCPI_XILINX_TOOLS_DIR -> OCPI_XILINX_DIR -> OCPI_XILINX_VERSION -> self-discover
# Note we don't set anything in the environment except what is needed:
# OCPI_XILINX_TOOLS_DIR
# OCPI_XILINX_LICENSE_FILE

ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  # use the script to find ISE that is shared with other tools
  $(and $(call DoShell,$(OCPI_CDK_DIR)/scripts/xilinx-ise.sh,XilinxIseDir),\
        $(error $(XilinxIseDir)))
  export OCPI_XILINX_TOOLS_DIR:=$(XilinxIseDir)
  # Find a license file FIXME: maybe don't bother for isim?
  ifndef OCPI_XILINX_LICENSE_FILE
   ifndef OCPI_XILINX_DIR
    OCPI_XILINX_DIR=/opt/Xilinx
   endif
   export OCPI_XILINX_LICENSE_FILE:=$(OCPI_XILINX_DIR)/Xilinx-License.lic
  endif
  ifeq ($(or $(findstring @,$(OCPI_XILINX_LICENSE_FILE)),$(findstring :,$(OCPI_XILINX_LICENSE_FILE)),$(call OcpiExists,$(OCPI_XILINX_LICENSE_FILE))),)
   $(error Xilinx license file not found at $(OCPI_XILINX_LICENSE_FILE))
  endif
endif
# The "set -e" is to force an error if the settings64.sh has problems
XilinxInit=\
  set -e ; \
  . $(OCPI_XILINX_TOOLS_DIR)/settings64.sh $(and $1,> $1); \
  export LM_LICENSE_FILE=$(OCPI_XILINX_LICENSE_FILE)
endif
