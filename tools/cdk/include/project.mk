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
# FIXME: Error message makes no sense if give hdltargets but not platforms
ifeq ($(HdlPlatform)$(HdlPlatforms),)
  ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter projectpackage,$(MAKECMDGOALS)),)
    include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk
    ifeq ($(findstring export,$(MAKECMDGOALS))$(findstring import,$(MAKECMDGOALS)),)
      $(info No HDL platforms specified.  No HDL assets will be targeted.)
      $(info Possible HdlPlatforms are: $(sort $(HdlAllPlatforms)).)
    endif
  endif
endif

# imports need to be created before ocpisetup.mk no matter what
ifeq ($(filter imports projectpackage,$(MAKECMDGOALS)),)
  doimports=$(shell $(OcpiExportVars) $(MAKE) imports NoExports=1)
  ifeq ($(wildcard imports),)
    $(info Imports are not set up for this project.  Doing it now.)
    $(info $(doimports))
  else
    # If the imports already exist, we still want to make sure they are up to date
    $(infox Updating imports. $(doimports))
  endif
endif

ifeq ($(NoExports)$(wildcard exports)$(filter projectpackage,$(MAKECMDGOALS)),)
  doexports=$(OcpiExportVars) $(OCPI_CDK_DIR)/scripts/makeProjectExports.sh - $(ProjectPackage) xxx
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    $(info Exports are not set up for this project.  Doing it now.)
    $(info $(shell $(doexports)))
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

# Package issue - if we have a top level specs directory, we must make the
# associate package name available to anything that includes it, both within the
# project and outside it (when this project is accessed via OCPI_PROJECT_PATH)
ifneq ($(wildcard specs),)
  ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter projectpackage,$(MAKECMDGOALS)),)
    # package-id needs to be created early on by any non-clean make command.
    # This can be accomplished by having imports depend on it.
    imports: specs/package-id
    # If Project.mk changes, recreate specs/package-id file unless the package-id file contents
    # exactly match ProjectPackage.
    specs/package-id: Project.mk
	$(AT)if [ ! -e specs/package-id ] || [ "$$(cat specs/package-id)" != "$(ProjectPackage)" ]; then \
	       echo "$(ProjectPackage)" > specs/package-id; \
	     fi
  endif
endif

hdlassemblies applications: imports exports

# Perform test-related goals where they might be found.
DoTests=$(foreach t,\
          components hdl/devices hdl/adapters hdl/cards $(wildcard hdl/platforms/*/devices),\
          $(call MaybeMake,$t,$1) &&) true
$(OcpiTestGoals):
	$(call DoTests,$@)

# Make the imports link to the registry if it does not exist.
# If imports exists and is a link, leave it alone
# If imports is not a link, error
# If imports exists, but does not match the environment variable, warn
# If imports exists but is a broken link, replace it
imports:
	if [ ! -L imports ]; then \
	  if [ -e imports ]; then \
	    echo "Error: This project's imports is not a symbolic link and is therefore invalid." >&2 ; \
	    echo "Remove the imports file at the top level of the project before proceeding." >&2 ; \
	    exit 1 ; \
	  fi; \
	  if [ -d "$(OcpiProjectRegistryDir)" ]; then \
            $(call MakeSymLink2,$(OcpiProjectRegistryDir),$(realpath .),imports); \
	  else \
	    echo "Warning: The project registry '$(OcpiProjectRegistryDir)' does not exist" >&2 ; \
	  fi; \
	else \
	  if [ -n "$(OCPI_PROJECT_REGISTRY_DIR)" ]; then \
	    if [ "$(realpath $(OCPI_PROJECT_REGISTRY_DIR))" != "$(realpath imports)" ]; then \
	      echo "Warning: OCPI_PROJECT_REGISTRY_DIR is globally set to \"$(OCPI_PROJECT_REGISTRY_DIR)\"," >&2 ; \
	      echo "         but the '$(ProjectPackage)' project located at '$$(pwd)' is using" >&2 ; \
	      echo "         'imports -> $(realpath imports)'" >&2 ; \
	      echo "         The project's 'imports' link will take precedence when within the project." >&2 ; \
	    fi; \
	  fi; \
	  if [ ! -e imports ]; then \
	    if [ -d "$(OcpiProjectRegistryDir)" ]; then \
	      echo "Warning: 'imports' is a broken link and will be replaced with the default \"$(OcpiProjectRegistryDir)\"" >&2 ; \
	      rm imports; \
              $(call MakeSymLink2,$(OcpiProjectRegistryDir),$(realpath .),imports); \
	    else \
	      echo "Warning: Tried to update the broken 'imports' link, but the project registry '$(OcpiProjectRegistryDir)' does not exist" >&2 ; \
	    fi; \
	  elif [ ! -d "$(realpath imports)" ]; then \
	    echo "Warning: The project registry '$(realpath imports)' pointed to by 'imports' is not a directory" >&2 ; \
	  fi; \
	fi

exports:
	$(OCPI_CDK_DIR)/scripts/makeProjectExports.sh "$(OCPI_TARGET_DIR)" $(ProjectPackage)

components: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,components,rcc hdl)
	$(MAKE) exports

hdlprimitives: imports
	$(MAKE) imports
	$(call MaybeMake,hdl/primitives)
	$(MAKE) exports

hdlcomponents: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,components,hdl)
	$(MAKE) exports

hdldevices: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,hdl/devices)
	$(MAKE) exports

hdladapters: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,hdl/adapters)
	$(MAKE) exports

hdlcards: hdlprimitives
	$(MAKE) imports
	$(call MaybeMake,hdl/cards)
	$(MAKE) exports

hdlplatforms: hdldevices hdlcards hdladapters
	$(MAKE) imports
	$(call MaybeMake,hdl/platforms)
	$(MAKE) exports

hdlassemblies: hdlcomponents hdlplatforms hdlcards hdladapters
	$(MAKE) imports
	$(call MaybeMake,hdl/assemblies)
	$(MAKE) exports

# Everything that does not need platforms
hdlportable: hdlcomponents hdladapters hdldevices hdlcards

hdl: hdlassemblies

cleanhdl cleanrcc cleanocl cleancomponents cleanapplications: imports

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
# use $(realpath) rather than $(readlink -e) for portability (vs BSD/Darwin) and speed
cleanimports:
	if [ \( -L imports -a "$(realpath imports)" == "$(realpath $(OcpiProjectRegistryDir))" \) \
	     -o \( -L imports -a ! -e imports \) ]; then \
	  rm imports; \
	fi

cleanexports:
	rm -r -f exports

cleaneverything: clean
	find . -name '*~' -exec rm {} \;
	find . -depth -name '*.dSym' -exec rm -r {} \;
	find . -depth -name gen -exec rm -r -f {} \;
	find . -depth -name 'target-*' -exec rm -r -f {} \;
	find . -depth -name lib -exec rm -r -f {} \;

ifdef ShellProjectVars
projectpackage:
$(info ProjectPackage="$(ProjectPackage)";)
endif
