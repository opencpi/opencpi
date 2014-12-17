#
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
# This makefile is for building assemblies and bitstreams.
# The assemblies get built according to the standard build targets
HdlMode:=assembly
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

# Theses next lines are similar to what worker.mk does
ifneq ($(MAKECMDGOALS),clean)
$(if $(wildcard $(CwdName).xml),,\
  $(error The XML for the assembly, $(CwdName).xml, is missing))
endif
override Workers:=$(CwdName)
override Worker:=$(Workers)
Worker_$(Worker)_xml:=$(Worker).xml
Worker_xml:=$(Worker).xml
OcpiLanguage:=verilog
override LibDir=lib/hdl
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifndef HdlSkip
# Add to the component libraries specified in the assembly Makefile,
# and also include those passed down from the "assemblies" level
# Note we also add libraries that are needed to parse containers (FIXME).
override ComponentLibraries+= $(ComponentLibrariesInternal) components devices adapters cards
$(infox XMLII:$(XmlIncludeDirs)--$(ComponentLibraries))

# Add XML include dirs for parsing container XML, including "dot" for parsing
# this assembly's XML from container XML that is not in this dir
# and also include those passed down from the "assemblies" level - the internals
# FIXME: we should avoid full parsing of container XML here
# Override since they may be passed in from assemblies level
override XmlIncludeDirs+=. $(HdlPlatformsDir) $(HdlPlatformsDir)/specs $(XmlIncludeDirsInternal)
override HdlLibraries+=platform
ifdef Container
  ifndef Containers
    Containers:=$(Container)
  endif
endif
HdlContName=$(Worker)_$1_$2
HdlContOutDir=$(OutDir)container-$1
HdlContDir=$(call HdlContOutDir,$(call HdlContName,$1,$2))
ifneq ($(MAKECMDGOALS),clean)
  $(eval $(HdlPrepareAssembly))
  # default the container names
  $(foreach c,$(Containers),$(eval HdlContXml_$c:=$c))
  ## Extract the platforms and targets from the containers
  ## Then we can filter the platforms and targets based on that
  # Create the default container directories and files
  # $(call doDefaultContainer,<platform>,<config>)
  HdlDefContXml=<HdlContainer platform='$1/$1_$2' default='true'/>
  define doDefaultContainer
    $(call OcpiDbg,In doDefaultContainer for $1/$2 and HdlPlatforms: $(HdlPlatforms))
    ifneq (,$(if $(HdlPlatforms),$(filter $1,$(HdlPlatforms)),yes))
      # Create this default container's directory and xml file
      $(foreach c,$(call HdlContName,$1,$2),\
        $(foreach d,$(call HdlContDir,$1,$2)/gen,\
          $(foreach x,$d/$c.xml,\
            $$(shell \
                 mkdir -p $d; \
                 if test ! -f $x || test "`cat $x`" != "$(call HdlDefContXml,$1,$2)"; then \
                   echo "$(call HdlDefContXml,$1,$2)"> $x; \
                 fi))))
      Containers:=$(Containers) $(call HdlContName,$1,$2)
      HdlContXml_$(Worker)_$1_$2:=$(call HdlContDir,$1,$2)/gen/$(call HdlContName,$1,$2).xml
    endif
  endef  
  ifeq ($(origin DefaultContainers),undefined)
    $(call OcpiDbg,No Default Containers: HdlPlatforms: $(HdlPlatforms))
    # If undefined, we determine the default containers based on HdlPlatform
    $(foreach p,$(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlPlatforms))),$(eval $(call doDefaultContainer,$p,base)))
    $(call OcpiDbg,No Default Containers: Containers: $(Containers))
  else
    $(foreach d,$(DefaultContainers),\
       $(if $(findstring /,$d),\
         $(eval \
           $(call doDefaultContainer $(word 1,$(subst /, ,$d)),$(word 2,$(subst /, ,$d)))),\
         $(if $(filter $d,$(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlAllPlatforms)))),\
              $(eval $(call doDefaultContainer,$d,$d_base)),\
              $(error In DefaultContainers, $d is not a defined HDL platform.))))
  endif
  HdlContXml=$(or $($1_xml),$(if $(filter .xml,$(suffix $1)),$1,$1.xml))
  HdlStripXml=$(if $(filter .xml, $(suffix $1)),$(basename $1),$1)
  ifdef Containers
    # This procedure is (simply) to extract platform, target, and configuration info from the
    # xml for each container.. This allows us to build the list of targets necessary for all the
    # platforms mentioned by all the containers.  THis list then is the default targets when
    # targets are not specified.
    define doGetPlatform
      $(and $(call DoShell,$(OcpiGen) -S $(CwdName) -x platform $(HdlContXml_$1),HdlContPlatform),\
          $(error Processing container XML $1: $(HdlContPlatform)))
      $(and $(call DoShell,$(OcpiGen) -S $(CwdName) -x configuration $(HdlContXml_$1),HdlContConfig),\
          $(error Processing container XML $1: $(HdlContConfig)))
      $$(call OcpiDbgVar,HdlContPlatform)
      $$(call OcpiDbgVar,HdlContConfig)
      $(if $(HdlContPlatform),,$(error Could not get HdlPlatform for container $1))
      $(if $(HdlContConfig),,$(error Could not get HdlConfiguration for container $1))
      HdlMyPlatforms+=$(HdlContPlatform)
      HdlMyTargets+=$(call OcpiDbg,HdlPart_$(HdlContPlatform) is $(HdlPart_$(HdlContPlatform)))$(call HdlGetFamily,$(HdlPart_$(HdlContPlatform)))
      HdlPlatform_$1:=$(HdlContPlatform)
      HdlTarget_$1:=$(call HdlGetFamily,$(HdlPart_$(HdlContPlatform)))
      HdlConfig_$1:=$(HdlContConfig)
      $$(call OcpiDbgVar,HdlPlatform_$1)
      $$(call OcpiDbgVar,HdlMyPlatforms)
      $$(call OcpiDbgVar,HdlMyTargets)
    endef
    $(foreach c,$(Containers),$(eval $(call doGetPlatform,$(call HdlStripXml,$c))))
    #  $(info HdlMyPlatforms:$(HdlMyPlatforms) HdlMyTargets:=$(HdlMyTargets))
    $(call OcpiDbgVar,HdlPlatforms)
    $(call OcpiDbgVar,HdlTargets)
    $(call OcpiDbgVar,HdlMyPlatforms)
    $(call OcpiDbgVar,HdlMyTargets)
    ifdef HdlPlatforms
      override HdlPlatforms:=$(call Unique,$(filter $(HdlMyPlatforms),$(HdlPlatforms)))
    else
      override HdlPlatforms:=$(call Unique,$(HdlMyPlatforms))
    endif
    $(call OcpiDbgVar,ExcludePlatforms)
    $(call OcpiDbgVar,HdlPlatforms)
    $(call OcpiDbg,foo: $(filter-out $(ExcludePlatforms),$(HdlPlatforms)))
    override HdlPlatforms:=$(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlPlatforms)))
    $(call OcpiDbgVar,ExcludePlatforms)
    $(call OcpiDbgVar,HdlPlatforms)
    ifdef HdlTargets
      HdlTargets:=$(call Unique,$(filter $(HdlMyTargets),$(HdlTargets)))
    else
      HdlTargets:=$(call Unique,$(foreach p,$(HdlPlatforms),$(call HdlGetFamily,$(HdlPart_$p))))
    endif
    $(call OcpiDbgVar,ExcludePlatforms)
    $(call OcpiDbgVar,HdlPlatforms)
    $(call OcpiDbgVar,HdlTargets)
  endif # for ifdef Containers
else # for "clean" goal
  HdlTargets:=all
endif
# Due to our filtering, we might have no targets to build
$(call OcpiDbgVar,HdlPlatforms)
$(call OcpiDbgVar,HdlTargets)
$(call OcpiDbgVar,OnlyPlatforms)
ifeq ($(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlPlatforms))),)
  $(info No targets or platforms to build for this assembly)
else
  include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
  ifndef HdlSkip
    ifndef Containers
      ifneq ($(MAKECMDGOALS),clean)
        $(info No containers will be built since none match the specified platforms.)
      endif
    else
      # We have containers to build locally.
      # We already build the directories for default containers, and we already
      # did a first pass parse of the container XML for these containers
      HdlContResult=$(call HdlContOutDir,$1)/target-$(HdlTarget_$1)/$1.bitz
      define doContainer
         .PHONY: $(call HdlContOutDir,$1)
         all: $(call HdlContResult,$1)$(infox DEPEND:$(call HdlContResult,$1))
        $(call HdlContResult,$1): links
	  $(AT)mkdir -p $(call HdlContOutDir,$1)
	  $(AT)$(MAKE) -C $(call HdlContOutDir,$1) -f $(OCPI_CDK_DIR)/include/hdl/hdl-container.mk \
               HdlAssembly=../../$(CwdName) \
	       ComponentLibrariesInternal=\
               ComponentLibraries="$(call HdlAdjustLibraries,$(ComponentLibraries))" \
	       HdlLibrariesCommand=\
	       HdlLibraries="$(call HdlAdjustLibraries,$(HdlLibraries))" \
               XmlIncludeDirs="$(call AdjustRelative,$(XmlIncludeDirs))"
      endef
      $(foreach c,$(Containers),$(and $(filter $(HdlPlatform_$c),$(HdlPlatforms)),$(eval $(call doContainer,$c))))
    endif # containers
  endif # of skip
endif # check for no targets
endif
clean::
	$(AT) rm -r -f container-* lib
