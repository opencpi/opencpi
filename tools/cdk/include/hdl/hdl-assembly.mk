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

# This makefile is for building assemblies and bitstreams.
# The assemblies get built according to the standard build targets
HdlMode:=assembly
Model:=hdl
include $(OCPI_CDK_DIR)/include/util.mk
$(OcpiIncludeProject)
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk

# These next lines are similar to what worker.mk does
ifneq ($(MAKECMDGOALS),clean)
hdl: all # for convenience
$(if $(wildcard $(CwdName).xml),,\
  $(error The XML for the assembly, $(CwdName).xml, is missing))
endif
override Workers:=$(CwdName)
override Worker:=$(Workers)
Worker_$(Worker)_xml:=$(Worker).xml
Worker_xml:=$(Worker).xml
# Not good, but we default the language differently here for compatibility.
OcpiLanguage:=$(call OcpiGetLanguage,$(Worker_xml))
ifndef OcpiLanguage
  OcpiLanguage:=verilog
endif
override LibDir=lib/hdl
override HdlLibraries+=platform
# If HdlPlatforms is explicitly defined to nothing, then don't build containers.
ifeq ($(HdlPlatform)$(HdlPlatforms),)
  ifneq ($(origin HdlPlatforms),undefined)
    override Containers=
    override DefaultContainers=
  endif
endif
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifndef HdlSkip
# Add to the component libraries specified in the assembly Makefile,
# and also include those passed down from the "assemblies" level
# THIS LIST IS FOR THE APPLICATION ASSEMBLY NOT FOR THE CONTAINER
override ComponentLibraries+= $(ComponentLibrariesInternal) components adapters
# Override since they may be passed in from assemblies level
override XmlIncludeDirsInternal:=$(XmlIncludeDirs) $(XmlIncludeDirsInternal)
ifdef Container
  ifndef Containers
    Containers:=$(Container)
  endif
endif
HdlContName=$(Worker)_$1_$2
HdlContOutDir=$(OutDir)container-$1
HdlContDir=$(call HdlContOutDir,$(call HdlContName,$1,$2))
HdlStripXml=$(if $(filter .xml,$(suffix $1)),$(basename $1),$1)
ifneq ($(MAKECMDGOALS),clean)
#  $(info 0.Only:$(OnlyPlatforms),All:$(HdlAllPlatforms),Ex:$(ExcludePlatforms),HP:$(HdlPlatforms))
  ifndef ShellHdlAssemblyVars
    $(eval $(HdlPrepareAssembly))
  endif
  # default the container names
  $(foreach c,$(Containers),$(eval HdlContXml_$c:=$c))
  # $(call doConfigConstraints,<container>,<platform>,<config>)
  getConfigConstraints=$(strip\
     $(if $(call DoShell,$(call OcpiGenTool) -Y $(HdlPlatformDir_$2)/hdl/$3,HdlConfConstraints),\
          $(error Processing platform configuration XML "$3" for platform "$2" for container "$1"),\
	  $(call HdlGetConstraintsFile,$(HdlConfConstraints),$2)))
  ## Extract the platforms and targets from the containers that have their own xml
  ## Then we can filter the platforms and targets based on that
  ifdef Containers
    # Add a defined non-default container to the build
    # $(call addContainer,<container>,<platform>,<config>,<constraints>)
    define addContainer
      $$(if $$(HdlPart_$2),,\
        $$(error Platform for container $1, $2, is not defined))
      $$(call OcpiDbg,HdlPart_$2:$$(HdlPart_$2))
      HdlMyTargets+=$$(call HdlGetFamily,$$(HdlPart_$2))
      ContName:=$(Worker)_$2_$3_$1
      HdlContainerImpls_$1+= $$(ContName)
      HdlPlatform_$$(ContName):=$2
      HdlTarget_$$(ContName):=$$(call HdlGetFamily,$$(HdlPart_$2))
      HdlConfig_$$(ContName):=$3
      ifeq ($4,-)
        HdlConstraints_$$(ContName):=$$(strip\
          $$(foreach c,$$(call getConfigConstraints,$1,$2,$3),\
             $$(HdlPlatformDir_$2)/$$c))
      else
        HdlConstraints_$$(ContName):=$$(strip \
          $$(foreach c,$4,\
             $$(call AdjustRelative,$$c,$$(HdlPlatformDir_$2))))
      endif
      HdlContXml_$$(ContName):=$$(call HdlContOutDir,$$(ContName))/gen/$$(ContName).xml
      $$(shell mkdir -p $$(call HdlContOutDir,$$(ContName))/gen; \
               if ! test -e  $$(HdlContXml_$$(ContName)); then \
                 ln -s ../../$1.xml $$(HdlContXml_$$(ContName)); \
               fi)
      HdlContainers:=$$(HdlContainers) $$(ContName)
    endef
    # This procedure is (simply) to extract platform, target, and configuration info from the
    # xml for each explicit container. This allows us to build the list of targets necessary for
    # all the platforms mentioned by all the containers.  This list then is the default targets
    # when targets are not specified.
    # ocpigen -X returns the config as word 1 followed by "only platforms" as subsequent words
    define doGetPlatform
      $$(and $$(call DoShell,$(call OcpiGen, -X $1),HdlContPfConfig),\
          $$(error Processing container XML $1: $$(HdlContPfConfig)))
      HdlContPlatforms:=$$(wordlist 3,$$(words $$(HdlContPfConfig)),$$(HdlContPfConfig))
      $$(foreach p,$$(filter $$(or $$(HdlContPlatforms),$$(OnlyPlatforms),$$(HdlAllPlatforms)),\
                      $$(filter-out $$(ExcludePlatforms),$$(HdlPlatforms))),\
	 $$(eval $$(call addContainer,$1,$$p,$$(word 1,$$(HdlContPfConfig)),$$(word 2,$$(HdlContPfConfig)))))
    endef
    $(foreach c,$(Containers),$(eval $(call doGetPlatform,$(call HdlStripXml,$c))))
  endif
  # Create the default container directories and files
  # $(call doDefaultContainer,<platform>,<config>)
  HdlDefContXml=<HdlContainer platform='$1/$2' default='true'/>
#  $(info 1.Only:$(OnlyPlatforms),All:$(HdlAllPlatforms),Ex:$(ExcludePlatforms),HP:$(HdlPlatforms))
  define doDefaultContainer
    $(call OcpiDbg,In doDefaultContainer for $1/$2 and HdlPlatforms: $(HdlPlatforms) [filtered=$(filter $1,$(HdlPlatforms))])
    ifneq (,$(if $(HdlPlatforms),$(filter $1,$(HdlPlatforms)),yes))
      # Create this default container's directory and xml file
      ContName:=$(call HdlContName,$1,$2)
      $(foreach d,$(call HdlContDir,$1,$2)/gen,\
        $$(foreach x,$d/$$(ContName).xml,\
          $$(shell \
               mkdir -p $d; \
               if test ! -f $$x || test "`cat $$x`" != "$(call HdlDefContXml,$1,$2)"; then \
                 echo "$(call HdlDefContXml,$1,$2)"> $$x; \
               fi)))
      HdlPlatform_$$(ContName):=$1
      HdlTarget_$$(ContName):=$(call HdlGetFamily,$(HdlPart_$1))
      HdlConfig_$$(ContName):=$2
      HdlContXml_$$(ContName):=$$(call HdlContOutDir,$$(ContName))/gen/$$(ContName).xml
      HdlContainers:=$$(HdlContainers) $$(ContName)
      HdlBaseContainerImpls:=$$(HdlBaseContainerImpls) $$(ContName)
    endif
  endef
  ifeq ($(origin DefaultContainers),undefined)
    $(call OcpiDbg,No Default Containers: HdlPlatforms: $(HdlPlatforms))
    # If undefined, we determine the default containers based on HdlPlatforms
    $(foreach p,$(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),\
                 $(filter-out $(ExcludePlatforms),$(HdlPlatforms))),\
                $(eval $(call doDefaultContainer,$p,base)))
    $(call OcpiDbg,No Default Containers: Containers: $(Containers) (Container=$(Container)))
  else
    $(foreach d,$(DefaultContainers),\
       $(if $(findstring /,$d),\
         $(eval \
           $(call doDefaultContainer $(word 1,$(subst /, ,$d)),$(word 2,$(subst /, ,$d)))),\
         $(if $(filter $d,$(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlAllPlatforms)))),\
              $(eval $(call doDefaultContainer,$d,base)),\
              $(error In DefaultContainers, $d is not a defined HDL platform.))))
  endif
  HdlContXml=$(or $($1_xml),$(if $(filter .xml,$(suffix $1)),$1,$1.xml))
  ifdef HdlContainers
    HdlMyPlatforms:=$(foreach c,$(HdlContainers),$(HdlPlatform_$c))
    ifdef HdlPlatforms
      override HdlPlatforms:=$(call Unique,$(filter $(HdlMyPlatforms),$(HdlPlatforms)))
    else
      override HdlPlatforms:=$(call Unique,$(HdlMyPlatforms))
    endif
    override HdlPlatforms:=$(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlPlatforms)))
    ifdef HdlTargets
      HdlTargets:=$(call Unique,$(filter $(HdlMyTargets),$(HdlTargets)))
    else
      HdlTargets:=$(call Unique,$(foreach p,$(HdlPlatforms),$(call HdlGetFamily,$(HdlPart_$p))))
    endif
  endif # for ifdef Containers
else # for "clean" goal
  HdlTargets:=all
endif
# Due to our filtering, we might have no targets to build
ifeq ($(filter $(or $(OnlyPlatforms),$(HdlAllPlatforms)),$(filter-out $(ExcludePlatforms),$(HdlPlatforms)))$(filter skeleton,$(MAKECMDGOALS)),)
  $(and $(HdlPlatforms),$(info Not building assembly $(Worker) for platform(s): $(HdlPlatforms) in "$(shell pwd)"))
#  $(info No targets or platforms to build for this "$(Worker)" assembly in "$(shell pwd)")
else ifndef HdlContainers
  ifneq ($(MAKECMDGOALS),clean)
    $(info No containers will be built since none match the specified platforms.)
  endif
else
  ifndef ShellHdlAssemblyVars
    $(eval $(OcpiProcessBuildFiles))
    include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
  endif
  ifndef HdlSkip
    ifndef HdlContainers
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
               HdlAssembly=../../$(CwdName)  HdlConfig=$(HdlConfig_$1) \
               HdlConstraints=$(HdlConstraints_$1) \
               HdlPlatforms=$(HdlPlatform_$1) HdlPlatform=$(HdlPlatform_$1) \
	       ComponentLibrariesInternal="$(call OcpiAdjustLibraries,$(ComponentLibraries))" \
	       HdlLibrariesInternal="$(call OcpiAdjustLibraries,$(HdlMyLibraries))" \
               XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirsInternal))"
      endef
      $(foreach c,$(HdlContainers),$(and $(filter $(HdlPlatform_$c),$(HdlPlatforms)),$(eval $(call doContainer,$c))))
    endif # containers
  endif # of skip
endif # check for no targets
endif
clean::
	$(AT) rm -r -f container-* lib

# Expose information about this assembly's containers for use
# in some external application.
#
# Impl is an Implementation of a Container (i.e. the actual build directory)
# An Impl is bound to a Platform and even a Platform Configuration
# TODO: Here we should also provide HdlConfig_* so the actual Platform Configuration
#       mapping is available for each container implementation
ifdef ShellHdlAssemblyVars
shellhdlassemblyvars:
$(info Containers="$(Containers)"; \
       DefaultContainers="$(DefaultContainers)"; \
       HdlContainers="$(HdlContainers)"; \
       HdlBaseContainerImpls="$(HdlBaseContainerImpls)"; \
       $(foreach c,$(Containers),\
	 HdlContainerImpls_$c="$(HdlContainerImpls_$c)"; )\
       $(foreach c,$(HdlContainers),\
         HdlPlatform_$c="$(HdlPlatform_$c)"; )\
        )
endif
