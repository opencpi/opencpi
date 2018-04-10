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

# This file has the HDL tool details for isim

################################################################################
# Name of the tool that needs to be installed to use this compilation flow
HdlToolName_isim=ISE

# This variable is needed when including this file
# for the sole purpose of extracting variable information
# E.g. if you want to know some information about a supported tool,
# you should not need the corresponding toolchain installed
ifndef __ONLY_TOOL_VARS__

include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk
toolsDir:=$(call OcpiXilinxIseDir,$(and $(filter clean%,$(MAKECMDGOALS)),warning))
################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.
# In isim the result is a library directory that is always built all at once, and is
# always removed entirely each time it is built.  It is so fast that there is no
# point in fussing over "incremental" mode.
# So there is not a specific file name we can look for
HdlToolLibraryFile=$2
################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=isim
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
# Function required by toolset: $(call HdlToolCoreRef,coreref)
# Modify a stated core reference to be appropriate for the tool set
HdlToolCoreRef=$(call HdlRmRv,$1)
HdlToolCoreRef_isim=$(call HdlRmRv,$1)

IsimFiles=\
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f)))
$(call OcpiDbgVar,IsimFiles)

IsimCoreLibraryChoices=$(strip \
  $(foreach c,$(call HdlRmRv,$1),$(call HdlCoreRef,$c,isim)))

IsimLibs=\
    $(foreach l,\
      $(HdlLibrariesInternal),\
      -lib $(notdir $l)=$(strip \
            $(call FindRelative,$(TargetDir),$(call HdlLibraryRefDir,$l,isim,,isim)))) \
    $(foreach c,$(call HdlCollectCores,isim),$(infox CCC:$c)\
      -lib $(call HdlRmRv,$(notdir $(c)))=$(infox fc:$c)$(call FindRelative,$(TargetDir),$(strip \
          $(firstword $(foreach l,$(call IsimCoreLibraryChoices,$c),$(call HdlExists,$l))))))

MyIncs=\
  $(foreach d,$(VerilogDefines),-d $d) \
  $(foreach d,$(VerilogIncludeDirs),-i $(call FindRelative,$(TargetDir),$(d))) \
  $(foreach l,$(HdlXmlComponentLibraries),-i $(call FindRelative,$(TargetDir),$l))

IsimArgs=-v 2 -work $(call ToLower,$(WorkLib))=$(WorkLib) $(IsimLibs)

HdlToolCompile=\
  $(OcpiXilinxIseInit);\
  $(call OcpiDbgVar,IsimFiles,htc) $(call OcpiDbgVar,SourceFiles,htc) $(call OcpiDbgVar,CompiledSourceFiles,htc)\
  $(and $(filter %.v,$(IsimFiles))$(findstring $(HdlMode),platform),\
    vlogcomp $(MyIncs) $(IsimArgs) $(filter %.v,$(IsimFiles)) \
       $(and $(findstring $(HdlMode),platform), $(toolsDir)/ISE/verilog/src/glbl.v) ;) \
  $(and $(filter %.vhd,$(IsimFiles)),\
    vhpcomp $(IsimArgs) $(filter %.vhd,$(IsimFiles)) ;)\
  $(if $(filter worker platform config assembly,$(HdlMode))$(HdlIsimProbe),\
    $(if $(HdlNoSimElaboration),, \
      echo verilog work $(toolsDir)/ISE/verilog/src/glbl.v \
	> $(Worker).prj; \
      fuse $(WorkLib).$(WorkLib)$(and $(filter assembly config,$(HdlMode)),_rv) work.glbl -v 2 \
           -prj $(Worker).prj -L unisims_ver -o $(Worker).exe \
           -lib $(call ToLower,$(WorkLib))=$(WorkLib) $(IsimLibs)))

# the "touch" below is because isim creates a bunch of files for the precompiled library
# and no specific file.  So we touch the dir so dependencies on the library work even when
# individual files get updated.
# Since there is not a singular output, make's builtin deletion will not work
HdlToolPost=\
  if test $$HdlExit != 0; then \
    rm -r -f $(WorkLib); \
  else \
    touch $(WorkLib);\
  fi;

BitFile_isim=$1.tar

define HdlToolDoPlatform_isim

# Generate bitstream
$1/$3.tar:
	$(AT)echo Building isim simulation executable: "$$@" with details in $1/$3-fuse.out
	$(AT)echo verilog work $(toolsDir)/ISE/verilog/src/glbl.v > $1/$3.prj
	$(AT)(set -e; cd $1; $(call OcpiXilinxIseInit,); \
	      fuse $3.$3 work.glbl -v 2  -prj $3.prj \
              -lib $3=$3 $$(IsimLibs) -L unisims_ver \
	      -o $3.exe ; \
	      tar cf $3.tar $3.exe metadatarom.dat isim) > $1/$3-fuse.out 2>&1

endef

ifneq (,)

Notes:
Workers in an assembly need individual library treatment since we need to point to the actual
per-worker libraries.  ComponentLibraries need specific treatment to point to the worker cores.

endif
endif
