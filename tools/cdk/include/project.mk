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
  doexports=$(shell $(OCPI_CDK_DIR)/scripts/makeExportLinks.sh - $(ProjectPrefix)_ xxx)
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    $(info Exports are not set up for this project.  Doing it now. $(doexports))
  else
    # we are assuming that exports are not required for any clean goal.
    # $(nuthin $(doexports))
  endif
endif

include $(OCPI_CDK_DIR)/include/ocpisetup.mk

MaybeMake=$(AT)[ ! -d $1 ] || $(MAKE) -C $1 $2


.PHONY: all applications test clean exports components
.PHONY: hdl hdlassemblies hdlprimitives hdlcomponents hdldevices hdladapters hdlplatforms hdlassemblies hdlportable
all: hdlassemblies applications

hdlassemblies applications: exports

# Build tests where they might be found.
test:
	$(call MaybeMake,components,test)
	$(call MaybeMake,hdl/devices,test)
	$(call MaybeMake,hdl/platforms,test)

clean: cleanhdl
	$(call MaybeMake,components,clean)

exports:
	$(AT)$(OCPI_CDK_DIR)/scripts/makeExportLinks.sh $(OCPI_TARGET_DIR) $(ProjectPrefix)_

components: hdlprimitives
	$(call MaybeMake,components,rcc hdl HdlTargets="$(HdlTargets)" HdlPlatforms="$(HdlPlatforms)")
	$(AT)$(MAKE) exports

hdlprimitives:
	$(call MaybeMake,hdl/primitives)
	$(AT)$(MAKE) exports

hdlcomponents: hdlprimitives
	$(call MaybeMake,components,hdl)
	$(AT)$(MAKE) exports

hdldevices: hdlprimitives
	$(call MaybeMake,hdl/devices)

hdladapters: hdlprimitives
	$(call MaybeMake,hdl/adapters)

hdlcards: hdlprimitives
	$(call MaybeMake,hdl/cards)

hdlplatforms: hdldevices hdlcards
	$(call MaybeMake,hdl/platforms)
	$(AT)$(MAKE) exports

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

applications: components hdl
	$(call MaybeMake,applications)

run: all test
	$(call MaybeMake,components,run)
	$(call MaybeMake,applications,run)

cleancomponents:
	$(call MaybeMake,components,clean)

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

# Package issue - if we have a top level specs directory, we must make the
# associate package name available to anything that includes it, both within the
# project and outside it (when this project is accessed via OCPI_PROJECT_PATH)

ifneq ($(wildcard specs),)
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    specs/package-name: Project.mk
	$(AT)echo "$(ProjectPackage)" > specs/package-name
    components: specs/package-name
  endif
endif
