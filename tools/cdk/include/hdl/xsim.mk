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

# This file has the HDL tool details for vivado simm

include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk

################################################################################
# $(call HdlToolLibraryRefFile,libname,target)
# Function required by toolset: return the file for dependencies on the library
# This is attached to a directory, with a slash
# Thus it is empty if the library is a diretory full of stuff that doesn't
# have a file name we can predict.

# XST: libraries are named differently depending on the source language
# so we can't know them unfortunately (for referencing).
HdlToolLibraryRefFile=

################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.
# In xsim the result is a library directory that is always built all at once, and is
# always removed entirely each time it is built.  It is so fast that there is no
# point in fussing over "incremental" mode.
# So there not a specific file name we can look for
HdlToolLibraryFile=$(LibName)

################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=xsim
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
HdlToolLibRef=xsim

XsimFiles=\
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f)))
$(call OcpiDbgVar,XsimFiles)

#        -lib $(notdir $(l))_$(notdir $(w))=$(strip\

XsimLibraries=$(HdlLibrariesInternal)
XsimLibs=\
  $(and $(filter assembly container,$(HdlMode)),\
  $(eval $(HdlSetWorkers)) \
  $(foreach w,$(HdlWorkers),\
    $(foreach f,$(firstword \
                  $(or $(foreach c,$(ComponentLibraries), \
                         $(foreach d,$(call HdlComponentLibrary,$c,xsim),\
	                   $(wildcard $d/$w))),\
                       $(error Worker $w not found in any component library.))),\
      -lib $w=$(call FindRelative,$(TargetDir),$f)))) \
  $(foreach l,\
    $(HdlLibrariesInternal) $(Cores),\
    -lib $(notdir $(l))=$(strip \
          $(call FindRelative,$(TargetDir),$(call HdlLibraryRefDir,$l,xsim))))

XsimVerilogIncs=\
  $(foreach d,$(VerilogDefines),-d $d) \
  $(foreach d,$(VerilogIncludeDirs),-i $(call FindRelative,$(TargetDir),$(d))) \
  $(foreach l,$(call HdlXmlComponentLibraries,$(ComponentLibraries)),-i $(call FindRelative,$(TargetDir),$l))

ifndef XsimTop
XsimTop=$(Worker).$(Worker)
endif

XsimArgs=\
  -initfile xsim.ini \
  -v 2 \
  -work $(call ToLower,$(LibName))=$(LibName) \

HdlToolCompile=\
  (echo '-- This file is generated for building this '$(LibName)' library.  Used for both VHDL and verilog';\
   $(foreach l,$(HdlLibrariesInternal),\
      echo $(lastword $(subst -, ,$(notdir $l)))=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$l,$(HdlTarget)))));) \
   ) > xsim.ini ; \
   $(VivadoXilinx); \
   $(and $(filter %.vhd,$(XsimFiles)),\
     xvhdl $(XsimArgs) $(filter %.vhd,$(XsimFiles)) ; ) \
   $(and $(filter %.v,$(XsimFiles))$(findstring $(HdlMode),platform),\
     xvlog $(XsimVerilogIncs) $(XsimArgs) $(filter %.v,$(XsimFiles)) \
       $(and $(findstring $(HdlMode),platform),\
         $(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/glbl.v) ;) \
  $(if $(findstring $(HdlMode),worker),\
    echo verilog work $(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/glbl.v \
	> $(Worker).prj; \
    $(if $(HdlSkipSimElaboration),, \
      xelab $(XsimTop) work.glbl -timescale 1ns/1ps -v 2 -prj $(Worker).prj -L unisims_ver \
	-s $(Worker).exe -lib $(Worker)=$(Worker) $(XsimLibs);)) \
  $(if $(findstring $(HdlMode),platform),\
    echo verilog work ../../../containers/mkOCApp_bb.v > $(Worker).prj ; \
    xelab $(XsimTop) $(Worker).glbl -timescale 1ns/1ps -v 2 -prj $(Worker).prj -L unisims_ver \
	-s $(Worker).exe -lib work=work -lib $(call ToLower,$(Worker))=$(Worker) $(XsimLibs) ;)

# Since there is not a singular output, make's builtin deletion will not work
HdlToolPost=\
  if test $$HdlExit != 0; then \
    rm -r -f $(LibName); \
  else \
    touch $(LibName);\
  fi;

XsimPlatform:=xsim_pf
XsimAppName=$(call AppName,$(XsimPlatform))
ExeFile=$(XsimAppName).exe
BitFile=$(XsimAppName).bit
BitName=$(call PlatformDir,$(XsimPlatform))/$(BitFile)
XsimPlatformDir=$(HdlPlatformsDir)/$(XsimPlatform)
XsimTargetDir=$(call PlatformDir,$(XsimPlatform))
XsimFuseCmd=\
  $(VivadoXilinx); xelab $(XsimPlatform).main $(XsimPlatform).glbl -v 2 -debug typical \
		-lib $(XsimPlatform)=$(XsimPlatformDir)/target-xsim/$(XsimPlatform) \
		-lib mkOCApp4B=mkOCApp4B \
	        -lib $(Worker)=../target-xsim/$(Worker) \
	$$(XsimLibs) -L unisims_ver -s $(ExeFile) && \
	tar -c -f $(BitFile) xsim.dir metadatarom.data

define HdlToolDoPlatform
# Generate bitstream
$$(BitName): TargetDir=$(call PlatformDir,$(XsimPlatform))
$$(BitName): HdlToolCompile=$(XsimFuseCmd)
$$(BitName): HdlToolSet=xelab
$$(BitName): override HdlTarget=xsim
$$(BitName): $(XsimPlatformDir)/target-xsim/$(XsimPlatform) $(XsimTargetDir)/mkOCApp4B | $(XsimTargetDir)
	$(AT)echo Building xsim simulation executable: $$(BitName).  Details in $$(XsimAppName)-xelab.out
	$(AT)$$(HdlCompile)
endef
