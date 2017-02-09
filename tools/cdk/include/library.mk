# The makefile fragment for libraries, perhaps under a project, perhaps not.

include $(OCPI_CDK_DIR)/include/util.mk
# Include project settings for this library, IF it is in a project
# These settings are are available for the project, libraries in the project, and workers
$(OcpiIncludeProject)
# Include library settings for this library, which are available here and for workers
# Thus library settings can depend on project settings
$(call OcpiIncludeLibrary,.,error)

ifndef ProjectPackage
  ProjectPackage:=local
endif
ifdef Package
  ifneq ($(filter .%,$(Package)),)
    Package:=$(ProjectPackage)$(Package)
  endif
else ifeq ($(CwdName),components)
  Package:=$(ProjectPackage)
else
  Package:=$(ProjectPackage).$(CwdName)
endif
include $(OCPI_CDK_DIR)/include/lib.mk

.PHONY: generate run $(OcpiTestGoals)
run: runtest # generic "run" runs test
OldTests=$(foreach i,$(TestImplementations),\
           $(shell [ -f $i/Makefile ] && grep -q '(OCPI_CDK_DIR)/include/test.mk' $i/Makefile || echo $i))
$(filter-out test cleantest,$(OcpiTestGoals)):
	$(AT)set -e; for i in $(filter-out $(OldTests),$(TestImplementations)); do \
	  echo ==============================================================================;\
	  echo ==== Performing goal \"$@\" for unit tests in $$i;\
	  $(MAKE) --no-print-directory -C $$i $@ ; \
	done

# The ordering here assumes HDL cannot depend on RCC.
generate:
	$(call BuildModel,hdl,generate)
	$(call BuildModel,ocl,generate)
	$(call BuildModel,rcc,generate)
	$(call BuildModel,test,generate)
