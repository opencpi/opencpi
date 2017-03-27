# This file is for an application directory whose name is the name of the app.
#
# The "app" is either the foo.cxx, foo.cc or foo.xml
include $(OCPI_CDK_DIR)/include/util.mk
OcpiApp:=$(CwdName)
$(call OcpiIncludeProject)

# If not cleaning, we don't need to be target-specific etc.
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  include $(OCPI_CDK_DIR)/include/ocpisetup.mk
endif
OcpiAppCC:=$(wildcard $(OcpiApp).cxx)$(wildcard $(OcpiApp).cc)
ifdef OcpiAppCC

  ifndef OcpiRunCC
    OcpiRunCC=$(OcpiRunBefore) $(OcpiProg) $(OcpiRunArgs) $(OcpiRunAfter)
  endif
  OcpiIncs=-I$(OCPI_INC_DIR)
  OcpiDir=target-$(OCPI_TARGET_DIR)
  OcpiProg=$(OcpiDir)/$(OcpiApp)
  all : $(OcpiProg)
  $(OcpiProg): $(OcpiAppCC) | $(OcpiDir)
	$(AT)echo Building $@...
	$(AT)$(CXX) -g -Wall $(OCPI_EXPORT_DYNAMIC) -o $@ $(OcpiIncs) $^ $(OCPI_LD_FLAGS)

  run: all
	$(AT)$(OcpiRunCC)

else ifneq ($(wildcard $(OcpiApp).xml),)
  ifndef OcpiRunXML
    OcpiRunXML=$(OcpiRunBefore) $(OCPI_BIN_DIR)/ocpirun $(OcpiRunArgs) $1 $(OcpiRunAfter)
  endif

  run: all
	$(AT)$(call OcpiRunXML,$(OcpiApp).xml)

else
  $(info No application found when looking for: $(OcpiApp).xml, $(OcpiApp).cc, or $(OcpiApp).cxx)
  run:
endif

clean::
	$(AT)rm -r -f lib target-* *.*~ timeData.raw
