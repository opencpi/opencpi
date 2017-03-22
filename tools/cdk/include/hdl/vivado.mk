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

# This file has the HDL tool details for xst

include $(OCPI_CDK_DIR)/include/hdl/xilinx.mk


################################################################################
# $(call HdlToolLibraryFile,target,libname)
# Function required by toolset: return the file to use as the file that gets
# built when the library is built.

HdlLibSuffix=xpr

#HdlToolLibraryFile=$2/$2.dcp
HdlToolLibraryFile=$2/$2.xpr

# This was correct BEFORE we forced using the new parser in xst
#$(strip \
#  $2/$(if $(filter virtex6 spartan6 zynq,$(call HdlGetFamily,$1)),$2.$(HdlLibSuffix),hdllib.ref))

################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=$(call Unique,$(foreach t,$(1),$(call HdlGetFamilies,$(t))))
################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
#HdlBin=.dcp
#HdlBin_vivado=.dcp
HdlBin=.edf
HdlBin_vivado=.edf
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
#HdlToolNeedBB:=yes
#HdlToolNeedBB_vivado:=yes
################################################################################
# Function required by toolset: $(call HdlToolCoreRef,coreref)
# Modify a stated core reference to be appropriate for the tool set
HdlToolCoreRef=$1
HdlToolCoreRef_vivado=$1

################################################################################
# This is the name after library name in a path
# It might adjust (genericize?) the target
VivadoLibRef=$(or $3,$(call HdlGetFamily,$2))

empty:=
space:=$(empty) $(empty)

VivadoMakePart1=$(firstword $1)$(word 3,$1) $(word 2,$1)
VivadoMakePart=$(subst $(space),-,$(call VivadoMakePart1,$(subst -, ,$1)))
################################################################################
# $(call XstLibraryFileTarget2(target,libname)
# Return the actual file to depend on when it is built
# TODO: What is this doing??
#VivadoLibraryFileTarget=$(if $(filter virtex6 spartan6 zynq,$(call HdlGetFamily,$(infox x3:$1)$(1))),$(2).sdbl,hdllib.ref)
#VivadoLibraryCleanTargets=$(strip \
  $(if $(filter virtex6 spartan6 zynq,$(call HdlFamily,$(1))),*.sdb?,hdllib.ref vlg??))

# When making a library, xst still wants a "top" since we can't precompile 
# separately from synthesis (e.g. it can't do what vlogcomp can with isim)
# Need to be conditional on libraries
ifeq ($(HdlMode),library)
ifeq ($(Core),)
Core=onewire
Top=onewire
endif
endif
#################################################################################
# NOTE:  XST only makes the verilog primitive libraries available if there are verilog sources
# in the project.  This means that, during elaboration, it will fail to elaborate lower level
# verilog modules in libraries if there are no verilog sources in the project file.
# Sooo, we ALWAYS stick the "onewire" verilog in all projects.
# If we are building a library, this serves as the "top" of the library, otherwise
# it is simply stuck in to the compilation to cause XST to make the verilog primitives available
CompiledSourceFiles+= $(OCPI_CDK_DIR)/include/hdl/onewire.v 
WorkLibrarySources+= $(OCPI_CDK_DIR)/include/hdl/onewire.v

### What internal options for Vivado?
#VivadoInternalOptions=$(wildcard $(TargetDir)/$(HdlPlatform).xdc)
#XstDefaultExtraOptions=
####XstDefaultContainerExtraOptions=-iobuf yes -bufg 32 -iob auto
#XstDefaultContainerExtraOptions=-bufg 32 -iob auto
#XstMyExtraOptions=$(strip \
  $(if $(filter $(HdlMode),container),\
      $(if $(filter undefined,$(origin XstContainerExtraOptions)),\
          $(XstDefaultContainerExtraOptions),\
          $(XstContainerExtraOptions)),\
      $(if $(filter undefined,$(origin XstExtraOptions)),\
          $(XstDefaultExtraOptions),\
          $(XstExtraOptions))))
$(call OcpiDbgVar,Core)
$(call OcpiDbgVar,HdlMode)

# TODO: tempdir,xsthdpdir?, -p family vs part?, sim
# XST parameters controlled by the user should NOT include these:
# ifn:       because we generate the project file
# ofn:       because the output file is according to our conventions
# top:       because the top level module is named in metadata
# xsthdpini: because the library dependencies are in set in the Libraries variable
# p:         because targets are in the Targets variable for the whole library
#            or overriden in the hdl implementation directory (target device)
# ifmt:      because we generate the project file
# iobuf:     because this is a component built, not a chip build
# sd:        how is this different from libraries?
# this may be conditionalized based on xst version.  This is baselined on 11.3.

# target must already include the canonical xst target-device, which has hyphens...
# MAYBE USE PLUS SIGN AS TARGET SEPARATOR
# XST stuff
# From IncludeDirs
# -ofn, not -o
# -vlgincdir {a c b}
# -vlgpath to search for modules

# there is no such thing
#OBJ:=.xxx
# Options that the user should not specify
#XstBadOptions=\
    -ifn -ofn -top -xsthdpini -ifmt -sd -vlgincdir -vlgpath -lso
# Options that the user may specify/override
#XstGoodOptions=-bufg -opt_mode -opt_level -read_cores -iobuf \
-p \
-power \
-iuc \
-opt_mode Speed \
-opt_level \
-keep_hierarchy \
-netlist_hierarchy \
-rtlview \
-glob_opt \
-write_timing_constraints \
-cross_clock_analysis \
-hierarchy_separator \
-bus_delimiter \
-case \
-slice_utilization_ratio \
-bram_utilization_ratio \
-dsp_utilization_ratio \
-lc \
-reduce_control_sets \
-fsm_extract \
-fsm_encoding \
-safe_implementation \
-fsm_style \
-ram_extract \
-ram_style \
-rom_extract \
-shreg_extract \
-rom_style  \
-auto_bram_packing \
-resource_sharing \
-async_to_sync \
-shreg_min_size \
-use_dsp48 \
-max_fanout \
-register_duplication \
-register_balancing \
-optimize_primitives \
-use_clock_enable \
-use_sync_set \
-use_sync_reset \
-iob \
-equivalent_register_removal \
-slice_utilization_ratio_maxmargin

VivadoCores=$(call HdlCollectCores,$(HdlTarget),VivadoCores)

VivadoLibraries=$(HdlLibrariesInternal)

# Choices as to where a bb might be
# 1. For pointing to a built target directory with the filename being the core name,
#    where the core, and bblib is built.  Core is $1.$(HdlBin) Lib is $(dir $1)/bb/$(notdir $1)
VivadoCoreLibraryChoices=$(infox XCLT:$1:$2)$(strip \
  $(and $(filter target-%,$(subst /, ,$1)),$(dir $1)/bb/$(notdir $1)) \
  $(and $(filter target-%,$(subst /, ,$1)),$(dir $1)/bb/$(patsubst %_rv,%,$(notdir $1))) \
  $(call HdlLibraryRefDir,$1_bb,$(if $(HdlTarget),$(call HdlGetFamily,$(HdlTarget)),NONE3),x,X1) \
  $(call HdlLibraryRefDir,$1,$(or $(HdlTarget),$(info NONE4)),x,X2) \
  $(call HdlLibraryRefDir,$1_rv,$(or $(HdlTarget),$(info NONE4)),x,X3) \
)

#   $(if $(PreBuiltCore)$(filter container assembly platform worker core,$(HdlMode)),,echo work;) $(if $(findstring work,$(LibName)),,echo $(LibName);) \
#####################################################
# The list of libraries is ordered:
# 1. Component libraries
# 2. Primitive libraries
# 3. Black box core libraries
# This must be consistent with the XstMakeIni
# The trick is to filter out component library BB libs from the cores.
VivadoCompLibs=$(ComponentLibraries) $(DeviceLibraries) $(CDKComponentLibraries) $(CDKDeviceLibraries)
# Remove the RV in the middle or at the end.
# This will get fixed when we put the RV at the end everywhere...
VivadoLibFromCore=$(foreach n,$(patsubst %_rv,%,$(basename $(notdir $1))),\
                 $(if $(findstring _rv_c,$n),$(subst _rv_c,_c,$n),$n))
VivadoPathFromCore=$(foreach n,$(call VivadoLibFromCore,$1),$(and $(findstring /,$1),$(dir $1))$n)

VivadoPathsFromCores=$(foreach l, $(VivadoCores),$(call VivadoPathFromCore, $l))
VivadoLibsFromCores=$(foreach l, $(VivadoCores),$(call VivadoLibFromCore, $l))

#VivadoProcessCores=\
  $(foreach l,$(VivadoCores), $(infox VivadoCore:$l)\
    $(call VivadoLibFromCore,$l)=$(call FindRelative,$(TargetDir),$(strip\
      $(firstword $(foreach c,$(call VivadoCoreLibraryChoices,$(call VivadoPathFromCore,$l),a),$(infox CECEL:$c)$(call HdlExists,$c))))));

VhdlSources    = ${strip ${filter %vhd, $(HdlSources)}}
VerilogSources = ${strip ${filter-out %vhd, $(HdlSources)}}

VivadoPartNumber=$(HdlTargets_$(HdlTarget))

VivadoMergeOptions=$(infox XNO with VivadoCores:$(VivadoCores))\
     $(foreach c,$(VivadoCores),$(infox XNO:$c)-sd $(call FindRelative,$(TargetDir),$(dir $(call HdlCoreRef,$c,$(HdlTarget)))))

VivadoCollectCoresTcl=\
   $(foreach c,$(SubCores_$(HdlTarget)),\
    read_edif $(call FindRelative,$(TargetDir)/..,$(call HdlCoreRef,$c,$(HdlTarget))) ;\
    $(foreach w,$(subst _rv,,$(basename $(notdir $c))),\
      $(foreach d,$(dir $c),\
        $(foreach l,$(if $(filter vhdl,$(HdlLanguage)),vhd,v),\
          $(foreach f,$(or $(xxcall HdlExists,$d../gen/$w-defs.$l),\
                           $(call HdlExists,$d$w.$l)),\
            read_$(if $(filter vhdl,$(HdlLanguage)),vhdl,verilog) -library $w $(call FindRelative,$(TargetDir/..),$f);\
            $(and $(filter vhdl,$(HdlLanguage)),\
              $(foreach g,$(or $(call HdlExists,$d/generics.vhd),\
                               $(call HdlExists,$d/$(basename $(notdir $c))-generics.vhd)),\
                 read_vhdl -library $w $(call FindRelative,$(TargetDir)/..,$g);)))))))
 
# Here we actually run the synthesis step. This is executed for primitive cores/libraries, workers, platforms, configs, assemblies, containers
#   We call the vivado.tcl script to actually perform synthesis, and we pass it information via tclargs
HdlToolCompile=\
  $(foreach l,$(VivadoLibraries),\
     $(if $(wildcard $(call HdlLibraryRefDir,$(l),$(HdlTarget),,X5)),,\
          $(error Error: Specified library: "$l", in the "HdlLibraries" variable, was not found for $(call HdlGetFamily,$(HdlTarget)).))) \
  $(foreach l,$(VivadoCores),$(infox AC:$l)\
    $(if $(foreach x,$(call VivadoCoreLibraryChoices,$l,b),$(call HdlExists,$x)),,\
	$(info Error: Specified core library for "$l", in the "Cores" variable, was not found.) \
        $(error Error:   after looking in $(call HdlExists,$(call VivadoCoreLibraryChoices,$l,c))))) \
  $(infox ALLCORE1) \
  cd .. ; \
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/vivado-$(Core).out.;\
  echo Doing Vivado compile ;\
  mkdir -p $(dir $@);\
  $(VivadoMakeGenerics)\
  $(call OcpiXilinxVivadoInit); \
  vivado -mode batch -source $(OCPI_CDK_DIR)/include/hdl/vivado.tcl -log $(TargetDir)/vivado.log -journal $(TargetDir)/vivado.jou -tclargs \
    load_cores_function='$(VivadoCollectCoresTcl)' \
    prim_libs='$(VivadoLibraries)' \
    vhdl_sources='$(VhdlSources)' \
    v_sources='$(VerilogSources)' \
    inc_dirs='$(VerilogIncludeDirs)' \
    top_module='$(Top)' \
    core='$(Core)' \
    artifact='$@' \
    tgt_dir='$(dir $@)' \
    target_part='$(HdlTarget)' \
    full_part='$(VivadoPartNumber)' \
    hdl_mode='$(HdlMode)' \
    abs_path='$(abspath ./)';

# We can't trust xst's exit code so we conservatively check for zero errors
## Plus we create the edif all the time...
##($(call OcpiXilinxVivadoInit); ngc2edif -log ngc2edif-$(Top).log -w $(Top).ngc) >> $(HdlLog) 2>&1 ; 
#if grep -qE '[0-9]* Infos, [0-9]* Warnings, [0-9]* Critical Warnings, and 0 Errors encountered.' $(HdlLog); then \

HdlToolPost=\
  if grep -q ' 0 Errors encountered' $(HdlLog); then \
    HdlExit=0; \
  else \
    HdlExit=1; \
  fi;

################################################################################
# These functions choose output/log files, call the actual implementation
# steps with arguments. Exit status/errors are grepped for, and the run is 
# timed. 
# Flow: 
#   1. Make-rule calls DoXilinx
#   2. DoXilinx calls DoXilinxPat
#   3. DoXilinxPat runs implementation step
#   4. DoXilinxPat calls XilinxAfter
#   5. XilinxAfter greps for errors/warning/exit status, reports time
#   6. XilinxAfter exits with 0 if the implementation step was successful
################################################################################
# Generate the per-platform files into platform-specific target dirs
XilinxAfter=set +e;grep -i error $1.out|grep -v '^WARNING:'|grep -i -v '[_a-z]error'; \
	     if grep -q $2 $1.out || ( [ $(HdlMode) == library ] && grep -q 'Not elaborating library to speed up build.' $1.out ) ; then \
	       echo Time: `cat $1.time` at `date +%T`; \
	       exit 0; \
	     else \
	       exit 1; \
	     fi
# Use default pattern to find error string in tool output
DoXilinx=$(call DoXilinxPat,$1,$2,$3,'Exit status: 0',$(TargetDir)/$4)
DoXilinxPat=\
	echo " "Details in $5.out; $(call OcpiXilinxVivadoInit,$5.out);\
	echo Command: $1 $3 >> $5.out; \
	/usr/bin/time -f %E -o $5.time bash -c "$1 $3; RC=\$$$$?; $(ECHO) Exit status: \$$$$RC; $(ECHO) \$$$$RC > $5.status" >> $5.out 2>&1;\
	(echo -n Elapsed time:; tr -d '\n' <$5.time; echo -n ', completed at '; date +%T) >> $5.out; \
	$(call XilinxAfter,$5,$4)

###############################################################################
# The contents below this point are for final implementation steps
# of the assembly bitstream.
###############################################################################
BitName=$1/$2$(and $(filter-out 0,$3),_$3).bit
BitFile_vivado=$1.bit

SynthName=$1/$2.edf
OptName=$1/$2-opt.dcp
PlaceName=$1/$2-place.dcp
PhysOptName=$1/$2-phys_opt.dcp
RouteName=$1/$2-route.dcp
TimingName=$1/$2-timing.out

XprName=$1/$2.xpr
Name=$1/$2.xpr

# Here we are creating the command that will run the vivado script ($1) and pass this command and its arguments to DoXilinx
# We choose the log outputs ($4), 
# And Choose tcl args to pass the script:
#   e.g. implementation stage, current target, previous checkpoint, part, constraints,
# We also forward the implementation (last argument) to DoXilinx for use when naming '.out' files
DoVivado=$(call DoXilinx,vivado -mode batch -source $(OCPI_CDK_DIR)/include/hdl/$1 -log $(TargetDir)/vivado-$4.log -journal $(TargetDir)/vivado-$4.jou,$2,$3,$4)

# For synth rule: load dcp files of platform and app workers. 
define HdlToolDoPlatform_vivado

$(call OptName,$1,$3): $(call SynthName,$1,$3)
	$(AT)echo ====Optimizing
	$(AT)$(call DoVivado,vivado_impl.tcl,$1,-tclargs \
        	stage=opt \
        	target_file=$(call OptName,$1,$3) \
        	part=$(call VivadoMakePart,$(HdlPart_$5)) \
		edif_file=$(call SynthName,$1,$3) \
        	,opt)

#                load_cores_function="$(VivadoCollectCoresTcl)"
#        	checkpoint=$(call SynthName,$1,$3) \

$(call PlaceName,$1,$3): $(call OptName,$1,$3) $(wildcard $(HdlPlatformDir_$5)/*.xdc)
	$(AT)echo ====Placing
	$(AT)$(call DoVivado,vivado_impl.tcl,$1,-tclargs \
		stage=place \
		target_file=$(call PlaceName,$1,$3) \
		checkpoint=$(call OptName,$1,$3) \
		part=$(call VivadoMakePart,$(HdlPart_$5)) \
		constraints=$(wildcard $(HdlPlatformDir_$5)/*.xdc) \
		,place)

$(call PhysOptName,$1,$3): $(call PlaceName,$1,$3) $(wildcard $(HdlPlatformDir_$5)/*.xdc)
	$(AT)echo ====Physical Optimizing
	$(AT)$(call DoVivado,vivado_impl.tcl,$1,-tclargs \
		stage=phys_opt \
		target_file=$(call PhysOptName,$1,$3) \
		checkpoint=$(call PlaceName,$1,$3) \
		part=$(call VivadoMakePart,$(HdlPart_$5)) \
		,phys_opt)

$(call RouteName,$1,$3): $(call PhysOptName,$1,$3) $(wildcard $(HdlPlatformDir_$5)/*.xdc)
	$(AT)echo ====Routing
	$(AT)$(call DoVivado,vivado_impl.tcl,$1,-tclargs \
		stage=route \
		target_file=$(call RouteName,$1,$3) \
		checkpoint=$(call PhysOptName,$1,$3) \
		part=$(call VivadoMakePart,$(HdlPart_$5)) \
		,route)

$(call TimingName,$1,$3): $(call RouteName,$1,$3) $(wildcard $(HdlPlatformDir_$5)/*.xdc)
	$(AT)echo ====Timing
	$(AT)$(call DoVivado,vivado_impl.tcl,$1,-tclargs \
		stage=timing \
		target_file=$(call TimingName,$1,$3) \
		checkpoint=$(call RouteName,$1,$3) \
		part=$(call VivadoMakePart,$(HdlPart_$5)) \
		,timing)

$(call BitName,$1,$3,$6): $(call TimingName,$1,$3) $(wildcard $(HdlPlatformDir_$5)/*.xdc)
	$(AT)echo ====Writing bitstream
	$(AT)$(call DoVivado,vivado_impl.tcl,$1,-tclargs \
		stage=bit \
		target_file=$(call BitName,$1,$3,$6) \
		checkpoint=$(call RouteName,$1,$3) \
		part=$(call VivadoMakePart,$(HdlPart_$5)) \
		,bit)

endef

