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

ifeq (@,$(AT))
  .SILENT: clean exports components hdlprimitives hdlcomponents hdldevices hdladapters hdlcards hdlplatforms hdlassemblies cleanhdl rcc cleanrcc ocl cleanocl applications run cleancomponents cleanapplications cleanexports cleaneverything $(OcpiTestGoals)
endif

MaybeMake=if [ -d $1 ]; then $(MAKE) -C $1 $2; fi

# Three parameters - $1 is before platform, $2 is after platform, $3 is call to $(MAKE)
MaybeMakePlatforms=\
$(foreach p,$(HdlPlatform) $(HdlPlatforms),\
   echo =============Building platform $p/$2 for $3 &&\
   $(call MaybeMake,$1/$p/$2,$3) &&) true

.PHONY: all applications clean exports components cleanhdl $(OcpiTestGoals)
.PHONY: hdl hdlassemblies hdlprimitives hdlcomponents hdldevices hdladapters hdlplatforms hdlassemblies hdlportable
all: applications

hdlassemblies applications: exports

# Perform test-related goals where they might be found.
DoTests=$(foreach t,\
          components hdl/devices hdl/adapters hdl/cards $(wildcard hdl/platforms/*/devices),\
          $(call MaybeMake,$t,$1) &&) true
$(OcpiTestGoals):
	$(call DoTests,$@)

clean: cleanhdl
	$(call MaybeMake,components,clean)

exports:
	$(OCPI_CDK_DIR)/scripts/makeExportLinks.sh $(OCPI_TARGET_DIR) $(ProjectPrefix)_

components: hdlprimitives
	$(call MaybeMake,components,rcc hdl)
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

hdlplatforms: hdldevices hdlcards hdladapters
	$(call MaybeMake,hdl/platforms)
	$(MAKE) exports

hdlassemblies: hdlcomponents hdlplatforms hdlcards hdladapters
	$(call MaybeMake,hdl/assemblies)

# Everything that does not need platforms
hdlportable: hdlcomponents hdladapters hdldevices hdlcards

hdl: hdlassemblies

cleanhdl:
	$(call MaybeMake,components,cleanhdl)
	for d in primitives devices adapters cards platforms assemblies; do \
	  [ ! -d hdl/$$d ] || $(MAKE) -C hdl/$$d clean; \
	done

rcc ocl hdl: exports

rcc:
	$(call MaybeMake,components,rcc)
	$(call MaybeMake,hdl/devices,rcc)
	$(call MaybeMakePlatforms,hdl/platforms,devices,rcc)

cleanrcc:
	$(call MaybeMake,components,cleanrcc)
	$(call MaybeMake,hdl/devices,cleanrcc)
	$(call MaybeMakePlatforms,hdl/platforms,devices,cleanrcc)

ocl:
	$(call MaybeMake,components,ocl)

cleanocl:
	$(call MaybeMake,components,cleanocl)

applications: rcc hdl
	$(call MaybeMake,applications)

run: all test
	$(call MaybeMake,components,run)
	$(call MaybeMake,applications,run)

cleancomponents:
	$(call MaybeMake,components,clean)

cleanapplications:
	$(call MaybeMake,applications,clean)

clean: cleancomponents cleanapplications cleanrcc cleanhdl cleanexports

cleanexports:
	rm -r -f exports

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
