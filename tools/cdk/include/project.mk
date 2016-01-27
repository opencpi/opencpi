# This file is the make file for a project

include $(OCPI_CDK_DIR)/include/util.mk

# This is mandatory since it signifies that this directory is a project
ifeq ($(wildcard Project.mk),)
  $(error A project directory must contain a "Project.mk" file.)
endif
$(OcpiIncludeProject)

# As a default, build for everything supported in the CDK
# FIXME: can we test for licensing?
ifeq ($(HdlPlatform)$(HdlPlatforms),)
  export HdlPlatforms=modelsim_pf isim_pf zed ml605 alst4
endif

ifeq ($(wildcard exports),)
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    $(info Exports have never been set up for this project.  Doing it now.)
  endif
  $(info $(shell $(OCPI_CDK_DIR)/scripts/makeExportLinks.sh - $(ProjectPrefix)_ xxx))
endif

include $(OCPI_CDK_DIR)/include/ocpisetup.mk

MaybeMake=$(AT)[ ! -d $1 ] || $(MAKE) -C $1 $2


.PHONY: all applications test clean exports components
.PHONY: hdl hdlassemblies hdlprimitives hdlcomponents hdldevices hdladapters hdlplatforms hdlassemblies hdlportable
all: hdlassemblies applications

hdlassemblies applications: exports

test:
	$(call MaybeMake,components,test)

clean: cleanhdl
	$(call MaybeMake,components,clean)

exports:
	$(AT)$(OCPI_CDK_DIR)/scripts/makeExportLinks.sh $(OCPI_TARGET_DIR) $(ProjectPrefix)_

components: hdlprimitives
	$(call MaybeMake,components,rcc hdl HdlTargets="$(HdlTargets)" HdlPlatforms="$(HdlPlatforms)")
	$(MAKE) exports

hdlprimitives:
	$(call MaybeMake,hdl/primitives)
	$(MAKE) exports

hdlcomponents: hdlprimitives
	$(call MaybeMake,components,hdl)
	$(MAKE) exports

hdldevices: hdlprimitives
	$(call MaybeMake,hdl/devices)

hdladapters: hdlprimitives
	$(call MaybeMake,hdl/adapters)

hdlcards: hdlprimitives
	$(call MaybeMake,hdl/cards)

hdlplatforms: hdldevices hdlcards
	$(call MaybeMake,hdl/platforms)
	$(MAKE) exports

hdlassemblies: components hdlplatforms hdlcards hdladapters
	$(call MaybeMake,hdl/assemblies)

# Everything that does not need platforms
hdlportable: hdlcomponents hdladapters hdldevices hdlcards

hdl: hdlassemblies

cleanhdl:
	$(call MaybeMake,components,cleanhdl)
	$(AT) for d in primitives devices adapters cards platforms assemblies; do \
		[ ! -d hdl/$$d ] || $(MAKE) -C hdl/$$d clean; \
	      done

rcc ocl hdl: exports

rcc:
	$(call MaybeMake,components,rcc)

cleanrcc:
	$(call MaybeMake,components,cleanrcc)

ocl:
	$(call MaybeMake,components,ocl)

cleanocl:
	$(call MaybeMake,components,cleanocl)

cleancomponents:
	$(call MaybeMake,components,clean)

applications: components hdl
	$(call MaybeMake,applications)

cleanapplications:
	$(call MaybeMake,applications,clean)

clean: cleancomponents cleanapplications cleanhdl cleanexports

cleanexports:
	$(AT)rm -r -f exports

cleaneverything: clean
	find . -name '*~' -exec rm {} \;
	find . -depth -name '*.dSym' -exec rm -r {} \;
	find . -depth -name gen -exec rm -r -f {} \;
	find . -depth -name 'target-*' -exec rm -r -f {} \;
	find . -depth -name lib -exec rm -r -f {} \;
