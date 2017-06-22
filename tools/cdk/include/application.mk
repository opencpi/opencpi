# This file is for an application directory whose name is the name of the app.
#
# The "app" is either the foo.{cc,cxx,cpp} or foo.xml

include $(OCPI_CDK_DIR)/include/util.mk
# Allow old-style "APP" to rename
ifdef APP
OcpiApp:=$(APP)
endif
ifndef OcpiApp
OcpiApp:=$(CwdName)
endif
$(call OcpiIncludeProject)

# The existence of a C++ app file determines if this is an ACI app
OcpiAppCC:=$(strip $(foreach s,cc cxx cpp,$(wildcard $(OcpiApp).$s)))
ifdef OcpiAppCC
  OcpiApps:=$(OcpiApp)
  include $(OCPI_CDK_DIR)/include/aci.mk
  # If we are running in this Makefile, then we are running the TOOL_PLATFORM
  ifndef OcpiRunCC
    OcpiRunCC=$(OcpiRunBefore) $(call AciExe,$(OCPI_TOOL_PLATFORM),$(OcpiApp)) $(OcpiRunArgs) \
              $(OcpiRunAfter)
  endif
  all: aciapps
  ifndef OcpiAppNoRun
    run: all
	$(AT)echo Executing the $(OcpiApp) application.
	$(AT)$(OcpiRunCC)
  endif
else ifneq ($(wildcard $(OcpiApp).xml),)
  ifndef OcpiAppNoRun
    ifndef OcpiRunXML
      OcpiRunXML=$(OcpiRunBefore) $(call AciDir,$(OCPI_TOOL_PLATFORM)/ocpirun $(OcpiRunArgs) $1 \
                 $(OcpiRunAfter)
    endif
    run: all
	$(AT)echo Executing the $(OcpiApp).xml application.
	$(AT)$(call OcpiRunXML,$(OcpiApp).xml)
  endif
else
  $(info No application found when looking for: $(OcpiApp).xml, $(OcpiApp).{cc,cxx,cpp})
  ifndef OcpiAppNoRun
    run:
  endif
endif

clean::
	$(AT)rm -r -f *~ timeData.raw
