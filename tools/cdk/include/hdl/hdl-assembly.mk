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

override ComponentLibraries+= components devices adapters cards
$(eval $(HdlSearchComponentLibraries))
override XmlIncludeDirs+=. $(HdlPlatformsDir) $(HdlPlatformsDir)/specs
override HdlLibraries+=platform
## Extract the platforms and targets from the containers
## Then we can filter the platforms and targets based on that
ifneq ($(MAKECMDGOALS),clean)
ifdef Container
ifndef Containers
Containers:=$(Container)
endif
endif
ifndef HdlPlatforms
  ifdef HdlPlatform
    HdlPlatforms:=$(HdlPlatform)
  else ifeq ($(Containers)$(DefaultContainers),)
    HdlPlatforms:=ml605
  endif
else
  ifndef HdlTargets
    HdlTargets:=$(foreach p,$(filter-out $(ExcludePlatforms),$(HdlPlatforms)),$(call HdlGetFamily,$(HdlPart_$p)))
  endif
endif

# Create the default container files in the gen diretories
# $(call doDefaultContainer,<platform>,<config>)
HdlDefContXml=<HdlContainer platform='$1/$2' default='true'/>
define doDefaultContainer
  $(call OcpiDbg,In doDefaultContainer for $1/$2 and HdlPlatforms: $(HdlPlatforms))
  ifneq (,$(if $(HdlPlatforms),$(filter $1,$(HdlPlatforms)),yes))
    $(CwdName)_$2_xml:=$(GeneratedDir)/$(CwdName)_$2.xml
    $$(shell \
	  mkdir -p $(GeneratedDir); \
          if test ! -f $$($(CwdName)_$2_xml) || test "`cat $$($(CwdName)_$2_xml)`" != "$(call HdlDefContXml,$1,$2)"; then \
            echo "$(call HdlDefContXml,$1,$2)"> $$($(CwdName)_$2_xml); \
          fi)
    Containers:=$(Containers) $(CwdName)_$2
  endif
endef  
ifeq ($(origin DefaultContainers),undefined)
  $(call OcpiDbg,No Default Containers: HdlPlatforms: $(HdlPlatforms))
  # If undefined, we determine the default containers based on HdlPlatform
  $(foreach p,$(filter-out $(ExcludePlatforms),$(HdlPlatforms)),$(eval $(call doDefaultContainer,$p,$p_base)))
else
  $(foreach d,$(DefaultContainers),\
     $(if $(findstring /,$d),\
	 $(eval $(call doDefaultContainer $(word 1,$(subst /,$d)),$(word 2,$(subst /,$d)))),\
         $(if $(filter $d,$(filter-out $(ExcludePlatforms),$(HdlAllPlatforms))),\
              $(eval $(call doDefaultContainer,$d,$d_base)),\
              $(error In DefaultContainers, $d is not a defined HDL platform.))))
endif
HdlContXml=$(or $($1_xml),$(if $(filter .xml,$(suffix $1)),$1,$1.xml))
HdlStripXml=$(if $(filter .xml, $(suffix $1)),$(basename $1),$1)
ifdef Containers
  define doGetPlatform
    $(and $(call DoShell,$(OcpiGen) -S $(CwdName) -x platform $(call HdlContXml,$1),HdlContPlatform),\
          $(error Processing container XML $1: $(HdlContPlatform)))
    $(and $(call DoShell,$(OcpiGen) -S $(CwdName) -x configuration $(call HdlContXml,$1),HdlContConfig),\
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
  override HdlPlatforms:=$(filter-out $(ExcludePlatforms),$(HdlPlatforms))
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

endif
else # for "clean" goal
HdlTargets:=all
endif
# Due to our filtering, we might have no targets to build
#$(info T:$(HdlTargets)|P:$(filter-out $(ExcludePlatforms),$(HdlPlatforms)))

ifeq ($(HdlTargets)$(filter-out $(ExcludePlatforms),$(HdlPlatforms)),)
  $(info No targets or platforms to build for this assembly)
else
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifndef HdlSkip

# Generate the assemnbly's bb file by simply linking to the defs
#CoreBlackBoxFile:=$(GeneratedDir)/$(Worker)_bb$(HdlSourceSuffix)
#$(CoreBlackBoxFile): $$(DefsFile) | $$(OutDir)gen
#	$(AT)$(call MakeSymLink2,$(DefsFile),$(GeneratedDir),$(notdir $@))

# Generate the source code for this "assembly worker" implementation file.
$(call OcpiDgbVar,$(HdlSourceSuffix))
# This variable is also used when processing the ImplWorkersFile to determine the language
ImplFile:=$(GeneratedDir)/$(Worker)-assy$(HdlSourceSuffix)
$(call OcpiDbgVar,ImplFile)
AssyWorkersFile:=$(GeneratedDir)/$(CwdName).wks
# This is overridden for the container
ImplWorkersFile=$(AssyWorkersFile)

$(ImplFile): $$(ImplXmlFile) | $$(GeneratedDir)
	$(AT)echo Generating the assembly source file: $@ from $<
	$(AT)$(OcpiGen) -D $(GeneratedDir) -W $(Worker) -a  $<

# The workers file is actually created at the same time as the -assy.v file
$(AssyWorkersFile): $(ImplFile)

# A dependency on the core
HdlPreCore=$(AssyWorkersFile)

# The source code for this "worker" is the generated assembly file.
GeneratedSourceFiles:=$(ImplFile)

# When parsing the HdlAssembly file, we need access to the xml from the 
# workers in the assembly, both at the implementation level and the spec level
override XmlIncludeDirs += $(call HdlXmlComponentLibraries,$(ComponentLibraries))

include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk
ifndef HdlSkip

################################################################################
# From here down its about building the final bitstreams
ifndef Containers
$(info No bitstreams will be built since no containers are specified.)
else
#can we get this without parsing multiple times? someday binary form...
HdlContDir=$(OutDir)target-$1
HdlConfig=$(HdlPlatformsDir)/$(HdlPlatform_$1)/target-$(call HdlConfig_$1)/$(call HdlConfig_$1)_rv
HdlAssembly=target-$(HdlTarget_$1)/$(Worker)_rv
HdlContainer=$(call HdlContDir,$1)/$1$(call HdlGetBinSuffix,$(HdlPlatform_$1))
HdlContSource=$(GeneratedDir)/$1-$2.vhd
HdlContSources=$(foreach s,defs impl assy,$(call HdlContSource,$1,$s))
HdlBitName=$(call BitFile_$(HdlToolSet_$(HdlTarget_$1)),$(Worker)-$1)
#HdlBitZName=$(HdlContDir)/$(call HdlBitName,$1).gz
ArtifactXmlName=$(call HdlContDir,$1)/$1-art.xml
UUIDFileName=$(call HdlContDir,$1)/$1_UUID.v
MetadataRom=$(call HdlContDir,$1)/metadatarom.dat
# This happens in the target dir immediately before compilation
HdlContPreCompile=\
  echo Generating UUID, artifact xml file and metadata ROM file for container $1. && \
  (cd .. && \
   $(OcpiGen) -D target-$1 -A -S $(Worker) -P $(HdlPlatform_$1) -e $(call HdlGetPart,$(HdlPlatform_$1)) $(call HdlContXml,$1) && \
   $(OcpiHdl) bram $(call ArtifactXmlName,$1) $(call MetadataRom,$1) \
   )
HdlContBitName=$(call HdlContDir,$1)/$(call HdlBitName,$1)
HdlContBitZName=$(call HdlContBitName,$1).gz
ContainerWorkersFile=$(call HdlContDir,$1)/$1.wks
ContainerXmlFile=$(call HdlContXml,$1)
HdlContOcpiGen=\
  $(OcpiGen) -D $(GeneratedDir) \
             $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	     $(and $(HdlPlatform),-P $(HdlPlatform)) \
             -S $(Worker) \
	     $(if $(and $(ComponentLibraries),$(HdlToolNeedBB_$(HdlToolSet_$(HdlTarget_$1)))),\
                $(foreach l,$(ComponentLibraries),\
                  -L $(notdir $l):$(call HdlXmlComponentLibrary,$l)/hdl)) \

################################################################################
# The function to evaluate for each container
$(call OcpiDbg,before doContainer)
define doContainer
$(call OcpiDbg,doContainer($1) platform $(HdlPlatform_$1) config $(HdlConfig_$1))
##### Generate source files for the container

# FIXME: we could share this with workers better

$(call HdlContSource,$1,defs): $(call HdlContXml,$1) | $(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $$@ from $$<
	$(AT)$(HdlContOcpiGen) -d $$<

$(call HdlContSource,$1,impl): $(call HdlContXml,$1) | $(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $$@ from $$<
	$(AT)$(HdlContOcpiGen) -i $$<

$(call HdlContSource,$1,assy): $(call HdlContXml,$1) | $(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $$@ from $$<
	$(AT)$(HdlContOcpiGen) -W $1 -a $$<

###### Define the bitstream.
###### Some tools may in fact make a container core, but some will not when
###### it is a top-level/bit-stream build

$(call HdlContDir,$1):
		$(AT)mkdir $$@

# This is for building the top level container core
$(call HdlContainer,$1): | $(call HdlContDir,$1)
$(call HdlContainer,$1): HdlPreCompile=$(call HdlContPreCompile,$1)
# This is not a dependency since this file is generated each time it is built
$(call HdlContainer,$1): TargetSourceFiles=$(call UUIDFileName,$1)
$(call HdlContainer,$1): GeneratedSourceFiles=$(call HdlContSources,$1)
$(call HdlContainer,$1): $(call HdlContSources,$1)
$(call HdlContainer,$1): Core:=$1
$(call HdlContainer,$1): Top:=$1
$(call HdlContainer,$1) $(call HdlContBitName,$1): HdlMode:=container
$(call HdlContainer,$1): HdlLibraries+=platform
$(call HdlContainer,$1): LibName:=$1
$(call HdlContainer,$1): XmlIncludeDirs+=$(HdlPlatformsDir)/$(HdlPlatform_$1)
$(call HdlContainer,$1): HdlSources:=$$(CompiledSourceFiles)
$(call HdlContainer,$1) $(call HdlContBitName,$1): \
                         override HdlTarget:=$(foreach p,$(call HdlGetPart,$(HdlPlatform_$1)),$p)
$(call HdlContainer,$1) $(call HdlContBitName,$1): \
                         TargetDir=$(call HdlContDir,$1)
$(call HdlContainer,$1) $(call HdlContBitName,$1): \
                         HdlPlatform=$(HdlPlatform_$1)
$(call HdlContainer,$1) $(call HdlContBitName,$1): \
                         override ImplWorkersFile=$$(call HdlExists,$(GeneratedDir)/$1.wks)
$(call HdlContainer,$1) $(call HdlContBitName,$1): \
                         override ImplFile=$$(call HdlContSource,$1,assy)
#$(call HdlContainer,$1) $(call HdlContBitName,$1): \
#                         AllCores=$$(call HdlCollectCores,$$(HdlTarget))
# The two basic pieces of the container are cores, not workers
$(call HdlContainer,$1) $(call HdlContBitName,$1): Cores=$(call HdlAssembly,$1) $(call HdlConfig,$1)
$(call HdlContainer,$1): \
      $(call HdlToolCoreRef_$(HdlToolSet_$(HdlTarget_$1)),$(call HdlAssembly,$1))$(HdlBin) \
      $(call HdlToolCoreRef_$(HdlToolSet_$(HdlTarget_$1)),$(call HdlConfig,$1))$(HdlBin)

assembly: target-$(HdlTarget_$1)/$(Worker)_rv$(HdlBin)

$(call HdlContainer,$1):
	$(AT)echo Building $$(HdlMode) core \"$1\" for assembly "$(Worker)" for target \"$(HdlPlatform_$1)\"
	$(AT)$$(HdlCompile)

$(call HdlContBitName,$1): $(call HdlContainer,$1)

# Finally we create our artifact, compressing and attaching the metadata

container: $(call HdlContBitName,$1)

$(call HdlContBitZName,$1): $(call HdlContBitName,$1)
	$(AT)echo Making compressed bit file: $$@ from $(call HdlContBitName,$1) and $(call ArtifactXmlName,$1)
	$(AT)gzip -c $(call HdlContBitName,$1) > $$@
	$(AT)$(ToolsDir)/../../scripts/addmeta $(call ArtifactXmlName,$1) $$@

all: $(call HdlContBitZName,$1)
# Maybe the platform and not the tool defines how things build...
-include $(HdlPlatformsDir)/$(HdlPlatform_$1)/$(HdlPlatform_$1).mk

# Now invoke the tool-specific build with: <target-dir>,<assy-name>,<core-file-name>,<config>,<platform>
$(call HdlToolDoPlatform_$(HdlToolSet_$(HdlTarget_$1)),$(call HdlContDir,$1),$(Worker),$(Worker)-$1,$1,$(HdlPlatform_$1),$(HdlTarget_$1))

HdlContainerCores+=$(call HdlContainer,$1)
HdlContainerBitZNames+=$(call HdlContBitZName,$1)
$$(call OcpiDbg,doContainer end)
endef

$(foreach c,$(Containers),\
  $(foreach x,$(call HdlStripXml,$c),\
    $(and $(filter $(HdlPlatform_$x),$(HdlPlatforms)),$(eval $(call doContainer,$x)))))

$(call OcpiDbg,afterdoContainers)

all: $(HdlContainerBitZNames)

# Force all workers to be built first for all targets
#containers: cores $(HdlContainers)
#all: containers

endif # of skip
endif # of containers
endif # of skip
endif
