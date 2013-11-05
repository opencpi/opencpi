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
# This makefile is for building platforms and platform configurations
# A platform worker is a device worker that implements the platform_spec.xml
# A platform worker is built, and then some number of platform configurations
# are built.  The platform configurations are used when bitstreams
# get built elsewhere based on assemblies and configurations

HdlMode:=platform
HdlLibraries+=platform
# We're not really a component library, so we force this
XmlIncludeDirs+=../specs
override ComponentLibraries+=../../devices
include $(OCPI_CDK_DIR)/include/util.mk
include $(OCPI_CDK_DIR)/include/hdl/hdl-pre.mk
ifndef HdlSkip
# add xml search in component libraries
ifneq ($(MAKECMDGOALS),clean)
$(eval $(HdlSearchComponentLibraries))
endif
################################################################################
# We are like a worker (and thus a core)
# But we're VHDL only, so we only need to build the rv core
# which is the "app" without container or the platform
# FIXME: we can't do this yet because the BB library name depends on there being both cores...
#Tops:=$(Worker)_rv

# Generate the worker's bb file by simply linking to the defs

#hdlCoreBlackBoxFile:=$(GeneratedDir)/$(Worker)_bb$(HdlSourceSuffix)
#$(CoreBlackBoxFile): $$(DefsFile) | $$(OutDir)gen
#	$(AT)$(call MakeSymLink2,$(DefsFile),$(GeneratedDir),$(notdir $@))

include $(OCPI_CDK_DIR)/include/hdl/hdl-worker.mk

################################################################################
# From here its about building the platform configuration cores
ifdef Configurations

HdlConfigDir=$(OutDir)target-$1
HdlConfig=$(call HdlConfigDir,$1)/$1$(and $(HdlToolReadCore),_rv$(HdlBin))
HdlConfigSource=$(GeneratedDir)/$1-$2.vhd
HdlConfigSources=$(foreach s,defs impl assy,$(call HdlConfigSource,$1,$s))
################################################################################
# The function to evaluate for each configuration
define doConfiguration

# FIXME: we could share this with workers better

$(call HdlConfigSource,$1,defs): $1.xml | $(GeneratedDir)
	$(AT)echo Generating the platform configuration assembly source file: $$@ from $$<
	$(AT)$$(OcpiGen) -W $(Worker) $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	 $(if $(ComponentLibraries),$(foreach l,$(ComponentLibraries),\
          -L $(notdir $l):$(call HdlXmlComponentLibrary,$l)/hdl)) \
	 -D $(GeneratedDir) -d  $(and $(HdlPlatform),-P $(HdlPlatform)) $$<

$(call HdlConfigSource,$1,impl): $1.xml | $(GeneratedDir)
	$(AT)echo Generating the platform configuration assembly source file: $$@ from $$<
	$(AT)$$(OcpiGen) -W $(Worker) $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	 $(if $(ComponentLibraries),$(foreach l,$(ComponentLibraries),\
          -L $(notdir $l):$(call HdlXmlComponentLibrary,$l)/hdl)) \
	 -D $(GeneratedDir) -i  $(and $(HdlPlatform),-P $(HdlPlatform)) $$<

$(call HdlConfigSource,$1,assy): $1.xml | $(GeneratedDir)
	$(AT)echo Generating the platform configuration assembly source file: $$@ from $$<
	$(AT)$$(OcpiGen) -W $(Worker) $(if $(Libraries),$(foreach l,$(Libraries),-l $l)) \
	 $(if $(ComponentLibraries),$(foreach l,$(ComponentLibraries),\
          -L $(notdir $l):$(call HdlXmlComponentLibrary,$l)/hdl)) \
	 -D $(GeneratedDir) -W $1 -a  $(and $(HdlPlatform),-P $(HdlPlatform)) $$<

HdlConfigs+= $(call HdlConfig,$1)

$(call HdlConfig,$1): | $(call HdlConfigDir,$1)
$(call HdlConfig,$1): $(call HdlConfigSources,$1)
$(call HdlConfig,$1): Core:=$1_rv
$(call HdlConfig,$1): Top:=$1_rv
$(call HdlConfig,$1): HdlMode:=config
$(call HdlConfig,$1): LibName:=$1
$(call HdlConfig,$1): HdlTarget:=$(HdlExactPart)
$(call HdlConfig,$1): HdlSources:=$(call HdlConfigSources,$1)
$(call HdlConfig,$1): TargetDir=$(call HdlConfigDir,$1)
# This allows the platform worker to be found
$(call HdlConfig,$1): target-$(call HdlGetFamily,$(HdlExactPart))/$(Worker)
$(call HdlConfig,$1): override ComponentLibraries+=target-$(call HdlGetFamily,$(HdlExactPart))/$(Worker)
# This causes the workers file to be read to add to the cores list
$(call HdlConfig,$1): override ImplWorkersFile=$(GeneratedDir)/$1.wks
# This indicates where the assy impl file is, for the $1.wks file
$(call HdlConfig,$1): override ImplFile=$$(call HdlConfigSource,$1,assy)
#ifdef HdlToolNeedBB
#AssyBlackBoxFile_$1:=$(GeneratedDir)/$1_bb$(HdlSourceSuffix)
#$$(AssyBlackBoxFile_$1): $(call HdlConfigSource,$1,defs) | $$(OutDir)gen
#	$(AT)$$(call MakeSymLink2,$(call HdlConfigSource,$1,defs),$$(GeneratedDir),$$(notdir $$@))
#CoreBlackBoxFiles+=$(call HdlConfigSource,$1,defs)
#endif
$(call HdlConfigDir,$1):
		$(AT)mkdir $$@
endef

ifneq ($(MAKECMDGOALS),clean)
$(foreach c,$(Configurations),\
  $(eval $(call doConfiguration,$c)) \
  $(eval $(call DoBBLibraryTarget,$c,$c,$(HdlExactPart),$(call HdlConfigSource,$c,defs))))
endif

$(HdlConfigs): $$(HdlPreCore)
	$(AT)echo Building platform configuration core \"$@\" for target \"$(HdlTarget)\"
	$(AT)$(HdlCompile)

all: configs $(BBLibResults)

configs: worker $(HdlConfigs)

# FIXME: These should be more modular: something like $(WorkerResults)
worker: $(CoreResults) $(OutLibFiles)

#$(call HdlConfigDir,$(Worker))/$(Worker)_rv$(HdlBin)

endif # Configurations: FIXME make the default empty config...

endif # end of HdlSkip

