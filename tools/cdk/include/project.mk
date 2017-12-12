# This file is protected by Copyright. Please refer to the COPYRIGHT file
# distributed with this source distribution.
#
# This file is part of OpenCPI <http://www.opencpi.org>
#
# OpenCPI is free software: you can redistribute it and/or modify it under the
# terms of the GNU Lesser General Public License as published by the Free
# Software Foundation, either version 3 of the License, or (at your option) any
# later version.
#
# OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
# WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
# A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
# details.
#
# You should have received a copy of the GNU Lesser General Public License along
# with this program. If not, see <http://www.gnu.org/licenses/>.

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
  ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter projectpackage,$(MAKECMDGOALS)),)
    include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk
    ifeq ($(findstring export,$(MAKECMDGOALS)),)
      $(info No HDL platforms specified.  No HDL assets will be targeted.)
      $(info Possible HdlPlatforms are: $(sort $(HdlAllPlatforms)).)
    endif
  endif
endif

ifeq ($(wildcard imports),)
  ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter projectpackage,$(MAKECMDGOALS)),)
    doimports=$(shell \
	[ -d imports ] || mkdir imports; \
	for i in $(CollectCreatableImports); do \
	  [ -L imports/$$(basename $$i) ] || ln -s $$i imports/; \
	done; \
	[ -L imports/ocpi.cdk ] || ln -s $(OCPI_CDK_DIR) imports/ocpi.cdk;)
    $(info Imports are not set up for this project.  Doing it now. $(doimports))
  else
    # we are assuming that imports are not required for any clean goal.
    # $(nuthin $(doimports))
  endif
endif

ifeq ($(wildcard exports)$(filter projectpackage,$(MAKECMDGOALS)),)
  doexports=$(shell $(OCPI_CDK_DIR)/scripts/makeExportLinks.sh - $(ProjectPrefix)_ $(ProjectPackage) xxx)
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    $(info Exports are not set up for this project.  Doing it now. $(doexports))
  else
    # we are assuming that exports are not required for any clean goal.
    # $(nuthin $(doexports))
  endif
endif

# Do not want to import ocpisetup.mk if all we are doing is exporting project variables to python/bash
ifeq ($(filter imports exports cleanimports cleanexports projectpackage,$(MAKECMDGOALS)),)
  include $(OCPI_CDK_DIR)/include/ocpisetup.mk
endif

ifeq (@,$(AT))
  .SILENT: clean imports exports components hdlprimitives hdlcomponents hdldevices hdladapters hdlcards hdlplatforms hdlassemblies cleanhdl rcc cleanrcc ocl cleanocl applications run cleancomponents cleanapplications cleanimports cleanexports cleaneverything $(OcpiTestGoals)
endif

MaybeMake=if [ -d $1 ]; then $(MAKE) -C $1 $2; fi

# Three parameters - $1 is before platform, $2 is after platform, $3 is call to $(MAKE)
MaybeMakePlatforms=\
$(foreach p,$(HdlPlatform) $(HdlPlatforms),\
   echo =============Building platform $p/$2 for $3 &&\
   $(call MaybeMake,$1/$p/$2,$3) &&) true

.PHONY: all applications clean imports exports components cleanhdl $(OcpiTestGoals)
.PHONY: hdl hdlassemblies hdlprimitives hdlcomponents hdldevices hdladapters hdlplatforms hdlassemblies hdlportable
all: applications

hdlassemblies applications: imports exports

# Perform test-related goals where they might be found.
DoTests=$(foreach t,\
          components hdl/devices hdl/adapters hdl/cards $(wildcard hdl/platforms/*/devices),\
          $(call MaybeMake,$t,$1) &&) true
$(OcpiTestGoals):
	$(call DoTests,$@)

CollectCreatableImports=$(strip $(infox OGCrI)\
  $(foreach p,$(wildcard $(OcpiProjectRegistryDir)/*),\
    $(if $(strip \
      $(or \
        $(filter $(realpath $p),$(realpath .)),\
        $(foreach i,$(OcpiGetProjectImports),\
          $(filter $(notdir $p),$(notdir $i))))),,\
      $p )))

# Make the imports directory if it does not exist.
# For any projects that do not already have a manually created
# link in imports/ create symlinks to project registry directory.
imports:
	[ -d imports ] || mkdir imports
	for i in $(CollectCreatableImports); do \
	  [ -L imports/$$(basename $$i) ] || ln -s $$i imports/; \
	done
	[ -L imports/ocpi.cdk ] || ln -s $(OCPI_CDK_DIR) imports/ocpi.cdk; \

exports:
	$(OCPI_CDK_DIR)/scripts/makeExportLinks.sh $(OCPI_TARGET_DIR) $(ProjectPrefix)_ $(ProjectPackage)

components: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,components,rcc hdl)
	$(MAKE) exports

hdlprimitives:
	$(MAKE) imports
	$(call MaybeMake,hdl/primitives)
	$(MAKE) exports

hdlcomponents: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,components,hdl)
	$(MAKE) exports

hdldevices: hdlprimitives
	$(call MaybeMake,hdl/devices)

hdladapters: hdlprimitives
	$(call MaybeMake,hdl/adapters)

hdlcards: hdlprimitives
	$(call MaybeMake,hdl/cards)

hdlplatforms: hdldevices hdlcards hdladapters
	$(MAKE) imports
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

rcc ocl hdl: imports exports

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

# Note that imports must be cleaned last because the host rcc platform directory
# needs to be accessible via imports for projects other than core
# (e.g. for cleaning rcc)
clean: cleancomponents cleanapplications cleanrcc cleanhdl cleanexports cleanimports

# Iterate through symlinks in imports. If the link points to the project registry dir,
# it is the CDK, or is a broken link, it can be cleaned/removed. If the imports directory
# is empty after clean, the whole directory can be removed.
cleanimports:
	for i in $(OcpiGetProjectImports) ; do \
	  if [ -L "$$i" ]; then \
	    if [[ "$$(dirname $(call SymLinkContents,$$i))" == "$(OcpiProjectRegistryDir)" \
	          || "$$(basename $$i)" == ocpi.cdk ]]; then \
	      rm "$$i"; \
	    fi; \
	  fi; \
	done; \
	if [ -e "$(OcpiImportsDirForContainingProject)" ]; then \
	  for i in $(OcpiImportsDirForContainingProject)/* ; do \
	    if [[ -L "$$i" && ! -e "$$i" ]]; then \
	        rm "$$i"; \
	    fi; \
	  done; \
	fi; \
	[ ! -d imports ] || [ "$$(ls -A imports)" ] || rm -r imports

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
  ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter projectpackage,$(MAKECMDGOALS)),)
    specs/package-id: Project.mk
	$(AT)echo "$(ProjectPackage)" > specs/package-id
    components: specs/package-id
  endif
endif

ifdef ShellProjectVars
projectpackage:
$(info ProjectPackage="$(ProjectPackage)";)
endif
