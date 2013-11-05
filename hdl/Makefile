# Make the hdl assets
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

MyMake=echo ===================$(or $(3),Making) HDL $(1); \
       $(MAKE) --no-print-directory -C $(2)$(1) $(HdlPassTargets)
.PHONY: primitives devices platforms components adapters assemblies
ifndef OCPI_CDK_DIR
export OCPI_CDK_DIR=$(shell pwd)/../ocpi
$(info Setting OCPI_CDK_DIR to $(OCPI_CDK_DIR))
endif
all hdl:assemblies

base: components adapters devices platforms

adapters: primitives
	$(AT)$(call MyMake,adapters)

assemblies: base
	$(AT)$(call MyMake,assemblies)

components: primitives
	$(AT)$(call MyMake,components,../) hdl

devices: primitives
	$(AT)$(call MyMake,devices)

platforms: primitives devices
	$(AT)$(call MyMake,platforms)

primitives:
	$(AT)$(call MyMake,primitives)

clean:
	$(AT)for i in primitives devices platforms; do \
		$(call MyMake,$$i,,Cleaning) clean; \
		done
	$(AT)$(call MyMake,components,../,Cleaning) cleanhdl
	$(AT)$(call MyMake,adapters,,Cleaning) clean
	$(AT)$(call MyMake,assemblies,,Cleaning) clean
