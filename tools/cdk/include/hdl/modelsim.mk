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

# This file has the HDL tool details for modelsim

################################################################################
# Name of the tool that needs to be installed to use this compilation flow
HdlToolName_modelsim=Modelsim

# This variable is needed when including this file
# for the sole purpose of extracting variable information
# E.g. if you want to know some information about a supported tool,
# you should not need the corresponding toolchain installed
ifndef __ONLY_TOOL_VARS__

################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.
# In modelsim the result is a library directory that is always built all at once, and is
# always removed entirely each time it is built.  It is so fast that there is no
# point in fussing over "incremental" mode.
# So there not a specific file name we can look for
HdlToolLibraryFile=$2
################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=modelsim
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
#
# For this tool, it is not sufficient just to include the assembly and pfconfig
# at the container level. We must also include app workers, devices and the PW
HdlToolRequiresFullCoreHierarchy_modelsim=yes
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=

################################################################################
# Function required by toolset: $(call HdlToolCoreRef,coreref)
# Modify a stated core reference to be appropriate for the tool set
HdlToolCoreRef=$(call HdlRmRv,$1)
HdlToolCoreRef_modelsim=$(call HdlRmRv,$1)

ModelsimFiles=\
  $(foreach f,$(HdlSources),\
     $(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f)))
$(call OcpiDbgVar,ModelsimFiles)

# Modelsim seems to have 32-bit portions, so needs the inode spoofing preloaded
ModelsimExec=LD_PRELOAD=$(OCPI_CDK_DIR)/$(OCPI_TOOL_DIR)/lib/inode64.so $(OCPI_MODELSIM_DIR)/linuxpe/$1

ModelsimVlogLibs=

# FIXME: make . in the include path for primitives as well as workers
ModelSimVlogIncs=\
  $(foreach d,$(VerilogDefines),+define+$d) +incdir+.. \
  $(foreach d,$(VerilogIncludeDirs),+incdir+$(call FindRelative,$(TargetDir),$d))

ModelsimArgs=-pedanticerrors -work $(WorkLib) -modelsimini modelsim.ini

HdlToolCompile=\
  (echo '; This file is generated for building this '$(LibName)' library.';\
   echo '[library]' ; \
   $(foreach l,$(HdlLibrariesInternal),$(infox LLL:$l)\
      echo $(lastword $(subst -, ,$(notdir $l)))=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$l,$(HdlTarget),,modelsim))));) \
   $(foreach c,$(call HdlCollectCorePaths),\
      echo $(call HdlRmRv,$(notdir $(c)))=$(call FindRelative,$(TargetDir),$(strip \
          $(call HdlCoreRef,$(call HdlRmRv,$c),modelsim)));) \
   echo others=$(OCPI_MODELSIM_DIR)/modelsim.ini \
   ) > modelsim.ini ; \
   export LM_LICENSE_FILE=$(OCPI_MODELSIM_LICENSE_FILE); \
   rm -r -f $(WorkLib); \
   $(if $(filter work,$(LibName)),,$(call ModelsimExec,vlib) $(WorkLib) &&) \
   $(and $(filter %.v,$(ModelsimFiles)),\
    $(call ModelsimExec,vlog) $(ModelSimVlogIncs) $(VlogLibs) $(ModelsimArgs) $(filter %.v, $(ModelsimFiles)) ;) \
   $(and $(filter %.vhd,$(ModelsimFiles)),\
    $(call ModelsimExec,vcom) -preserve $(if $(HdlNoSimElaboration),,$(ignore -bindAtCompile)) -error 1253 $(ModelsimArgs) $(filter %.vhd,$(ModelsimFiles)))

# Since there is not a singular output, make's builtin deletion will not work
HdlToolPost=\
  touch $(WorkLib);

BitFile_modelsim=$1.tar

define HdlToolDoPlatform_modelsim

# Generate bitstream
$1/$3.tar:
	$(AT)echo Building modelsim simulation executable: "$$@" with details in $1/$3-modelsim.out
	$(AT)(set -e ; cd $1 && \
	     echo -L $3 $$$$(grep = modelsim.ini | grep -v others= | sed 's/=.*//' | sed 's/^/-L /') > vsim.args && \
	     export MGLS_LICENSE_FILE=$(OCPI_MODELSIM_LICENSE_FILE) && \
	     echo 'log -r /*; archive write vsim.dbar -wlf vsim.wlf -include_src ; quit' | \
	     $(call ModelsimExec,vsim) -c $3.$3 -modelsimini modelsim.ini \
	       -f vsim.args && \
             echo vsim exited successfully, now creating archive: $$@ && \
             tar -cf $$(notdir $$@) -h vsim.dbar vsim.args metadatarom.dat \
	       $$(foreach i,$$(shell grep = $1/modelsim.ini | grep -v others=),\
                 $$(foreach l,$$(firstword $$(subst =, ,$$i)),\
                   $$(foreach p,$$(word 2,$$(subst =, ,$$i)),\
                     $$(foreach s,$$(if $$(filter /%,$$p),$$(patsubst /%,%,$$p),$$(subst ../,,$$p)),\
                       --xform='s=$$s$$$$=$$l=' --xform='s=$$s/=$$l/=' $$p )))) $3 ) > $1/$3-modelsim.out 2>&1

endef
endif
