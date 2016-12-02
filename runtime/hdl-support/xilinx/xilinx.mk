ifndef _XILINX_MK
_XILINX_MK=1
# Functions to retrieve and default pathnames for Xilinx tools
# FIXME: it may be worth caching these singular values
# The functions to retrieve dirs take an optional second argument which is used in place of
# "error" in error messages.  This is used when the caller wants an empty return rather than
# an error

OcpiXilinxDir=$(strip $(foreach t,$(or $(OCPI_XILINX_DIR),/opt/Xilinx),$(infox TT is $t)\
		 $(if $(shell test -d $t && echo 1),$t,\
		    $(call $(or $1,error), Directory "$t", for OCPI_XILINX_DIR, not found))))

OcpiXilinxLicenseFile=$(strip $(foreach t,$(or $(OCPI_XILINX_LICENSE_FILE),\
                                               $(call OcpiXilinxDir,$1)/Xilinx-License.lic),\
			 $(if $(or $(findstring @,$t),$(findstring :,$t),$(shell test -f $t && echo 1)),$t,\
          		    $(call $(or $1,error), File "$t", for OCPI_XILINX_LICENSE_FILE, not found))))

OcpiXilinxIseDir=$(strip\
$(foreach t,$(OcpiXilinxDir),\
  $(foreach i,\
    $(or $(OCPI_XILINX_TOOLS_DIR),\
      $(foreach v,\
        $(if $(filter-out undefined,$(origin OCPI_XILINX_VERSION)),\
          $(foreach e,$(OCPI_XILINX_VERSION),\
            $(if $(shell test -d $t/$e && echo 1),$e,\
              $(call $(or $1,error), Directory "$t/$e", for OCPI_XILINX_VERSION, not found))),\
          $(or $(shell for i in \
                        `shopt -s nullglob && echo $t/*  | tr ' ' '\n' | sort -n -r`; \
                       do \
                         [ -d $$i -a -d $$i/ISE_DS ] && echo `basename $$i` && break; \
                       done),\
            $(call $(or $1,error), No version directory under $t/*/ISE_DS for Xilinx ISE))),\
        $(infox VV:$v)$(call OcpiXilinxDir,$1)/$v/ISE_DS)),\
    $(infox II:$i.)\
    $(if $(shell test -d $i && echo 1),$i,\
      $(call $(or $1,error), Directory "$i", in OCPI_XILINX_TOOLS_DIR for ISE, not found)))))

OcpiXilinxEdkDir=$(strip\
 $(foreach d,$(or $(OCPI_XILINX_EDK_DIR),$(call OcpiXilinxIseDir,$1)/EDK),\
   $(infox EDK:$d)$(if $(shell test -d $d && echo 1),$d,\
                     $(call $(or $1,error), Directory "$d", for OCPI_XILINX_EDK_DIR, not found))))

# The "set -e" is to force an error if the settings64.sh has problems
OcpiXilinxIseInit=\
  set -e ; \
  . $(OcpiXilinxIseDir)/settings64.sh $(and $1,> $1); \
  export LM_LICENSE_FILE=$(OcpiXilinxLicenseFile)

OcpiXilinxVivadoDir=$(strip\
$(foreach t,$(OcpiXilinxDir)/Vivado,$(infox vt:$t)\
  $(foreach i,\
    $(or $(OCPI_XILINX_VIVADO_DIR),\
      $(foreach v,\
        $(if $(filter-out undefined,$(origin OCPI_XILINX_VIVADO_VERSION)),\
          $(foreach e,$(OCPI_XILINX_VIVADO_VERSION),\
            $(if $(shell test -d $t/$e && echo 1),$e,\
              $(call $(or $1,error), Directory "$t/$e", for OCPI_XILINX_VIVADO_VERSION, not found))),\
          $(or $(shell for i in \
                        `shopt -s nullglob && echo $t/*  | tr ' ' '\n' | sort -n -r`; \
                       do \
                         [ -d $$i -a -r $$i/settings64.sh ] && echo `basename $$i` && break; \
                       done),\
            $(call $(or $1,error), No version directory under $t/* for Xilinx Vivado))),\
        $(infox VV:$v)$(call OcpiXilinxDir,$1)/Vivado/$v)),\
    $(infox II:$i.)\
    $(if $(shell test -d $i && echo 1),$i,\
      $(call $(or $1,error), Directory "$i", in OCPI_XILINX_VIVADO_DIR, not found)))))

OcpiXilinxVivadoLicenseFile=$(strip\
  $(foreach t,$(or $(OCPI_XILINX_VIVADO_LICENSE_FILE),\
                $(call OcpiXilinxDir,$1)/Vivado/Xilinx-License.lic),\
    $(if $(or $(findstring @,$t),$(findstring :,$t),$(shell test -f $t && echo 1)),$t,\
      $($(or $1,error) File "$t", for OCPI_XILINX_VIVADO_LICENSE_FILE, not found))))

OcpiXilinxVivadoInit=\
  set -evx ; \
  . $(OcpiXilinxVivadoDir)/settings64.sh $(OcpiXilinxVivadoDir)

# emit shell assignments - allowing errors etc.
ifdef ShellIseVars
all:
  $(info OcpiXilinxIseDir=$(OcpiXilinxIseDir);\
         OcpiXilinxLicenseFile=$(OcpiXilinxLicenseFile))
endif

endif
