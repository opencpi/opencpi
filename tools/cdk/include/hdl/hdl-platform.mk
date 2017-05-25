#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #
# This makefile is for building platforms and platform configurations
# A platform worker is a device worker that implements the platform_spec.xml
# A platform worker is built, and then some number of platform configurations
# are built.  The platform configurations are used when bitstreams
# get built elsewhere based on assemblies and configurations
$(call OcpiDbgVar,HdlPlatforms)
HdlMode:=platform
include $(OCPI_CDK_DIR)/include/util.mk
$(OcpiIncludeProject)
HdlLibraries+=platform
# Force the platform path to point to this directory.
# This means we can build this platform without it being
# defined globally anywhere, whether in OCPI_HDL_PLATFORM_PATH
# or in the core hdl/platforms dir.
# Note "export" must appear BEFORE override because once 
# "override" is used, "export" doesn't apply.
export OCPI_HDL_PLATFORM_PATH
override OCPI_HDL_PLATFORM_PATH := $(call OcpiAbsDir,.)$(and $(OCPI_HDL_PLATFORM_PATH),:$(OCPI_HDL_PLATFORM_PATH))
$(call OcpiDbgVar,OCPI_HDL_PLATFORM_PATH)
# If no platforms were specified, we obviously want to build this platform.
# And not default to some global "default" one.
ifeq ($(origin HdlPlatform),undefined)
  ifeq ($(origin HdlPlatforms),undefined)
    ifeq ($(MAKELEVEL),0)
      HdlPlatform:=$(CwdName)
      HdlPlatforms:=$(CwdName)
    endif
  endif
endif
# We defer this because of side effects, which is wrong: FIXME hdl-make should not do hdl-target.mk?
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
$(call OcpiDbgVar,HdlPlatforms)
# Theses next lines are similar to what worker.mk does
ifneq ($(MAKECMDGOALS),clean)
$(if $(call OcpiExists,$(CwdName).xml),,\
  $(error The OWD for the platform and its worker, $(CwdName).xml, is missing))
endif
$(call OcpiDbgVar,HdlPlatforms)
override Workers:=$(CwdName)
override Worker:=$(Workers)
Worker_$(Worker)_xml:=$(Worker).xml
OcpiLanguage:=vhdl
# We need these libraries to parse the platform xml file.
# 1. a local "devices" library if there is one.
# 2. any other libraries defined for the platform
# 3. the generic "devices" and "cards" in the project path
ComponentLibraries+=\
 $(wildcard ./devices)\
 $(infox PCL:$(ComponentLibraries_$(Worker)):$(shell pwd))$(ComponentLibraries_$(Worker)) \
 devices cards components
LibDir=lib/hdl
$(call OcpiDbgVar,HdlPlatforms)
ifeq ($(origin HdlPlatforms),undefined)
 override HdlPlatforms:=$(HdlPlatform)
endif
# Before we really do any "worker" or "config" stuff we must recurse into the device directory
ifneq ($(MAKECMDGOALS),clean)
  ifneq ($(wildcard devices),)
    # We need to build the devices subdir before processing this file any further since the
    # parsing of the platform worker's XML depends on having these devices built.
    # Since we are NOT executing this in a recipe, exports are not happening automatically by make.
    $(and $(shell \
       RET=; \
       echo ======= Entering the \"devices\" library for the \"$(Worker)\" platform. 1>&2; \
       export OCPI_HDL_PLATFORM_PATH=$(CURDIR):$$OCPI_HDL_PLATFORM_PATH; \
       $(MAKE) -C devices --no-print-directory \
         ComponentLibrariesInternal="$(call OcpiAdjustLibraries,$(ComponentLibraries))" \
         XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirsInternal))" \
         HdlPlatforms="$(HdlPlatforms)" HdlPlatform="$(HdlPlatform)" \
         HdlTargets="$(HdlTargets)" HdlTarget="$(HdlTarget)" $(MAKECMDGOALS) 1>&2 && \
         mkdir -p lib 1>&2 && rm -f lib/devices && ln -s ../devices/lib lib/devices 1>&2 || \
	 RET=1; \
       echo ======= Exiting the \"devices\" library for the \"$(Worker)\" platform. 1>&2; \
       echo $$RET),$(error Error building devices library in $(CURDIR)))
  endif
endif
$(call OcpiDbgVar,HdlExactPart)
$(call OcpiDbgVar,HdlPlatform)
HdlExactPart:=$(HdlPart_$(HdlPlatform))
$(call OcpiDbgVar,HdlExactPart)
$(call OcpiDbgVar,HdlTargets)
$(eval $(HdlSearchComponentLibraries))
$(call OcpiDbgVar,XmlIncludeDirsInternal)
$(infox HP1:$(HdlPlatforms))
$(call OcpiDbgVar,HdlPlatforms)
SubCores_$(call HdlGetFamily,$(Worker)):=$(Cores)
OnlyPlatforms:=$(Worker)
OnlyTargets:=$(call HdlGetFamily,$(Worker))
# We might be building for multiple platforms (e.g. sim and this one)
# But from here, if we are building this platform, we must force it
# When the mode is platform and config, the underlying scripts depend
# on HdlPlatform being the currrent platform.
ifeq ($(filter $(Worker),$(HdlPlatforms))$(filter clean,$(MAKECMDGOALS)),)
  HdlSkip := 1
  $(info Skipping this platform ($(Worker)).  It is not in HdlPlatforms ($(HdlPlatforms)))
else
  override HdlPlatform:=$(Worker)
  include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
endif
# the target preprocessing may tell us there is nothing to do
# some platforms may have been used for the devices subdir (tests, sims, .etc.)
ifndef HdlSkip
  $(call OcpiDbgVar,HdlPlatforms)
  ################################################################################
  # We are like a worker (and thus a core)
  # But we're VHDL only, so we only need to build the rv core
  # which is the "app" without container or the platform
  # FIXME: we can't do this yet because the BB library name depends on there being both cores...
  #Tops:=$(Worker)_rv
  $(eval $(OcpiProcessBuildFiles))
  $(eval $(HdlSearchComponentLibraries))
  include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
  ifdef HdlSkip
    $(error unexpected target/platform skip)
  endif
  exports:
  .PHONY: exports
  ifneq ($(MAKECMDGOALS),clean)
    $(if $(wildcard base.xml)$(wildcard $(GeneratedDir)/base.xml),,\
      $(shell echo '<HdlConfig/>' > $(GeneratedDir)/base.xml))
  endif
  ################################################################################
  # From here its about building the platform configuration cores
  ifndef Configurations
    ifneq ($(MAKECMDGOALS),clean)
      ifeq ($(origin Configurations),undefined)
        Configurations:=base
      else
        $(info No platform configurations will be built since none are specified.)
      endif
    endif
  endif
  ifdef Configurations
    HdlConfName=$(Worker)_$1
    HdlConfOutDir=$(OutDir)config-$1
    HdlConfDir=$(call HdlConfOutDir,$(call HdlConfName,$1))
    # We have containers to build locally.
    # We already build the directories for default containers, and we already
    # did a first pass parse of the container XML for these containers
    .PHONY: configs
    define doConfig
      configs: $(call HdlConfOutDir,$1)
#       Cores="$(call OcpiAdjustLibraries,$(Cores))" \

      $(call HdlConfOutDir,$1): exports
	$(AT)mkdir -p $$@
	$(AT)echo ======= Entering the \"$1\" configuration for the \"$(Worker)\" platform.
	$(AT)$(MAKE) -C $$@ -f $(OCPI_CDK_DIR)/include/hdl/hdl-config.mk --no-print-directory \
               HdlPlatforms=$(Worker) \
               HdlPlatformWorker=../../$(Worker) \
               HdlLibrariesInternal="$(call OcpiAdjustLibraries,$(HdlLibraries) $(Libraries))" \
               ComponentLibrariesInternal="$(call OcpiAdjustLibraries,$(ComponentLibraries))" \
               XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirsInternal))" \
	       $(MAKECMDGOALS)
	$(AT)echo ======= Exiting the \"$1\" configuration for the \"$(Worker)\" platform.
    endef
    $(foreach c,$(Configurations),$(eval $(call doConfig,$c)))
    all: configs
  endif # have configurations
  # If the platform has special files to export to the CDK, do it
  # this is only done if we are building, so we don't export until we have actually built something
  ExportFiles:=$(call Unique,$(ExportFiles) $(wildcard $(Worker).mk))
  ifdef ExportFiles
    ExportLinks:=$(ExportFiles:%=lib/%)
    exports: $(ExportLinks)

    # order-only prereq should be something like $$(@:lib/%=%) but that doesn't work...
    $(ExportLinks): | $(ExportFiles)
	$(AT)mkdir -p lib
	$(AT)ln -s ../$(@:lib/%=%) lib

    all: $(ExportLinks)
  endif
endif # skip after hel-pre.mk
# There is no test target here, but there might be in the devices subdir
test:

clean::
	$(AT)if test -d devices; then make -C devices clean; fi
	$(AT) rm -r -f config-* lib


