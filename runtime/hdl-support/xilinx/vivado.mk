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

###############################################################################
# This file has the HDL tool details for vivado
# This file defines the HDL compilation (synthesis and implementation stages)
# specific to the vivado toolset.
#
# The first section of the file defines a set of tool-specific flags and
# variables. These are used in various places in the make-system when a tool
# specific parameter is asked for. For example, the extension of synthesis
# artifacts differ from tool to tool. For vivado, they generally end in .edf
# which is defined by the HdlBin variable.
#
# The second section of this file lists acceptable and unacceptable compilation
# options for the various stages of the vivado compilation flow. This is
# necessary because there are certain HDL compilation options that can break or
# interfere with the OpenCPI make-flow.
#
# Next, there are a set of functions defined for collecting source/core files
# when building workers, primitives....
#
# Those functions are called from HdlToolCompile, which actually performs the
# synthesis for any primitives, workers, platforms, assemblies.... This is done
# by executing vivado and directing it to a Tcl file with OpenCPI's synthesis
# commands.
#   Note: HdlToolCompile is called from hdl-make's HdlCompile
#
# Finally, the vivado implementation stages are defined one at a time. This
# allows the build system to resume when an a failure happens between stages
# (e.g. a license failure). These make-rules execute the vivado tool and
# perform each stage implementation via an Tcl script.
#
# Definition:
#   toolset: within this file, toolset refers to vivado
#
# Documentation:
#   Reference Vivado_Usage document for further information regarding the
#   options, functionality, and usage of this file and the vivado tool with
#   OpenCPI

include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk


################################################################################
# Name of the tool that needs to be installed to use this compilation flow
HdlToolName_vivado=Vivado
################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.
HdlToolLibraryFile=$2

HdlRecurseLibraries_vivado=yes

################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
HdlBin=$(or $(suffix $(PreBuiltCore)),.edf)
HdlBin_vivado=$(or $(suffix $(PreBuiltCore)),.edf)

HdlBinAlternatives_vivado=.dcp .ngc .xci

################################################################################
# Variable required by toolset: HdlToolRealCore
# Set if the tool can build a real "core" file when building a core
# I.e. it builds a singular binary file that can be used in upper builds.
# If not set, it implies that only a library containing the implementation is
# possible
HdlToolRealCore:=yes
HdlToolRealCore_vivado:=yes
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=
HdlToolNeedBB_vivado=
################################################################################
# Function required by toolset: $(call HdlToolCoreRef,coreref)
# Modify a stated core reference to be appropriate for the tool set
HdlToolCoreRef=$1
HdlToolCoreRef_vivado=$1

CoreOrLibName=$(or $(Core),$(LibName))
################################################################################
# Variable required by toolset: HdlToolNeedsSourceList_<tool>=yes
# Set if the tool requires a listing of source files and libraries
HdlToolNeedsSourceList_vivado=yes

################################################################################
# Here we rearrange the full hardware part number to vivado's expected
# part-package-speed
empty:=
space:=$(empty) $(empty)

VivadoRearrangePart=$(word 1,$1) $(word 3,$1) $(word 2,$1)$(if $(word 4,$1), $(word 4,$1))
HdlFullPart_vivado=$(if $1,$(subst $(space),-,$(call VivadoRearrangePart,$(subst -, ,$1))))

ifeq ($(HdlMode),library)
CoreOrLibName=$(LibName)
ifeq ($(Core),)
Core=$(LibName)
endif
else
CoreOrLibName=$(Core)
endif

$(call OcpiDbgVar,HdlMode)

VivadoBadOptions_all=\
-quiet

###############################################################################
# Options for synthesis
###############################################################################
# Options that the user may specify/override
VivadoGoodOptions_synth=\
-verbose \
-part \
-constrset \
-flatten_hierarchy \
-gated_clock_conversion \
-directive \
-bufg \
-no_lc \
-fanout_limit \
-shreg_min_size \
-fsm_extraction \
-rtl_skip_ip \
-rtl_skip_constraints \
-keep_equivalent_registers \
-resource_sharing \
-cascade_dsp \
-control_set_opt_threshold \
-max_bram \
-max_uram \
-max_dsp \
-max_bram_cascade_height \
-max_uram_cascade_height \
-retiming \
-no_srlextract \
-assert \
-no_timing_driven \
-sfcu

# Vivado parameters controlled by the user should NOT include these:
VivadoBadOptions_synth=\
-name \
-top \
-mode \
-rtl \
-include_dirs \
-generic \
-verilog_define \
$(VivadoBadOptions_all)

VivadoDefaultOptions_synth=\
-part $(HdlChoosePart) \
$(VivadoDefaultOptions_synth_$(HdlMode))


VivadoDefaultOptions_synth_core=$(VivadoDefaultOptions_synth_most)
VivadoDefaultOptions_synth_worker=$(VivadoDefaultOptions_synth_most)
VivadoDefaultOptions_synth_platform=$(VivadoDefaultOptions_synth_most)
VivadoDefaultOptions_synth_config=$(VivadoDefaultOptions_synth_most)
VivadoDefaultOptions_synth_assembly=$(VivadoDefaultOptions_synth_most)

VivadoDefaultOptions_synth_most=\
-top $(Top) \
-mode out_of_context \

VivadoDefaultOptions_synth_container=\
-top $(Top) \
-mode default

VivadoDefaultOptions_synth_library=\
-rtl \
-mode out_of_context \
-fsm_extraction off

VivadoDefaultExtraOptions_synth=
VivadoDefaultExtraOptions_opt=
VivadoDefaultExtraOptions_place=
VivadoDefaultExtraOptions_phys_opt=
VivadoDefaultExtraOptions_route=
VivadoDefaultExtraOptions_timing=
VivadoDefaultExtraOptions_bit=

VivadoMyExtraOptions_synth=$(strip \
      $(if $(filter undefined,$(origin VivadoExtraOptions_synth_$(HdlMode))),\
          $(VivadoDefaultExtraOptions_synth_$(HdlMode)),\
          $(VivadoExtraOptions_synth_$(HdlMode))) \
      $(if $(filter undefined,$(origin VivadoExtraOptions_synth)),\
          $(VivadoDefaultExtraOptions_synth),\
          $(VivadoExtraOptions_synth)))

# Use this to send options to the write_edif command.
#VivadoEdifOptions=

###############################################################################
# Options for opt stage of implementation
###############################################################################
VivadoGoodOptions_opt=\
-verbose \
-retarget \
-propconst \
-sweep \
-bram_power_opt \
-remap \
-resynth_area \
-resynth_seq_area \
-directive \
-muxf_remap \
-hier_fanout_limit \
-bufg_opt \
-shift_register_opt \
-control_set_merge \
-merge_equivalent_drivers \
-carry_remap \
-debug_log

VivadoBadOptions_opt=$(VivadoBadOptions_all)

VivadoDefaultOptions_opt=

###############################################################################
# Options for place stage of implementation
###############################################################################

# TODO: is unplace acceptable?? : Unplace all the instances which are not locked by Constraints.
VivadoGoodOptions_place=\
-verbose \
-directive \
-no_timing_driven \
-timing_summary \
-unplace \
-post_place_opt \
-fanout_opt \
-no_bufg_opt

VivadoBadOptions_place=$(VivadoBadOptions_all)

VivadoDefaultOptions_place=

###############################################################################
# Options for phys_opt stage of implementation run post-place
###############################################################################
VivadoGoodOptions_post_place_phys_opt=\
-verbose \
-fanout_opt \
-placement_opt \
-routing_opt \
-onroute_replace \
-rewire \
-critical_cell_opt \
-dsp_register_opt \
-bram_register_opt \
-uram_register_opt \
-bram_enable_opt \
-shift_register_opt \
-hold_fix \
-retime \
-force_replication_on_nets \
-directive \
-critical_pin_opt \
-clock_opt \
-path_groups

VivadoBadOptions_post_place_phys_opt=$(VivadoBadOptions_all)

VivadoDefaultOptions_post_place_phys_opt=\
-hold_fix

###############################################################################
# Options for route stage of implementation
###############################################################################
# TODO: is unplace acceptable?? : Unplace all the instances which are not locked by Constraints.
VivadoGoodOptions_route=\
-verbose \
-unroute \
-release_memory \
-nets \
-physical nets \
-pins \
-directive \
-tns_cleanup \
-no_timing_driven \
-preserve \
-delay \
-auto_delay \
-max_delay \
-min_delay \
-timing_summary \
-finalize

# TODO: was I correct to assume release_memory is not acceptable?
# release_memory "Release Router memory. Not compatible with any other options." - Xilinx UG835
VivadoBadOptions_route=\
$(VivadoBadOptions_all)

VivadoDefaultOptions_route=\
-directive NoTimingRelaxation

###############################################################################
# Options for phys_opt stage of implementation run post-route
###############################################################################
VivadoGoodOptions_post_route_phys_opt=\
-verbose \
-fanout_opt \
-placement_opt \
-routing_opt \
-onroute_replace \
-rewire \
-critical_cell_opt \
-dsp_register_opt \
-bram_register_opt \
-uram_register_opt \
-bram_enable_opt \
-shift_register_opt \
-hold_fix \
-retime \
-force_replication_on_nets \
-directive \
-critical_pin_opt \
-clock_opt \
-path_groups

VivadoBadOptions_post_route_phys_opt=$(VivadoBadOptions_all)

VivadoDefaultOptions_post_route_phys_opt=\
-hold_fix

###############################################################################
# Options for timing stage of implementation
###############################################################################
# TODO: Should we be using 'warn_on_violation' by default?
VivadoGoodOptions_timing=\
-verbose \
-from \
-rise_from \
-fall_from \
-to \
-rise_to \
-fall_to \
-through \
-rise_through \
-fall_through \
-delay_type \
-setup \
-hold \
-max_paths \
-nworst \
-unique_pins \
-path_type \
-input_pins \
-no_header \
-label_reused \
-slack_lesser_than \
-slack_greater_than \
-group \
-sort_by \
-no_report_unconstrained \
-user_ignored \
-of_objects \
-signficant_digits \
-column_style \
-append \
-warn_on_violation \

VivadoBadOptions_timing=\
-file \
-name \
-no_pr_attribute \
-return_string \
-cell \
-rpx \
$(VivadoBadOptions_all)

# -rpx 'target file' is added later
VivadoDefaultOptions_timing=\
-delay_type min_max \
-max_paths 20 \
-sort_by group \
-input_pins

###############################################################################
# Options for bit stage of implementation
###############################################################################
VivadoGoodOptions_bit=\
-verbose \
-raw_bitfile \
-mask_file \
-readback_file \
-logical_location_file \
-bin_file \
-no_binary_bitfile \
-no_partial_bitfile

VivadoBadOptions_bit=\
-force \
-cell \
$(VivadoBadOptions_all)

# target file is added later
VivadoDefaultOptions_bit=\
-force

# This allows us to define 'VivadoExtraOptions_<stage>'
VivadoMyExtraOptions_impl=$(strip \
      $(if $(filter undefined,$(origin VivadoExtraOptions_$1)),\
          $(VivadoDefaultExtraOptions_$1),\
          $(VivadoExtraOptions_$1)))

VivadoMyExtraOptions=$(if $(filter synth,$1),$(VivadoMyExtraOptions_synth),$(call VivadoMyExtraOptions_impl,$1))

# check the first option in the list and recurse
VivadoCheckOption=\
$(if $(filter -%,$(word 1,$(1))),\
    $(if $(filter $(VivadoGoodOptions_$2),$(word 1,$(1))),\
        $(if $(word 2,$(1)),\
            $(call VivadoCheckOption,$(wordlist 3,$(words $(1)),$(1)),$2)),\
        $(error Vivado option "$(word 1,$(1))" unknown)),\
   $(and $(1),$(warning Expected hyphen in Vivado option "$(1)". If this is an argument to the previous option, you can ignore this warning.)))

# Function to check whether options are bad, good, or unknown
VivadoCheckOptions=\
    $(foreach option,$(VivadoBadOptions_$2),\
        $(if $(findstring $(option),$1),\
            $(error Vivado option "$(option)" not allowed here)))\
    $(call VivadoCheckOption,$1,$2)

VivadoPruneOption=\
    $(if $(filter $(word 1,$(1)),$(call VivadoMyExtraOptions,$2)),,$(wordlist 1,2,$(1))) \
    $(if $(word 3,$(1)),\
        $(call VivadoPruneOption,$(wordlist 3,$(words $(1)),$(1)),$2))

# Check our extra options, prune our options and concatenate our defaults, extras, and internals
VivadoOptions=$(strip \
$(call VivadoCheckOptions,$(call VivadoMyExtraOptions,$1),$1)\
$(call VivadoPruneOption,$(VivadoDefaultOptions_$1),$1) $(call VivadoMyExtraOptions,$1) $(VivadoInternalOptions_$1))

VivadoPrimitiveCores=$(foreach c,$(Cores),$(if $(findstring /,$c),,$c))

###############################################################################
# VivadoIncludeDependencies
# Collect the libraries that this asset depends on
#   Extract the sources for each and include them using their paths relative to their library
# Call the tool specific function for including this asset's cores
# Call the tool specific function for including this asset's sources
# Call the tool specific function for including 'include directories' (ie for verilog includes)
VivadoIncludeDependencies=\
  $(foreach l,$(call HdlCollectLibraries,$(HdlTarget)),$(infox IncLib:$l)\
    $(call VivadoIncludeSources,\
      $(call HdlExtractSourcesForLib,$(HdlTarget),$l,$(TargetDir)),\
      $(notdir $(call HdlRmRv,$l))))\
  $(call VivadoIncludeCores,$(SubCores_$(HdlTarget)),$(CoreOrLibName))\
  $(call VivadoIncludeSources,$(foreach s,$(HdlSources),$(call FindRelative,$(TargetDir),$s)),$(CoreOrLibName))\
  $(call VivadoIncludeIncludedirs,$(call Unique,$(patsubst %/,%,$(dir $(HdlSources)) $(VerilogIncludeDirs))),$(CoreOrLibName))\

###############################################################################
# Here, the tool-specific functions are defined for including cores, sources
# and includedirs

# Vivado's function for including cores
# This is mostly adapted from the Quartus method.
# We check to see if the core is in 'Cores' before trying to add the
# worker source files (e.g. those underneath gen). The cores listed
# in 'Cores' are either primitive cores or worker-included cores.
VivadoIncludeCores=\
  $(foreach c,$1,\
    echo read_edif_or_dcp $(call FindRelative,$(TargetDir),$(call HdlCoreRefMaybeTargetSpecificFile,$c,$(HdlTarget))) >> $(CoreOrLibName)-imports.tcl;\
    $(if $(filter $c,$(Cores)),\
        echo add_files_set_lib $c '\"'$(call HdlExtractSourcesForLib,$(HdlTarget),$c,$(TargetDir))'\"' >> $(CoreOrLibName)-imports.tcl;,\
      $(foreach w,$(subst _rv,,$(basename $(notdir $c))),\
        $(foreach d,$(dir $c),\
          $(foreach l,$(if $(filter vhdl,$(HdlLanguage)),vhd,v),\
            $(foreach f,$(or $(xxcall HdlExists,$d../gen/$w-defs.$l),\
                             $(call HdlExists,$d$w.$l)),\
              echo read_$(if $(filter vhdl,$(HdlLanguage)),vhdl -library $w,verilog) $(call FindRelative,$(TargetDir),$f) >> $(CoreOrLibName)-imports.tcl;\
              $(and $(filter vhdl,$(HdlLanguage)),\
                $(foreach g,$(or $(call HdlExists,$d/generics.vhd),\
                                 $(call HdlExists,$d/$(basename $(notdir $c))-generics.vhd)),\
                   echo read_vhdl -library $w $(call FindRelative,$(TargetDir),$g) >> $(CoreOrLibName)-imports.tcl;))))))))

# Vivado's function for including sources
VivadoIncludeSources=$(infox IncSrcs:$1:$2)\
      echo puts '\"'Assignments for $2 local source files'\"' >> $(CoreOrLibName)-imports.tcl; \
      echo add_files_set_lib $2 '\"'$1'\"' >> $(CoreOrLibName)-imports.tcl;

# Vivado's function for including include dirs
VivadoIncludeIncludedirs=\
  echo puts '\"'Search paths for $2 local files'\"' >> $(CoreOrLibName)-imports.tcl; \
  echo set_property include_dirs '\"'[get_property include_dirs [current_fileset]] \
  $(foreach d,$1, \
    $(strip $(if $(filter /%,$d),$d,$(call FindRelative,$(TargetDir),$d))) )'\"' [current_fileset] >> $(CoreOrLibName)-imports.tcl; \
  echo puts '\"'[get_property include_dirs [current_fileset]]'\"' >> $(CoreOrLibName)-imports.tcl;

###############################################################################

# Here we actually run the synthesis step. This is executed for primitive cores/libraries, workers, platforms, configs, assemblies, containers
#   We call the vivado-synth.tcl script to actually perform synthesis, and we pass it information via tclargs
HdlToolCompile=\
  $(foreach l,$(HdlLibrariesInternal),\
     $(if $(wildcard $(call HdlLibraryRefDir,$(l),$(HdlTarget),,X5)),,\
          $(error Error: Specified library: "$l", in the "HdlLibraries" variable, was not found for $(call HdlGetFamily,$(HdlTarget)).))) \
  $(foreach l,$(SubCores_$(HdlTarget)),$(infox AC:$l)\
    $(if $(foreach x,$(call FindRelative,.,$(if $(findstring /,$l),$l,$(call HdlCoreRef,$l,$(HdlTarget)))),$(call HdlExists,$x)),,\
	$(info Error: Specified core library for "$l", in the "Cores" variable, was not found.) \
        $(error Error:   after looking in $(call HdlExists,$(call FindRelative,.,$(call HdlCoreRef,$c,$(HdlTarget))))))) \
  $(infox ALLCORE1) \
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/vivado-$(Core).out.;\
  if test -f vivado.jou ; then mv -f vivado.jou vivado.jou.bak ; fi;\
  echo Doing Vivado compile ;\
  rm -f $(CoreOrLibName)-imports.tcl;\
  $(VivadoIncludeDependencies)\
  $(call OcpiXilinxVivadoInit); \
  $(if $(and $(HdlNoElaboration),$(filter $(HdlMode),library)),,vivado -mode batch -source $(OCPI_CDK_DIR)/include/hdl/vivado-synth.tcl -nolog -journal vivado.jou -tclargs \
    tcl_imports=$(CoreOrLibName)-imports.tcl \
    artifact='$(notdir $@)' \
    hdl_mode='$(HdlMode)' \
    top_mod='$(Top)' \
    synth_part='$(HdlChoosePart)' \
    synth_opts='$(call VivadoOptions,synth)' \
    edif_opts='$(VivadoEdifOptions)';)

VivadoToolExports=\
  $(foreach f,$(if $(filter $(HdlMode),core),$(wildcard $(CoreBlackBoxFiles)),$(HdlSources)),\
     $(call FindRelative,$(TargetDir)/$(CoreOrLibName),$(dir $f))/$(notdir $f))

# To create the "library result", we create a directory full of source files
# that have the vivado library directive inserted as the first line to
# force them into the correct library when they are "discovered" via SEARCH_PATH.
ifneq ($(filter $(HdlMode),library core),)
HdlToolPost=$(infox HS:$(HdlSources):VTE:$(VivadoToolExports))\
  if ! test -d $(CoreOrLibName); then \
    mkdir $(CoreOrLibName); \
  else \
    rm -f $(CoreOrLibName)/*; \
  fi;\
  for s in $(VivadoToolExports); do \
    ln -s $$s $(CoreOrLibName); \
  done;
endif

#        rm -f $(CoreOrLibName)/$(notdir $$s);
# HdlGrepExclude specifies a string that will be passed to grep when determining
# which strings containing 'error' and 'warning' do NOT indicate failure.
HdlGrepExclude_vivado:=-e '^CRITICAL WARNING:' -e '|      |Item              |Errors |Warnings |Status |Description       |' -e '0 Errors encountered'

################################################################################
# These functions choose output/log files, call the actual implementation
# steps with arguments. Exit status/errors are grepped for, and the run is
# timed.
# Flow:
#   1. Make-rule calls DoXilinx
#   2. DoXilinx runs implementation step
#   3. DoXilinx calls XilinxAfter
#   4. XilinxAfter checks the exit status, reports time
#   5. XilinxAfter exits with 0 if the implementation step was successful
################################################################################
# Echo error or success based on the exit status
# XilinxAfter: <target-dir> <stage> <exit-status>
XilinxAfter=set +e; grep -e "ERROR:" $1.out; \
        echo Time: `cat $1.time` at `date +%T`; \
        if test $2 != 0; then \
          $(ECHO) Error: $(HdlToolSet) failed\($2\) on stage "$1". See $1.out.'  '; \
	  exit 1;\
        else \
          $(ECHO) ' Tool "$(HdlToolSet)" for target "$(HdlTarget)" succeeded on stage "$1".  '; \
	  exit 0;\
        fi; \

# Use default pattern to find error string in tool output
# DoXilinx: <command> <target-dir> <arguments> <impl-stage>
# time the implementation stage that is run with vivado.
# Pass the specified arguments to vivado tcl, and capture
# the exit status (pass it to XilinxAfter)
DoXilinx=\
        if test -f $(TargetDir)/vivado-$4.jou ; then \
          mv -f $(TargetDir)/vivado-$4.jou $(TargetDir)/vivado-$4.jou.bak ; \
        fi; \
	echo " "Details in $4.out; cd $2 ; $(call OcpiXilinxVivadoInit,$4.out);\
	echo Command: $1 $3 >> $4.out; \
	set +e;\
	(/usr/bin/time -f %E -o $4.time bash -c "$1 $3") >> $4.out 2>&1;\
        HdlExit=$$$$?;\
	set -e;\
	(echo -n Elapsed time:; tr -d '\n' <$4.time; echo -n ', completed at '; date +%T; echo -n Exit status: $$$$HdlExit) >> $4.out; \
	$(call XilinxAfter,$4,$$$$HdlExit)

###############################################################################
# The contents below this point are for final implementation steps
# of the assembly bitstream.
###############################################################################
BitBase=$1/$2$(and $(filter-out 0,$3),_$3)
BitName=$(call BitBase,$1,$2,$3).bit
BifName=$(call BitBase,$1,$2,$3).bif
BinName=$(call BitBase,$1,$2,$3).bin

# Some devices require that a *.bin be generated after the bitstream because .bit files are not directly loadable
BinOrBitBitstreamExt=$(if $(filter $(HdlTarget),zynq_ultra),bin,bit)
# This file will be used to create the compressed bitstream with metadata that is findable by ocpirun's searching
# This is either a bit or bin file depending on the device family. If this is a bin file, then the last implementation
# rule is used (BinName which depends on the bitfile), otherwise the bitfile is the last thing generated for implementation.
BitFile_vivado=$1.$(BinOrBitBitstreamExt)

# Bootgen requires that the FPGA/PS Architecture be specified. This is either zynq, zynqmp (zynq_ultra) or fpga.
# This is only used for zynq_ultra today, so 'zynq' and 'fpga' should never be chosen.
BootgenArch=$(if $(filter zynq,$(HdlTarget)),zynq,$(if $(filter zynq_ultra,$(HdlTarget)),zynqmp,fpga))

SynthName=$1/$2.edf
OptName=$1/$2-opt.dcp
PlaceName=$1/$2-place.dcp
RouteName=$1/$2-route.dcp
TimingName=$1/$2-timing.rpx

# This variable enables the searching for previous place/route results
# for incremental compilation. This can speed up rebuild times after
# small changes. Default is false.
#VivadoIncrementalCompilation=true

# This variable enables the power_opt_design stage of implementation.
# By default, power_opt_design is not run.
#VivadoPowerOpt=true

# This variable enables the phys_opt_design stage of implementation
# to be run after place_design but before route_design
# By default, phys_opt_design is not run.
#VivadoPostPlaceOpt=true

# This variable enables the phys_opt_design stage of implementation
# to be run after route_design
# By default, phys_opt_design is not run.
#VivadoPostRouteOpt=true

# Here we are creating the command that will run the vivado script ($1) and pass this command and its arguments to DoXilinx
# This is essentially a wrapper function for DoXilinx where we create the long vivado command ($1)
#
# We forward args 2-4 to DoXilinx.
# $3 is a list of args to pass to the vivado tcl script in format 'varName1=value1 ...'
#   e.g. implementation stage, current target, previous checkpoint, part, constraints,
# The implementation (last argument) is used when naming '.out' files and when priting
# results
DoVivado=$(call DoXilinx,vivado -mode batch -source $(OCPI_CDK_DIR)/include/hdl/$1 -nolog -journal vivado-$4.jou,$2,$3,$4)

# The constraint file(s) to use, first/only arg is platform
HdlConstraintsSuffix_vivado=.xdc
VivadoConstraints_default=$(HdlPlatformDir_$1)/$1$(HdlConstraintsSuffix_vivado)
VivadoConstraints=$(or $(HdlConstraints),$(VivadoConstraints_default))

# Bin file is generated for certain devices (e.g. zynq_ultra) after the bitstream due to bitstream-loading requirements

define HdlToolDoPlatform_vivado

$(call OptName,$1,$3): $(call SynthName,$1,$3) $(call VivadoConstraints,$5)
	$(AT)echo -n For $2 on $5 using config $4: creating optimized DCP file using '"opt_design"'.
	$(AT)$(call DoVivado,vivado-impl.tcl,$1,-tclargs \
		stage=opt \
		target_file=$(notdir $(call OptName,$1,$3)) \
		part=$(HdlChoosePart) \
		edif_file=$(notdir $(call SynthName,$1,$3)) \
		constraints='$(foreach u,$(call VivadoConstraints,$5),$(call AdjustRelative,$u))' \
		impl_opts='$(call VivadoOptions,opt)' \
		power_opt=$(if $(VivadoPowerOpt),true,false) \
		,opt)

$(call PlaceName,$1,$3): $(call OptName,$1,$3)
	$(AT)echo -n For $2 on $5 using config $4: creating placed DCP file using '"place_design"'.
	$(AT)$(call DoVivado,vivado-impl.tcl,$1,-tclargs \
		stage=place \
		target_file=$(notdir $(call PlaceName,$1,$3)) \
		checkpoint=$(notdir $(call OptName,$1,$3)) \
		part=$(HdlChoosePart) \
		impl_opts='$(call VivadoOptions,place)' \
		post_place_opt=$(if $(VivadoPostPlaceOpt),true,false) \
		phys_opt_opts='$(call VivadoOptions,post_place_phys_opt)' \
		incr_comp=$(if $(VivadoIncrementalCompilation),true,false) \
		,place)

$(call RouteName,$1,$3): $(call PlaceName,$1,$3)
	$(AT)echo -n For $2 on $5 using config $4: creating routed DCP file using '"route_design"'.
	$(AT)$(call DoVivado,vivado-impl.tcl,$1,-tclargs \
		stage=route \
		target_file=$(notdir $(call RouteName,$1,$3)) \
		checkpoint=$(notdir $(call PlaceName,$1,$3)) \
		part=$(HdlChoosePart) \
		impl_opts='$(call VivadoOptions,route)' \
		post_route_opt=$(if $(VivadoPostRouteOpt),true,false) \
		phys_opt_opts='$(call VivadoOptions,post_route_phys_opt)' \
		incr_comp=$(if $(VivadoIncrementalCompilation),true,false) \
		,route)

$(call TimingName,$1,$3): $(call RouteName,$1,$3)
	$(AT)echo -n Generating timing report '(RPX)' for $2 on $5 using $4 using '"report_timing"'.
	$(AT)$(call DoVivado,vivado-impl.tcl,$1,-tclargs \
		stage=timing \
		target_file=$(notdir $(call TimingName,$1,$3)) \
		checkpoint=$(notdir $(call RouteName,$1,$3)) \
		part=$(HdlChoosePart) \
                impl_opts='$(call VivadoOptions,timing)' \
		,timing)

$(call BitName,$1,$3,$6): $(call RouteName,$1,$3) $(call TimingName,$1,$3) $(wildcard $(HdlPlatformDir_$5)/*.xdc)
	$(AT)echo -n For $2 on $5 using config $4: Generating Xilinx Vivado bitstream file $$@.
	$(AT)$(call DoVivado,vivado-impl.tcl,$1,-tclargs \
		stage=bit \
		target_file=$(notdir $(call BitName,$1,$3,$6)) \
		checkpoint=$(notdir $(call RouteName,$1,$3)) \
		part=$(HdlChoosePart) \
		constraints='$(wildcard $(HdlPlatformDir_$5)/*_bit.xdc)' \
                impl_opts='$(call VivadoOptions,bit)' \
		,bit)

$(call BinName,$1,$3,$6): $(call BitName,$1,$3)
	$(AT)echo -n For $2 on $5 using config $4: Generating Xilinx Vivado bitstream file $$@ with BIN extension using "bootgen".
	$(AT)echo all: > $$(call BifName,$1,$3,$6); \
	     echo "{" >> $$(call BifName,$1,$3,$6); \
	     echo "       [destination_device = pl] $(notdir $(call BitName,$1,$3,$6))" >> $$(call BifName,$1,$3,$6); \
	     echo "}" >> $$(call BifName,$1,$3,$6);
	$(AT)$(call DoXilinx,bootgen,$1,-image $(notdir $(call BifName,$1,$3,$6)) -arch $(BootgenArch) -o $(notdir $(call BinName,$1,$3,$6)) -w,bin)

endef
