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

# This file has the HDL tool details for altera quartus_pro

################################################################################
# Name of the tool that needs to be installed to use this compilation flow
HdlToolName_quartus_pro=QuartusPro

# This variable is needed when including this file
# for the sole purpose of extracting variable information
# E.g. if you want to know some information about a supported tool,
# you should not need the corresponding toolchain installed
ifndef __ONLY_TOOL_VARS__

include $(OCPI_CDK_DIR)/include/hdl/altera.mk

################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built or touched when the library is changed or rebuilt.
#
# For quartus, no precompilation is available, so it is just a directory
# full of links whose name is the name of the library
HdlToolLibraryFile=$2

HdlRecurseLibraries_quartus_pro=yes

################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
#
# For quartus, it is generic since there is no processing
HdlToolLibraryTargets=altera
################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
HdlBin=.qdb
HdlBin_quartus_pro=.qdb
################################################################################
# Variable required by toolset: HdlToolRealCore
# Set if the tool can build a real "core" file when building a core
# I.e. it builds a singular binary file that can be used in upper builds.
# If not set, it implies that only a library containing the implementation is
# possible
HdlToolRealCore=yes
HdlToolRealCore_quartus_pro=yes
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=
HdlToolNeedBB_quartus_pro=
################################################################################
# Function required by toolset: $(call HdlToolCoreRef,coreref)
# Modify a stated core reference to be appropriate for the tool set
HdlToolCoreRef=$1
HdlToolCoreRef_quartus_pro=$1

ifeq ($(HdlMode),library)
## This allows primitive library .out files to have the correct name
## instead of 'onewire'
CoreOrLibName=$(LibName)
ifndef Core
Core:=onewire
Top:=onewire
endif
else
CoreOrLibName=$(Core)
endif


$(call OcpiDbgVar,HdlMode)
################################################################################
# Variable required by toolset: HdlToolNeedsSourceList_<tool>=yes
# Set if the tool requires a listing of source files and libraries
HdlToolNeedsSourceList_quartus_pro=yes

# This tool requires entity declarations next to the DEFS stubs.
# This means that the component declaration is not sufficient to use an already
# compiled worker's netlist, and an entity declaration is required as well.
HdlToolRequiresEntityStubs_quartus_pro=yes
# This tool requires built netlists/cores to be mapped to VHDL instances when
# imported into a design.
HdlToolRequiresInstanceMap_quartus_pro=yes
# Versions <= 17.0 merge subpartitions into the parent and therefore do not
# require the full hierarchy of cores. E.g. at the container level, the cores
# the assembly and pfconfig always need to be included, but the subcores for the
# platform worker, devices, and app workers only need to be included for 17.1+
HdlToolRequiresFullCoreHierarchy_quartus_pro=$(filter-out 16.% 17.0,$(call OcpiAlteraVersionFromEnv,pro))
################################################################################

QuartusSources=$(filter-out %.vh,$(HdlSources))

# Libraries can be built for specific targets, which just is for syntax checking
# Note that a library can be designed for a specific target
QuartusFamily_stratix4:=Stratix IV
QuartusFamily_stratix5:=Stratix V
QuartusFamily_arria10soc:=Arria 10

QuartusMakePart1=$(firstword $1)$(word 3,$1)$(word 2,$1)
QuartusMakePart=$(call QuartusMakePart1,$(subst -, ,$1))

QuartusMakeFamily=$(QuartusFamily_$(call HdlGetFamily,$1))

# Assign the FAMILY if it has been set.
# Use HdlChoosePart to set the DEVICE, but if the part is NONE, we do not set the device at all.
#   Note: this is done because certain families (Arria 10) do not support the AUTO device
#         construct. Instead, you can just set the FAMILY, and AUTO is implicit.
# arg 1 is HdlTarget
QuartusMakeDevices=$(infox QMD:$1)\
  $(and $(call QuartusMakeFamily,$1),\
    echo set_global_assignment -name FAMILY '\"'$(call QuartusMakeFamily,$1)'\"'; )\
  $(if $(filter-out NONE,$(HdlChoosePart)),\
    echo set_global_assignment -name DEVICE $(HdlChoosePart); )

# The constraint file(s) to use, first/only arg is platform
HdlConstraintsSuffix_quartus_pro=.qsf
QuartusConstraints_default=$(HdlPlatformDir_$1)/$1$(HdlConstraintsSuffix_quartus_pro)
QuartusConstraints=$(or $(HdlConstraints),$(QuartusConstraints_default))

# Collect the source files for all primitive libraries needed
# Collect the source/netlist files for all Cores/Workers/Assemblies/Containers... that are needed
# Collect the source files for the current asset being compiled
#   Omit the -ents.vhd file as it is meant for later inclusions/imports of this asset
# Include any verilog-include-dirs for the current asset being compiled
# All of this information is 'echo'ed in a format that can be put in a Quartus Pro QSF file for compilation
QuartusIncludeDependencies=\
  $(foreach l,$(call HdlCollectLibraries,$(HdlTarget)),$(infox IncLib:$l)\
    $(call QuartusIncludeSources,\
      $(call HdlExtractSourcesForLib,$(HdlTarget),$l,$(TargetDir)),\
      $(notdir $(call HdlRmRv,$l)),$1))\
  $(call QuartusIncludeCores,$(call HdlCollectCores),$(CoreOrLibName),$1)\
  $(call QuartusIncludeSources,$(foreach s,$(filter-out %/$(call HdlRmRv,$(CoreOrLibName))-ents.vhd,$(QuartusSources)),$(call FindRelative,$(TargetDir),$s)),$(CoreOrLibName),$1)\
  $(call QuartusIncludeIncludedirs,$(call Unique,$(patsubst %/,%,$(dir $(HdlSources)) $(VerilogIncludeDirs))),$(CoreOrLibName),$1)

# Call the FindRelative utility function from TargetDir
# (optionally appended with /$2 if $2 is non-empty) to $1
FindRelToTgtDirSubDir=$(strip \
  $(foreach path,$(call FindRelative,$(TargetDir)$(and $2,/$2),$1),\
    $(path)))

# Quartus limits partition names to 50 characters, and it appends its own 9 characters at the end.
# This leaves us with only 41 characters for our partition names. So, we use the first 8 characters
# of a core's instance name (after the last '|'), an underscore,
# and the 32 character md5sum of the name (41 chars total).
QuartusPartitionName=$(strip \
  $(call OcpiCallPythonFunc,import hashlib; print("$(lastword $(subst |, ,$1))"[0:8] + "_" + hashlib.md5("$1".encode("utf-8")).hexdigest())))

# Echo a string (for writing to QSF), but only during elaboration (if the syn flag is unset)
QuartusElabEcho=$(strip \
    $(if $(filter $2,syn),,echo $1; ))

# Echo a string (for writing to QSF), but only during elaboration (if the syn flag is unset)
# This command calls the hdl_files helper function in quartus_pro_qsf_gen.sh. This helper
# function takes an HDL library and a list of source files. It creates a Quartus Pro
# Tcl assignment for each source file based on file-extension.
QuartusElabEchoFiles=$(strip \
    $(if $(filter $3,syn),,hdl_files $1 $2; ))

# Echo a string (for writing to QSF), but only during synthesis (if the syn flag is set)
QuartusSynEcho=$(strip \
    $(if $(filter $2,syn),echo $1; ))

# Get the list of user-defined Cores, but only the core-name/path (firstword)
# This is used when determining whether a core is user-included in QuartusIncludeCores
CoreNames=$(foreach fullcore,$(Cores),$(firstword $(subst :, ,$(fullcore))) )

# Loop through each core in $1. These are in the format <core-name>:<core-path>:<core-HDL-instance>
# If in synthesis mode (not just elaboration), generate partition assignments for each core's QDB files
#   These partition assignments map the path to the QDB file to an HDL instance i the design.
# If a core is in the Cores list (user-defined list of cores to include for THIS asset),
#   Search for the core as we would for a library, to collect any source-files needed (e.g. stubs, _pkg.vhd, _bb.v)
# otherwise, this core was not user-included, so it is an OpenCPI core-like asset (core, worker, platform, config, container, assembly)
#   In this case, look for required source files like -defs, -ents, generics,... that must accompany OpenCPI cores
#
# NON-STANDARD FLOW IS DISABLED IN THIS BRANCH (Args 4 and 5 are never used in this branch):
#   Arg 4 is optional and is an instance hierarchy prefix to prepend any instance names. This is useful in non-standard flows where
#   the OpenCPI container is included in a larger project, and the instances need to be prepended with that larger project's
#   instance hierarchy to the container.
#   Similarly, Arg5 is used in the same non-standard flow when an external project is pulled in, and path names need to be
#   relative to that external project's local copy directory.
#   NOTE: Arg4 and 5 logic were left here because of how interleaved they are with the standard flow
QuartusIncludeCores=$(infox QIC:$1:$2:$3)\
  $(and $1,$(call QuartusSynEcho,'\#' Import QDB file for each core,$3) ) \
  $(foreach corestr,$1,$(infox CoreStr:$(corestr))\
    $(foreach corepath,$(word 2,$(subst :, ,$(corestr))),$(infox CorePath:$(corepath))\
      $(foreach inst,$(lastword $(subst :, ,$(corestr))),$(infox CoreInst:$(inst))\
        $(call QuartusSynEcho,set_instance_assignment -name PARTITION $(call QuartusPartitionName,$(inst)) -to $4$(inst) $(if $4,,-entity $(or $(Top),$(Core))),$3)\
        $(call QuartusSynEcho,set_instance_assignment -name QDB_FILE_PARTITION $(call FindRelToTgtDirSubDir,$(corepath),$5) -to $4$(inst) $(if $4,,-entity $(or $(Top),$(Core))),$3))\
      $(foreach corename,$(firstword $(subst :, ,$(corestr))),\
        $(if $(infox Filter:$(filter $(corename),$(CoreNames)))$(filter $(corename),$(CoreNames)),\
          $(call QuartusElabEchoFiles,$(corename),\
            $(foreach s,$(call HdlExtractSourcesForLib,$(HdlTarget),$(corename),$(TargetDir)),\
              $(call FindRelToTgtDirSubDir,$s,$5) ),$3),\
          $(foreach w,$(subst _rv,,$(basename $(notdir $(corepath)))),\
            $(call QuartusElabEchoFiles,$w,\
              $(foreach d,$(dir $(corepath)),\
                $(foreach l,$(if $(filter vhdl,$(HdlLanguage)),vhd,v),\
                  $(foreach f,$(call HdlExists,$d$w.$l) $(call HdlExists,$d$w-ents.$l),\
                    $(call FindRelToTgtDirSubDir,$f,$5) ))\
                $(and $(filter vhdl,$(HdlLanguage)),\
                    $(call FindRelToTgtDirSubDir,$(strip \
                      $(or $(call HdlExists,$d/generics.vhd),\
                           $(call HdlExists,$d/$(basename $(notdir $(corepath)))-generics.vhd))),$5))),$3))))))

QuartusIncludeSources=$(infox =====VHDL:$(VHDLLibName))$(infox IncSrcs:$1:$2)\
  $(call QuartusElabEchoFiles,$2,$1,$3)

QuartusIncludeIncludedirs=\
  $(call QuartusElabEcho,'\# Search paths for $2 local files',$3)  \
  $(foreach d,$1, \
    $(call QuartusElabEcho,set_global_assignment -name SEARCH_PATH '\"'$(strip \
     $(call FindRelative,$(TargetDir),$d))'\"',$3) ) \

# Make the settings file
# Note that the local source files use notdir names and search paths while the
# remote libraries use pathnames so that you can have files with the same names.
# FIXME: use of "sed" below is slow - perhaps use .cN rather than _cN?
#
# Generate the QSF file for elaboration or synthesis of the current asset.
# Elaboration and synthesis require different QSF settings (elab omits QDBs/partitions).
# $(Core).qsf is the elab/synth QSFs.
QuartusMakeQsf=\
 if test -f $(Core).qsf; then cp $(Core).qsf $(Core).qsf.bak; fi; \
 $(and $(findstring $(HdlMode),library),\
   $(X Fake top module so we can compile libraries anyway for syntax errors) \
   echo 'module onewire(input  W_IN, output W_OUT); assign W_OUT = W_IN; endmodule' > onewire.v;) \
 $(X From here we generate qsf file contents, e.g. "settings") \
 source $$OCPI_CDK_DIR/include/hdl/quartus_qsf_gen.sh; \
 $(if $(filter $1,syn),\
   (\
     echo '\#' Synthesis specific assignments for core. ;\
     $(call QuartusIncludeDependencies,$1)\
   ) >> $(Core).qsf; ,\
  (\
    echo '\#' Common assignments whether a library or a core; \
    $(call QuartusMakeDevices,$(HdlTarget)) \
    echo set_global_assignment -name TOP_LEVEL_ENTITY $(or $(Top),$(Core)); \
    \
    $(if $(findstring $(HdlMode),library),\
      echo '\#' Assignments for building libraries; \
      echo set_global_assignment -name SYNTHESIS_EFFORT fast;) \
    $(if $(findstring $(HdlMode),container),\
      echo '\#' Include the platform-related assignments. ;)\
    $(call QuartusIncludeDependencies,$1)\
    ) > $(Core).qsf; \
     $(if $(findstring $(HdlMode),platform config container),\
       ( \
        echo '\#' Include constraints file from Platform Worker; \
        echo source $(call AdjustRelative,$(call QuartusConstraints,$(HdlPlatform))); \
       ) >> $(Core).qsf; ))\

QuartusSynOptions=--read_settings_files=on --write_settings_files=off -c $(Core) --parallel

QuartusExportOptions=$(CoreOrLibName) --export_partition root_partition --snapshot synthesized --file $(Core).qdb

# Perform the actual synthesis of a Library or Core (core, worker, pfconfig, assembly, container)
# For libraries, just run elaboration
# For cores:
#   Create the elab QSF, omitting QDB imports
#   Run elaboration
#   Archive the elab project for later observation
#   Create the synth QSF, including QDB imports and partition assignments
#   Run synthesis
#   Export the synthesized results as a QDB partition file
HdlToolCompile=\
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/quartus-$(Core).out.;\
  rm -r -f db incremental_db $(Core).qxp $(Core).*.rpt $(Core).*.summary $(Core).qpf $(Core).qsf $(Core)-elab.qsf $(notdir $@); \
  $(call QuartusMakeQsf) cat -n $(Core).qsf; cp $(Core).qsf $(Core)-elab.qsf; \
  set -e; $(call OcpiDbgVar,HdlMode,xc) \
  $(call DoAltera,quartus_ipgenerate,$(CoreOrLibName) -c $(CoreOrLibName) --synthesis=vhdl,$(CoreOrLibName),ipgen,pro); \
  $(if $(findstring $(HdlMode),core worker platform assembly config container),\
    $(call DoAltera,quartus_syn,$(QuartusSynOptions) --analysis_and_elaboration $(CoreOrLibName),$(CoreOrLibName),elab,pro); \
    $(if $(QuartusArchiveElaborations),\
      echo '  'Archiving pre-partition elaboration results in $(CoreOrLibName)-elab.qar; \
      $(call QuartusArchiveProject,$(CoreOrLibName),elab); \
      echo '  'Restoring elaboration QSF - archiving forcibly overwrites qsf....; \
      cp $(Core)-elab.qsf $(Core).qsf; )\
    echo '  'Updating QSF settings with partition assignments and proceeding with synthesis; \
    rm -f $(Core)-syn.qsf; $(call QuartusMakeQsf,syn)  cat -n $(Core).qsf; cp $(Core).qsf $(Core)-syn.qsf; \
    $(call DoAltera,quartus_syn,$(QuartusSynOptions) $(CoreOrLibName),$(CoreOrLibName),syn,pro); \
    echo '  'Exporting synthesized snapshot of the root_partition; \
    echo '  'Running: quartus_cdb $(QuartusExportOptions); \
    $(call DoAltera,quartus_cdb,$(QuartusExportOptions),$(Core),export,pro); )\
  $(if $(findstring $(HdlMode),library),\
    $(call DoAltera,quartus_syn,$(QuartusSynOptions) --analysis_and_elaboration $(Core),$(Core),elab,pro);) \
  $(if $(findstring $(HdlMode),container),\
    touch .$(Core)-container-syn.done;) \

# List of files to be copied out for later reference when the current asset is imported as pre-compiled
HdlToolFiles=$(call OcpiUniqueNotDir,\
  $(foreach f,$(if $(filter $(HdlMode),core),$(wildcard $(CoreBlackBoxFiles)),$(QuartusSources)),\
    $(call FindRelative,$(TargetDir),$(dir $f))/$(notdir $f)))

# To create the "library result", we create a directory full of source files
# that have the quartus library directive inserted as the first line to
# force them into the correct library when they are "discovered" via SEARCH_PATH.
ifneq ($(filter $(HdlMode),library core),)
HdlToolPost=\
    if ! test -d $(LibName); then \
      mkdir $(LibName); \
    else \
      rm -f $(LibName)/*; \
    fi;\
    for s in $(HdlToolFiles); do \
      ln -s ../$$s $(LibName); \
    done;
endif

###################################################################################################
# IMPLEMENTATION STAGES
###################################################################################################

#TODO/FIXME: should this be rbf instead? if so, should we generate it directly instead of converting?
BitFile_quartus_pro=$1.sof

# Unset means use up to 12 cores based on system
#QuartusFitOptions= --parallel=4
QuartusFitOtherOptions= --read_settings_files=on --write_settings_files=off

QuartusArchiveProject=\
  $(call DoAltera,quartus_sh --archive -auto_common_dir -include_output -output $1-$2.qar $1,,$1,$2-qar,pro,no64)

QuartusExportDesign=\
  $(call DoAltera1,quartus_cdb $1 --export_design --snapshot $2 --file $1.qdb,,$1,$3-export-design,pro,no64)

###################################################################################################
# Notes on Implementation Stages
#
# This is the standard, Non-PR flow for generic OpenCPI compilation:
#   Reuse same QSF used during Container synthesis
#   Separate Quartus Fit into the different fit stages (plan, place, route, finalize)
#     Using separate make-rules will allow for 'resume' functionality, but does slow down
#       compilation a bit. Since each stage of fit does not result in a true artifact, but
#       instead just overwrites the already existing QDB file used since synthesis, we
#       create the -<stage>.done files for each to record stage-completion time-stamps.
#   Separate make-rules for the sta/timing and asm/bitstream stages
#     Ensure sure both SOF and RBF bitstreams are generated
#
#   Note regarding .$3-container-syn.updated and .$3-container-syn.done:
#     Since the QDB file for synthesis is updated with each "fit" stage, we want to make
#     sure that the first "fit" stage (plan), is not falsely triggered. To explain further,
#     we do not want "plan" to be rerun just because the QDB file (which is the synthesis
#     artifact) has been updated when in reality synthesis may not have been rerun.
#     Instead, later stages of "fit" may have updated the QDB.
#
#     So, when synthesis completes, we write out the .*-container-syn.done file. The
#     .*-container-syn.updated make rule depends on this as well as the synthesis QDB.
#     The *.updated file is only updated if the .*-container-syn.done file has been
#     updated (this is checked by saving off .*-container-syn.done.last and preserving
#     the timestamp for comparison).
#
#     Now, the "plan" stage depends on this *.updated file. The *.updated file in turn
#     depends on the synthesis QDB. So, a dependency path from the bitstream all the way
#     back to the synthesis QDB is maintained, but the "plan" stage will not be rerun
#     JUST because the QDB has been updated., It will only be rerun if the *.updated
#     file has been regenerated (i.e. if synthesis has truly been rerun).

define HdlToolDoPlatform_quartus_pro

  $1/.$3-container-syn.updated: $1/$3$(HdlBin) $1/.$3-container-syn.done
	$(AT)cd $1; \
	if [ ! -f .$3-container-syn.updated ]; then \
	  touch .$3-container-syn.updated; \
	  cp -p .$3-container-syn.done .$3-container-syn.done.last; \
	elif [ "`stat -c %Y .$3-container-syn.done`" != "`stat -c %Y .$3-container-syn.done.last`" ] ; then \
	  touch .$3-container-syn.updated; \
	  cp -p .$3-container-syn.done .$3-container-syn.done.last; \
	fi; \

  $1/.$3-plan.done: $1/.$3-container-syn.updated $$(call QuartusConstraints,$5)
	$(AT)echo Running Quartus Fit - Stage \"Plan\". Assembly $2 on platform $5 using $4 "($6)".
	$(AT)echo Saving pre-fit QSF and QDB files to allow recreation of the Quartus Pro post-synthesis project.
	$(AT)cd $1 && \
	cp $3-syn.qsf $3.qsf; \
	cp $3.qdb $3-syn.qdb; \
	(echo ; echo \# Include the constraints file "(generally found in the Platform Worker directory)"; \
	 echo \# Include subdirs of platform worker in the search path as there may be exported directories of IP files that need to be found there; \
	 $(foreach inc,$(filter %/,$(wildcard $(HdlPlatformDir_$5)/*/)),\
	   echo set_global_assignment -name SEARCH_PATH \"$(call AdjustRelative,$(inc))\"; )\
	 ) >> $3.qsf; \
	$(call DoAltera1,quartus_fit --plan,$(QuartusFitOptions) $3 $(QuartusFitOtherOptions),$3,plan,pro) && \
	touch .$3-plan.done; \

  $1/.$3-place.done: $1/.$3-plan.done
	$(AT)echo Running Quartus Fit - Stage \"Place\". Assembly $2 on platform $5 using $4 "($6)".
	$(AT)cd $1 && \
	$(call DoAltera1,quartus_fit --place,$(QuartusFitOptions) $3 $(QuartusFitOtherOptions),$3,place,pro) && \
	touch .$3-place.done; \

  $1/.$3-route.done: $1/.$3-place.done
	$(AT)echo Running Quartus Fit - Stage \"Route\". Assembly $2 on platform $5 using $4 "($6)".
	$(AT)cd $1 && \
	$(call DoAltera1,quartus_fit --route,$(QuartusFitOptions) $3 $(QuartusFitOtherOptions),$3,route,pro) && \
	touch .$3-route.done; \

  $1/.$3-finalize.done: $1/.$3-route.done
	$(AT)echo Running Quartus Fit - Stage \"Finalize\". Assembly $2 on platform $5 using $4 "($6)".
	$(AT)cd $1 && \
	$(call DoAltera1,quartus_fit --finalize,$(QuartusFitOptions) $3 $(QuartusFitOtherOptions),$3,finalize,pro) && \
	$(call QuartusExportDesign,$3,final,finalize); \
	touch .$3-finalize.done; \

  $1/.$3-sta.done: $1/.$3-finalize.done
	$(AT)echo Running Quartus Timing Analysis. Assembly $2 on platform $5 using $4 "($6)".
	$(AT)cd $1 && \
	$(call DoAltera1,quartus_sta,$3,$3,sta,pro) && \
	touch .$3-sta.done

  $1/$3.sof $1/$3.rbf: $1/.$3-sta.done
	$(AT)echo Building Quartus Bit file: $$@.  Assembly $2 on platform $5 using $4 "($6)".
	$(AT)cd $1 && \
	$(call DoAltera1,quartus_asm,$(QuartusFitOptions) $3 $(QuartusFitOtherOptions),$3,asm,pro)
	$(AT)echo Creating RBF Bit file from SOF: $3.rbf.  Assembly $2 on platform $5 using $4 "($6)".
	$(AT)cd $1 && \
	$(call DoAltera1,quartus_cpf,-c $3.sof $3.rbf,$3,cpf,pro)

endef
endif
