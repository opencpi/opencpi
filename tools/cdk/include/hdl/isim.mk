# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
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

# This file has the HDL tool details for isim

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
# In isim the result is a library directory that is always built all at once, and is
# always removed entirely each time it is built.  It is so fast that there is no
# point in fussing over "incremental" mode.
# So there not a specific file name we can look for
HdlToolLibraryFile=$2
################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=isim
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
HdlToolLibRef=isim

# filter %.v %.V,$^),\

IsimFiles=\
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f)))
$(call OcpiDbgVar,IsimFiles)

#        -lib $(notdir $(l))_$(notdir $(w))=$(strip\

IsimLibs=\
  $(and $(filter assembly container,$(HdlMode)),\
  $(eval $(HdlSetWorkers)) \
  $(foreach w,$(HdlWorkers),\
    $(foreach f,$(firstword \
                  $(or $(foreach c,$(ComponentLibraries), \
                         $(foreach d,$(call HdlComponentLibrary,$c,isim),\
	                   $(wildcard $d/$w))),\
                       $(error Worker $w not found in any component library.))),\
      -lib $w=$(call FindRelative,$(TargetDir),$f)))) \
  $(foreach l,\
    $(HdlLibraries) $(Cores) $(and $(findstring $(HdlMode),worker device platform assembly container),\
				   util_xilinx),\
    -lib $(notdir $(l))=$(strip \
          $(call FindRelative,$(TargetDir),$(call HdlLibraryRefDir,$l,isim))))

MyIncs=\
  $(foreach d,$(VerilogDefines),-d $d) \
  $(foreach d,$(VerilogIncludeDirs),-i $(call FindRelative,$(TargetDir),$(d))) \
  $(foreach l,$(call HdlXmlComponentLibraries,$(ComponentLibraries)),-i $(call FindRelative,$(TargetDir),$l))

ifndef IsimTop
IsimTop=$(Worker).$(Worker)
endif

HdlToolCompile=\
  $(Xilinx) $(call OcpiDbgVar,IsimFiles,htc) $(call OcpiDbgVar,SourceFiles,htc) $(call OcpiDbgVar,CompiledSourceFiles,htc) $(call OcpiDbgVar,CoreBlackBoxFile,htc) $(if $(filter .v,$(suffix $(firstword $(IsimFiles)))),vlogcomp $(MyIncs),vhpcomp) -v 2 -work $(call ToLower,$(LibName))=$(LibName) $(IsimLibs) \
     $(IsimFiles) $(if $(findstring $(HdlMode),platform),\
	 	       $(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/glbl.v) \
  && ls $(if $(findstring $(HdlMode),worker), && \
    echo verilog work $(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/glbl.v \
	> $(Worker).prj \
    $(if $(HdlSkipSimElaboration),,&& ls &&  \
      fuse $(IsimTop) work.glbl -v 2 -prj $(Worker).prj -L unisims_ver \
	-o $(Worker).exe -lib $(call ToLower,$(Worker))=$(Worker) $(IsimLibs))) \
  $(if $(findstring $(HdlMode),platform), && \
    echo verilog work ../../../containers/mkOCApp_bb.v > $(Worker).prj && \
    fuse $(IsimTop) $(Worker).glbl -v 2 -prj $(Worker).prj -L unisims_ver \
	-o $(Worker).exe -lib work=work -lib $(call ToLower,$(Worker))=$(Worker) $(IsimLibs))

# the "touch" above is because isim creates a bunch of files for the precompiled library
# and no specific file.  So we touch the dir so dependencies on the library work even when
# individual files get updated.


# Since there is not a singular output, make's builtin deletion will not work
HdlToolPost=\
  if test $$HdlExit != 0; then \
    rm -r -f $(LibName); \
  else \
    touch $(LibName);\
  fi;

IsimPlatform:=isim_pf
IsimAppName=$(call AppName,$(IsimPlatform))
ExeFile=$(IsimAppName).exe
BitFile=$(IsimAppName).bit
BitName=$(call PlatformDir,$(IsimPlatform))/$(BitFile)
IsimPlatformDir=$(HdlPlatformsDir)/$(IsimPlatform)
IsimTargetDir=$(call PlatformDir,$(IsimPlatform))
IsimFuseCmd=\
  $(Xilinx) fuse  isim_pf.main isim_pf.glbl -v 2 \
		-lib isim_pf=$(IsimPlatformDir)/target-isim/isim_pf \
		-lib mkOCApp4B=mkOCApp4B \
	        -lib $(Worker)=../target-isim/$(Worker) \
	$$(IsimLibs) -L unisims_ver -o $(ExeFile) && \
	tar cf $(BitFile) $(ExeFile) metadatarom.data isim

#		-lib work=$(IsimPlatformDir)/target-isim/isim/work \
#" > $$(call AppName,$(IsimPlatform))-fuse.out 2>&1


define HdlToolDoPlatform
# Generate bitstream
$$(BitName): TargetDir=$(call PlatformDir,$(IsimPlatform))
$$(BitName): HdlToolCompile=$(IsimFuseCmd)
$$(BitName): HdlToolSet=fuse
$$(BitName): $(IsimPlatformDir)/target-isim/$(IsimPlatform) $(IsimTargetDir)/mkOCApp4B | $(IsimTargetDir)
	$(AT)echo Building isim simulation executable: $$(BitName).  Details in $$(IsimAppName)-fuse.out
	$(AT)$$(HdlCompile)
endef

ifdef FFF
	$(AT)$(TIME) sh -c "cd $$(TargetDir); \
	$(Xilinx) fuse  isim_pf.main work.glbl \
		-lib isim_pf=$(IsimPlatformDir)/target-isim/isim_pf \
		-lib work=$(IsimPlatformDir)/target-isim/isim/work \
		-lib mkOCApp4B=$(IsimTargetDir)/mkOCApp4B \
	$$(IsimLibs) -L unisims_ver -o $(BitFile)" > $$(call AppName,$(IsimPlatform))-fuse.out 2>&1

endif


# fancier looking at output file?
ifneq (,)
  if grep -q 'Number of errors   :    0 ' xst-$(Core).out; then \
    ngc2edif -w $(Core).ngc >> xst-$(Core).out; \
    exit 0; \
  else \
    exit 1; \
  fi
  if test $$EC = 0
endif
