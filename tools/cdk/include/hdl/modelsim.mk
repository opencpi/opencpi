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

# This file has the HDL tool details for modelsim

################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.
# In modelsim the result is a library directory that is always built all at once, and is
# always removed entirely each time it is built.  It is so fast that there is no
# point in fussing over "incremental" mode.
# So there not a specific file name we can look for
HdlToolLibraryFile=$(LibName)
################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=modelsim
################################################################################
# Variable required by toolset: what is the name of the file or directory that
# is the thing created when a library is created. The thing that will be installed
HdlToolLibraryResult=$(LibName)
################################################################################
# Variable required by toolset: HdlToolCoreLibName
# What library name should we give to the library when a core is built from
# sources
HdlToolCoreLibName=$(Core)
################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
# Note we can't build cores for further building, only simulatable "tops"
HdlBin=
################################################################################
# Variable required by toolset: HdlToolRealCore
# Set if the tool can build a real "core" file when building a core
# I.e. it builds a singular binary file that can be used in upper builds.
# If not set, it implies that only a library containing the implementation is
# possible
HdlToolRealCore=
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=
################################################################################
# Function required by toolset: $(call HdlToolLibRef,libname)
# This is the name after library name in a path
# It might adjust (genericize?) the target
HdlToolLibRef=modelsim

ModelsimFiles=\
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f)))
$(call OcpiDbgVar,ModelsimFiles)


ModelsimVlogLibs=

ModelSimVlogIncs=\
  $(foreach d,$(VerilogDefines),+define+$d) \
  $(foreach d,$(VerilogIncludeDirs),+incdir+$(call FindRelative,$(TargetDir),$d))

ModelsimArgs=-pedanticerrors -work $(LibName) -modelsimini modelsim.ini

HdlToolCompile=\
  (echo '; This file is generated for building this '$(LibName)' library.';\
   echo '[library]' ; \
   $(foreach l,$(HdlLibrariesInternal),\
      echo $(lastword $(subst -, ,$(notdir $l)))=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$l,$(HdlTarget)))));) \
   echo others=$(OCPI_MODELSIM_DIR)/modelsim.ini \
   ) > modelsim.ini ; \
   export LM_LICENSE_FILE=$(OCPI_MODELSIM_LICENSE_FILE); \
   rm -r -f $(LibName); \
   $(if $(filter work,$(LibName)),,$(OCPI_MODELSIM_DIR)/bin/vlib $(LibName) &&) \
   $(and $(filter %.v,$(ModelsimFiles)),\
    $(OCPI_MODELSIM_DIR)/bin/vlog $(ModelSimVlogIncs) $(VlogLibs) $(ModelsimArgs) $(filter %.v, $(ModelsimFiles)) ;) \
   $(and $(filter %.vhd,$(ModelsimFiles)),\
    $(OCPI_MODELSIM_DIR)/bin/vcom -preserve -bindAtCompile -error 1253 $(ModelsimArgs) $(filter %.vhd,$(ModelsimFiles)))

# Since there is not a singular output, make's builtin deletion will not work
HdlToolPost=\
  if test $$HdlExit != 0; then \
    rm -r -f $(LibName); \
  else \
    touch $(LibName);\
  fi;

ModelsimPlatform:=modelsim_pf
ModelsimAppName=$(call AppName,$(ModelsimPlatform))
ExeFile=$(ModelsimAppName).exe
BitFile=$(ModelsimAppName).bit
BitName=$(call PlatformDir,$(ModelsimPlatform))/$(BitFile)
ModelsimPlatformDir=$(HdlPlatformsDir)/$(ModelsimPlatform)
ModelsimTargetDir=$(call PlatformDir,$(ModelsimPlatform))


ModelsimBuildCmd=\
  $(eval $(HdlSetWorkers)) \
  $(eval MyLibMap:=$(strip \
    $(foreach l,$(HdlLibrariesInternal),\
       $(lastword $(subst -, ,$(notdir $l)))=$(strip \
         $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$l,$(HdlTarget)))))) \
    $(foreach w,$(HdlWorkers),\
      $(foreach f,$(firstword \
                    $(or $(foreach c,$(ComponentLibraries), \
                           $(foreach d,$(call HdlComponentLibrary,$c,modelsim),\
	                     $(wildcard $d/$w))),\
                       $(error Worker $w not found in any component library.))),\
        $w=$(call FindRelative,$(TargetDir),$f))) \
    $(ModelsimPlatform)=$(ModelsimPlatformDir)/target-modelsim/modelsim_pf \
    mkOCApp4B=mkOCApp4B \
    $(Worker)=../target-modelsim/$(Worker))) \
  $(eval MyLibs:=$(foreach m,$(MyLibMap),$(firstword $(subst =, ,$m)))) \
  (echo '; This file is generated for building this '$(LibName)' library.';\
   echo '[library]' ; \
   $(foreach l,$(MyLibMap),echo $l;) \
   echo others=$(OCPI_MODELSIM_DIR)/modelsim.ini; \
   ) > $(ModelsimAppName).ini ; \
   echo $(foreach l,$(MyLibs),-L $l) > vsim.args; \
   export LM_LICENSE_FILE=$(OCPI_MODELSIM_LICENSE_FILE); \
   echo 'log -r /*; archive write vsim.dbar -wlf vsim.wlf -include_src ; quit' | \
   $(OCPI_MODELSIM_DIR)/bin/vsim -c modelsim_pf.main -modelsimini $(ModelsimAppName).ini \
	-f vsim.args && \
    echo vsim exited successfully, now creating archive: $(BitName) && \
    pax -wf $(BitFile) -L vsim.dbar vsim.args metadatarom.data \
      -s =../target-modelsim/$(Worker)/modelsim== \
      $(foreach l,$(MyLibMap),-s =$(word 2,$(subst =, ,$l))=$(firstword $(subst =, ,$l))=)  \
      $(foreach l,$(MyLibMap),$(word 2,$(subst =, ,$l))) \

define HdlToolDoPlatform
# Generate bitstream
$$(BitName): TargetDir=$(call PlatformDir,$(ModelsimPlatform))
$$(BitName): HdlToolCompile=$$(ModelsimBuildCmd)
$$(BitName): HdlToolSet=vsim
$$(BitName): HdlToolPost=
$$(BitName): override HdlTarget=modelsim
$$(BitName): HdlName=$(ModelsimAppName)
$$(BitName): $(ModelsimPlatformDir)/target-modelsim/$(ModeisimPlatform) $(ModelsimTargetDir)/mkOCApp4B | $(ModelsimTargetDir)
	$(AT)echo Building modelsim simulation executable: $$(BitName).  Details in $$(ModelsimAppName)-vsim.out
	$(AT)$$(HdlCompile)
endef

