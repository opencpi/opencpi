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

override ComponentLibraries+= components devices
$(eval $(HdlSearchComponentLibraries))
override XmlIncludeDirs+=. $(HdlPlatformsDir) $(HdlPlatformsDir)/specs
override HdlLibraries+=platform
## Extract the platforms and targets from the containers
## Then we can filter the platforms and targets based on that
ifneq ($(MAKECMDGOALS),clean)
ifdef Containers
  define doGetPlatform
    $(and $(call DoShell,$(OcpiGen) -S $(CwdName) -x platform $1,HdlContPlatform),\
          $(error Processing container XML $1: $(HdlContPlatform)))
    $(and $(call DoShell,$(OcpiGen) -S $(CwdName) -x configuration $1,HdlContConfig),\
          $(error Processing container XML $1: $(HdlContConfig)))
    $(call OcpiDbgVar,HdlContPlatform)
    $(call OcpiDbgVar,HdlContConfig)
    $(if $(HdlContPlatform),,$(error Could not get HdlPlatform for container $1))
    $(if $(HdlContConfig),,$(error Could not get HdlConfiguration for container $1))
    HdlMyPlatforms+=$(HdlContPlatform)
    HdlMyTargets+=$(call HdlGetFamily,$(HdlPart_$(HdlContPlatform)))
    HdlPlatform_$1:=$(HdlContPlatform)
    HdlTarget_$1:=$(call HdlGetFamily,$(HdlPart_$(HdlContPlatform)))
    HdlConfig_$1:=$(HdlContConfig)
    $(call OcpiDbgVar,HdlPlatform_$1)
  endef
  $(foreach c,$(Containers),$(eval $(call doGetPlatform,$c)))
#  $(info HdlMyPlatforms:$(HdlMyPlatforms) HdlMyTargets:=$(HdlMyTargets))
  ifdef HdlPlatforms
    HdlPlatforms:=$(call Unique,$(filter $(HdlMyPlatforms),$(HdlPlatforms)))
  else
    HdlPlatforms:=$(HdlMyPlatforms)
  endif
  ifndef HdlTargets
    HdlTargets:=$(HdlMyTargets)    
  endif
endif
else
HdlTargets:=all
endif
# Due to our filtering, we might have no targets to build
ifeq ($(HdlTargets)$(HdlPlatforms),)
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
ImplFile:=$(GeneratedDir)/$(Worker)-assy$(HdlSourceSuffix)
$(call OcpiDbgVar,ImplFile)
AssyWorkersFile:=$(GeneratedDir)/$(CwdName).wks
# This is overridden for the container and for the bitstream
ImplWorkersFiles=$(AssyWorkersFile)

$(ImplFile): $$(ImplXmlFile) | $$(GeneratedDir)
	$(AT)echo Generating the assembly source file: $@ from $<
	$(AT)$(OcpiGen) -D $(GeneratedDir) -W $(Worker) -a  $<

# The workers file is actually created at the same time as the -assy.v file
$(AssyWorkersFile): $(ImplFile)

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
HdlContainer=$(call HdlContDir,$1)/$1$(call HdlGetBinSuffix,$(HdlPlatform_$1))
HdlContSource=$(GeneratedDir)/$1-$2.vhd
HdlContSources=$(foreach s,defs impl assy,$(call HdlContSource,$1,$s))
BitZName=$(HdlContDir)/$(Worker)-$1.bit.gz
# This happens in the target dir immediately before compilation
HdlContPreCompile=\
  echo Generating UUID and artifact xml file for container $1. && \
  (cd .. && $(OcpiGen) -D target-$1 -A -S $(Worker) -P $(HdlPlatform_$1) -e $(HdlTarget_$1) $1)
HdlContBitName=$(call HdlContDir,$1)/$(call BitFile_$(HdlToolSet_$(HdlTarget_$1)),$1)
HdlContBitZName=$(call HdlContDir,$1)/$(Worker)-$1.bit.gz
ContainerWorkersFile=$(HdlContDir)/$1.wks
ContainerXmlFile=$1.xml
ArtifactXmlName=$(call HdlContDir,$1)/$1-art.xml
HdlContOcpiGen=\
  $(OcpiGen) -D $(GeneratedDir) \
             -W $(Worker) $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	     $(if $(ComponentLibraries),\
                $(foreach l,$(ComponentLibraries),\
                  -L $(notdir $l):$(call HdlXmlComponentLibrary,$l)/hdl)) \
	     $(and $(HdlPlatform),-P $(HdlPlatform)) \
             -S $(Worker)
################################################################################
# The function to evaluate for each container
define doContainer

##### Generate source files for the container

# FIXME: we could share this with workers better

$(call HdlContSource,$1,defs): $1.xml | $(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $$@ from $$<
	$(AT)$(HdlContOcpiGen) -d $$<

$(call HdlContSource,$1,impl): $1.xml | $(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $$@ from $$<
	$(AT)$(HdlContOcpiGen) -i $$<

$(call HdlContSource,$1,assy): $1.xml | $(GeneratedDir)
	$(AT)echo Generating the container assembly source file: $$@ from $$<
	$(AT)$(HdlContOcpiGen) -a $$<

###### Define the bitstream.
###### Some tools may in fact make a container core, but some will not when
###### it is a top-level/bit-stream build

# This is for building the top level container core
$(call HdlContainer,$1): | $(call HdlContDir,$1)
$(call HdlContainer,$1): HdlPreCompile=$(call HdlContPreCompile,$1)
$(call HdlContainer,$1): $(call HdlContSources,$1)
$(call HdlContainer,$1): Core:=$1
$(call HdlContainer,$1): Top:=$1
$(call HdlContainer,$1): HdlMode:=container
$(call HdlContainer,$1): HdlLibraries+=../$(Worker)
$(call HdlContainer,$1): LibName:=$1
$(call HdlContainer,$1): XmlIncludeDirs+=$(HdlPlatformsDir)/$(HdlPlatform_$1)
$(call HdlContainer,$1): HdlTarget:=$(call HdlGetPart,$(HdlPlatform_$1))
$(call HdlContainer,$1): HdlSources:=$(call HdlContSources,$1)
$(call HdlContainer,$1): TargetDir=$(call HdlContDir,$1)
$(call HdlContainer,$1): \
	Cores=$(HdlPlatformsDir)/$(HdlPlatform_$1)/target-$(HdlConfig_$1)/$(HdlConfig_$1)_rv \
              $(HdlPlatformsDir)/$(HdlPlatform_$1)/target-$(HdlPlatform_$1)/$(HdlPlatform_$1) \
	      pcie_4243_trn_v6_gtx_x4_250 \
	      target-$(call HdlGetFamily,$(HdlPlatform_$1))/$(Worker)

$(call HdlContainer,$1):
	$(AT)echo Building container core \"$1\" for target \"$(HdlPlatform_$1)\"
	$(AT)$$(HdlCompile)

$(call HdlContDir,$1):
		$(AT)mkdir $$@

$(call HdlContBitName,$1): override ImplWorkersFiles=$(AssyWorkersFile) $(call ContainerWorkersFile,$1)
$(call HdlContBitName,$1): $(call HdlContainer,$1)
$(call HdlContBitZName,$1): $(call HdlContBitName,$1)
	$(AT)echo Making compressed bit file: $$@ from $(call HdlContBitName,$1) and $(call ArtifactXmlName,$1)
	$(AT)gzip -c $(call HdlContBitName,$1) > $$@
	$(AT)$(ToolsDir)/../../scripts/addmeta $(call ArtifactXmlName,$1) $$@

all: $(call HdlContBitZName,$1)
# Maybe the platform and not the tool defines how things build...
-include $(HdlPlatformsDir)/$(HdlPlatform_$1)/$(HdlPlatform_$1).mk

# Now invoke the tool-specific build with: <target-dir>,<assy-name>,<core-file-name>,<config>,<platform>
$(call HdlToolDoPlatform_$(HdlToolSet_$(HdlTarget_$1)),$(call HdlContDir,$1),$(Worker),$1,$1,$(HdlPlatform_$1))

HdlContainerCores+=$(call HdlContainer,$1)
HdlContainerBitZNames+=$(call HdlContBitZName,$1)
endef

$(foreach c,$(Containers),\
  $(info For container:$c:$(call doContainer,$c))$(eval $(call doContainer,$c)))
#  $(eval $(call doContainer,$c)))

all: $(HdlBitZNames)

# Force all workers to be built first for all targets
containers: cores $(HdlContainers)
all: containers
endif # of skip
endif # of containers
endif # of skip
endif
