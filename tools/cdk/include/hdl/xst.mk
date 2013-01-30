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

# When XST compiles VHDL files it creates <lib>.vdbl, <lib>.vdbx
# When XST compiles Verilog files it creates <lib>.sdbl, <lib>.sdbx
# Core platform and assembly modes only use Verilog
# Libraries and workers may also have VHDL
ifneq ($(findstring $(HdlMode),core platform assembly container),)
  HdlLibSuffix=sdbl
else
   ifneq ($(strip $(filter %.vhd,$(CompiledSourceFiles))),)
      HdlLibSuffix=vdbl
   else
      HdlLibSuffix=sdbl
   endif
endif

HdlToolLibraryFile=$(strip \
  $2/$(if $(filter virtex6,$(call HdlGetFamily,$1)),$2.$(HdlLibSuffix),hdllib.ref))

################################################################################
# Function required by toolset: given a list of targets for this tool set
# Reduce it to the set of library targets.
HdlToolLibraryTargets=$(sort $(foreach t,$(1),$(call HdlGetFamilies,$(t))))
################################################################################
# Variable required by toolset: what is the name of the file or directory that
# is the thing created when a library is created. The thing that will be installed
HdlToolLibraryResult=$(LibName)
################################################################################
# Variable required by toolset: HdlToolCoreLibName
# What library name should we give to the library when a core is built from
# sources
# This is not "work" so that it doesn't get included when building the bb library
HdlToolCoreLibName=$(Core)
################################################################################
# Variable required by toolset: HdlBin
# What suffix to give to the binary file result of building a core
HdlBin=.ngc
################################################################################
# Variable required by toolset: HdlToolRealCore
# Set if the tool can build a real "core" file when building a core
# I.e. it builds a singular binary file that can be used in upper builds.
# If not set, it implies that only a library containing the implementation is
# possible
HdlToolRealCore=yes
################################################################################
# Variable required by toolset: HdlToolNeedBB=yes
# Set if the tool set requires a black-box library to access a core
HdlToolNeedBB=yes
################################################################################
# Function required by toolset: $(call HdlToolLibRef,libname)
# This is the name after library name in a path
# It might adjust (genericize?) the target
HdlToolLibRef=$(or $3,$(call HdlGetFamily,$2))


################################################################################
# $(call XstLibraryFileTarget2(target,libname)
# Return the actual file to depend on when it is built
XstLibraryFileTarget=$(if $(filter virtex6,$(call HdlGetFamily,$(1))),$(2).sdbl,hdllib.ref)
XstLibraryCleanTargets=$(strip \
  $(if $(filter virtex5,$(call HdlFamily,$(1))),hdllib.ref vlg??,*.sdb?))
# When making a library, xst still wants a "top" since we can't precompile 
# separately from synthesis (e.g. it can't do what vlogcomp can with isim)
# Need to be conditional on libraries
ifeq ($(HdlMode),library)
ifeq ($(Core),)
Core=onewire
Top=onewire
endif
CompiledSourceFiles:= $(OCPI_CDK_DIR)/include/hdl/onewire.v $(CompiledSourceFiles)
WorkLibrarySources:=$(WorkLibrarySources) $(OCPI_CDK_DIR)/include/hdl/onewire.v
else ifeq ($(HdlMode),platform)
XstExtraOptions=-iobuf yes -bufg 32
XstInternalOptions=-uc ../$(HdlPlatform).xcf
endif
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
OBJ:=.xxx
# Options that the user should not specify
XstBadOptions=\
    -ifn -ofn -top -xsthdpini -ifmt -sd -vlgincdir -vlgpath -lso
# Options that the user may specify/override
XstGoodOptions=-bufg -opt_mode -opt_level -read_cores -iobuf \
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

# Our default options, some of which may be overridden
# This is the set for building cores, not chips.
XstDefaultOptions=\
    -ifmt mixed -bufg 0 -iobuf no -opt_mode speed -opt_level 2 -ofmt NGC \
    -keep_hierarchy soft -netlist_hierarchy rebuilt \
    -read_cores $(if $(filter assembly,$(HdlMode)),yes,optimize) \
    -hierarchy_separator /

# Extra default ones:\
 -xor_collapse TRUE -verilog2001 TRUE -slice_packing TRUE \
 -shift_extract TRUE -register_balancing No -priority_extract Yes \
 -mux_style Auto -mux_extract Yes -decoder_extract TRUE
# check the first option in the list and recurse
XstCheckOption=\
$(if $(filter -%,$(word 1,$(1))),\
    $(if $(filter $(XstGoodOptions),$(word 1,$(1))),\
        $(if $(word 2,$(1)),\
            $(call XstCheckOption,$(wordlist 3,$(words $(1)),$(1))),\
            $(error Missing value for XST option after "$(1)")),\
        $(error XST option "$(word 1,$(1))" unknown)),\
   $(and $(1),$(error Expected hyphen in XST option "$(1)")))

# Function to check whether options are bad, good, or unknown
XstCheckOptions=\
    $(foreach option,$(XstBadOptions),\
        $(if $(findstring $(option),$(1)),\
            $(error XST option "$(option)" not allowed here)))\
    $(call XstCheckOption,$(1))

XstPruneOption=\
    $(if $(filter $(word 1,$(1)),$(XstExtraOptions)),,$(wordlist 1,2,$(1))) \
    $(if $(word 3,$(1)),\
        $(call XstPruneOption,$(wordlist 3,$(words $(1)),$(1))))

XstOptions=\
$(call XstCheckOptions,$(XstExtraOptions))\
$(call XstPruneOption,$(XstDefaultOptions)) $(XstExtraOptions) $(XstInternalOptions)

# Options and temp files if we have any libraries
# Note that "Libraries" is for precompiled libraries, whereas "OcpiLibraries" are for cores (in the -sd path),
# BUT! the empty module declarations must be in the lso list to get cores we need the bb in a library too.


XstLsoFile=$(Core).lso
XstIniFile=$(Core).ini

XstLibraries=$(HdlLibraries) \
             $(if $(findstring $(HdlMode),library),,\
               $(foreach l,$(if $(findstring util_xilinx,$(LibName)),,util_xilinx) \
                           util_$(call HdlGetFamily, $(HdlTarget)),\
                 $(and $(wildcard $(call HdlLibraryRefDir,$l,$(HdlTarget))),$l)))

XstNeedIni= $(strip $(XstLibraries)$(ComponentLibraries)$(CDKCompenentLibraries)$(CDKDeviceLibraries)$(Cores))
#   $(and $(findstring worker,$(HdlMode)),echo $(call ToLower,$(Worker))=$(call ToLower,$(Worker));) 
XstMakeIni=\
  (\
   $(foreach l,$(XstLibraries),\
      echo $(lastword $(subst -, ,$(notdir $(l))))=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$(l),$(HdlTarget)))));) \
   $(foreach l,$(Cores),\
      echo $(lastword $(subst -, ,$(notdir $(l))))_bb=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$(l)_bb,$(HdlTarget)))));) \
   $(foreach l,$(CDKComponentLibraries),echo $(notdir $(l))=$(strip \
     $(call FindRelative,$(TargetDir),$(l)/hdl/stubs/$(call HdlToolLibRef,,$(HdlTarget))));) \
   $(foreach l,$(CDKDeviceLibraries),echo $(notdir $(l))=$(strip \
     $(call FindRelative,$(TargetDir),$(l)/hdl/stubs/$(call HdlToolLibRef,,$(HdlTarget))));) \
   $(foreach l,$(DeviceLibraries),echo $(notdir $(l))=$(strip \
     $(call FindRelative,$(TargetDir),$(l)/lib/hdl/stubs/$(call HdlToolLibRef,,$(HdlTarget))));) \
   $(foreach l,$(ComponentLibraries),echo $(notdir $(l))=$(strip \
     $(call FindRelative,$(TargetDir),$(call HdlComponentLibrary,$l,stubs/$(HdlTarget))));) \
  ) > $(XstIniFile);

XstMakeLso=\
  (\
   $(if $(PreBuiltCore)$(filter worker core,$(HdlMode)),,echo work;) $(if $(findstring work,$(LibName)),,echo $(LibName);) \
   $(foreach l,$(XstLibraries) $(CDKComponentLibraries) $(CDKDeviceLibraries) $(ComponentLibraries) $(DeviceLibraries),\
                      echo $(lastword $(subst -, ,$(notdir $(l))));)\
  $(foreach l,$(Cores),\
            echo $(lastword $(subst -, ,$(notdir $(l))_bb));)) > $(XstLsoFile);
XstOptions += -lso $(XstLsoFile) 
#endif
XstPrjFile=$(Core).prj
# old XstMakePrj=($(foreach f,$(HdlSources),echo verilog $(if $(filter $(WorkLibrarySources),$(f)),work,$(LibName)) '"$(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f))"';)) > $(XstPrjFile);

VhdlSources    = ${strip ${filter %vhd, $(HdlSources)}}
VerilogSources = ${strip ${filter-out %vhd, $(HdlSources)}}
XstMakePrj=rm -f $(XstPrjFile); \
           $(if $(VerilogSources), ($(foreach f,$(VerilogSources), echo verilog $(if $(filter $(WorkLibrarySources),$(f)),work,$(LibName)) '"$(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f))"';)) >  $(XstPrjFile);, ) \
           $(if $(VhdlSources),    ($(foreach f,$(VhdlSources),    echo vhdl    $(if $(filter $(WorkLibrarySources),$(f)),work,$(LibName)) '"$(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f))"';)) >> $(XstPrjFile);, )
XstScrFile=$(Core).scr

XstMakeScr=(echo set -xsthdpdir . $(and $(XstNeedIni),-xsthdpini $(XstIniFile));echo run $(strip $(XstOptions))) > $(XstScrFile);
# The options we directly specify
#$(info TARGETDIR: $(TargetDir))
# $(and $(findstring worker,$(HdlMode)),-work_lib work) 
# $(and $(findstring worker,$(HdlMode)),-work_lib $(call ToLower,$(Worker))) 
# -p $(strip \
      $(foreach t,$(or $(HdlExactPart),$(HdlTarget)),\
        $(if $(findstring $t,$(HdlAllFamilies)),\
           $(firstword $(HdlTargets_$(t))),$t))) \

XstOptions +=\
 -ifn $(XstPrjFile) -ofn $(Core).ngc -top $(Top) \
 -p $(or $(HdlExactPart),$(HdlTarget))\
 $(and $(VerilogIncludeDirs),$(strip\
   -vlgincdir { \
     $(foreach d,$(VerilogIncludeDirs),$(call FindRelative,$(TargetDir),$(d))) \
    })) \
  $(and $(CDKComponentLibraries)$(CDKDeviceLibraries)$(ComponentLibraries)$(DeviceLibraries)$(Cores),-sd { \
     $(foreach l,$(CDKComponentLibraries),$(strip \
       $(call FindRelative,$(TargetDir),\
         $(l)/hdl/$(call HdlToolLibRef,$(LibName),$(HdlTarget)))))\
     $(foreach l,$(CDKDeviceLibraries),$(strip \
       $(call FindRelative,$(TargetDir),\
         $(l)/hdl/$(call HdlToolLibRef,$(LibName),$(HdlTarget)))))\
     $(foreach l,$(ComponentLibraries),$(strip \
       $(call FindRelative,$(TargetDir),$(call HdlComponentLibrary,$l,$(HdlTarget))))) \
     $(foreach l,$(DeviceLibraries),$(strip \
       $(call FindRelative,$(TargetDir),\
         $(l)/lib/hdl/$(call HdlToolLibRef,$(LibName),$(HdlTarget)))))\
     $(foreach c,$(Cores),$(call FindRelative,$(TargetDir),$(dir $(call HdlCoreRef,$c,$(HdlTarget)))))\
      })

XstNgcOptions=\
  $(foreach l,$(CDKComponentLibraries), -sd \
    $(call FindRelative,$(TargetDir),$(l)/hdl/$(or $(HdlPart) $(HdlTarget))))\
  $(foreach l,$(CDKDeviceLibraries), -sd \
    $(call FindRelative,$(TargetDir),$(l)/hdl/$(or $(HdlPart) $(HdlTarget))))\
  $(foreach l,$(ComponentLibraries), -sd \
    $(call FindRelative,$(TargetDir),$(l)/hdl/$(or $(HdlPart) $(HdlTarget))))\
  $(foreach l,$(DeviceLibraries), -sd \
    $(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(or $(HdlPart) $(HdlTarget))))\
  $(foreach c,$(Cores), -sd \
    $(call FindRelative,$(TargetDir),$(call HdlCoreRefDir,$(c),$(or $(HdlPart),$(HdlTarget))))) 

#$(info lib=$(HdlLibraries)= cores=$(Cores)= Comps=$(ComponentLibraries)= td=$(TargetDir)= @=$(@))

HdlToolCompile=\
  $(foreach l,$(XstLibraries),\
     $(if $(wildcard $(call HdlLibraryRefDir,$(l),$(HdlTarget))),,\
          $(error Error: Specified library: "$(l)", in the "HdlLibraries" variable, was not found for $(HdlTarget).))) \
  $(foreach l,$(Cores),\
     $(if $(wildcard $(call HdlLibraryRefDir,$(l)_bb,$(or $(HdlTarget),$(info NONE2)))),,\
	  $(info Error: Specified core library "$l", in the "Cores" variable, was not found.) \
          $(error Error:   after looking for "$(call HdlLibraryRefDir,$l_bb,$(HdlTarget))"))) \
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/xst-$(Core).out.;\
  rm -f $(notdir $@);\
  $(XstMakePrj)\
  $(XstMakeLso)\
  $(and $(XstNeedIni),$(XstMakeIni)) \
  $(XstMakeScr)\
  $(Xilinx) xst -ifn $(XstScrFile) && touch $(LibName)

# optional creation of these doesn't work...
#    $(XstMakeLso) $(XstMakeIni))\

# We can't trust xst's exit code so we conservatively check for zero errors
# Plus we create the edif all the time...
HdlToolPost=\
  if grep -q 'Number of errors   :    0 ' $(HdlLog); then \
    ($(Xilinx) ngc2edif -w $(Core).ngc) >> $(HdlLog) 2>&1 ; \
    HdlExit=0; \
  else \
    HdlExit=1; \
  fi;

#    mv $(Core).ngc temp.ngc; \
#    echo doing: ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc >> xst-$(Core).out; \
#    ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc >> xst-$(Core).out; \

# in case we need to run ngcbuild after xst
#    echo ngcbuild $(XstNgcOptions) temp.ngc $(Core).ngc; \
#    echo ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc; \
#
################################################################################
# Generate the per-platform files into platform-specific target dirs
InitXilinx=. $(OCPI_XILINX_TOOLS_DIR)/settings64.sh > /dev/null
XilinxAfter=grep -i error $1.out|grep -v '^WARNING:'|grep -i -v '[_a-z]error'; \
	     if grep -q $2 $1.out; then \
	       echo Time: `cat $1.time`; \
	       exit 0; \
	     else \
	       exit 1; \
	     fi
# Use default pattern to find error string in tool output
DoXilinx=$(call DoXilinxPat,$1,$2,$3,'Number of error.*s: *0')
DoXilinxPat=\
	echo " "Details in $1.out; cd $(call PlatformDir,$2); $(InitXilinx); \
	echo Command: $1 $3 > $1.out; \
	/usr/bin/time -f %E -o $1.time sh -c "$1 $3; echo $$? > $1.status" >> $1.out 2>&1;\
	(echo -n Time:; cat $1.time) >> $1.out; \
	$(call XilinxAfter,$1,$4)
#AppBaseName=$(PlatformDir)/$(Worker)-$(HdlPlatform)
PromName=$(call PlatformDir,$1)/$(call AppName,$1).mcs
BitName=$(call PlatformDir,$1)/$(call AppName,$1).bit
# FIXME: allow for multiple container mappings, possible platform-specific, possibly not
NgdName=$(call PlatformDir,$1)/$(call AppName,$1).ngd
NgcName=$(call PlatformDir,$1)/$(call AppName,$1).ngc
AppNgcName=$(call PlatformDir,$1)/$(ContainerModule).ngc
MapName=$(call PlatformDir,$1)/$(call AppName,$1)_map.ncd
ParName=$(call PlatformDir,$1)/$(call AppName,$1)_par.ncd
ChipScopeName=$(call PlatformDir,$1)/$(call AppName,$1)_csi.ngc
PcfName=$(call PlatformDir,$1)/$(call AppName,$1).pcf
TopNgcName=$(HdlPlatformsDir)/$1/target-$(call HdlGetPart,$1)/$1.ngc

define HdlToolDoPlatform

$(call NgdName,$1): $(call AppNgcName,$1) $(HdlPlatformsDir)/$1/$1.ucf $(call TopNgcName,$1) | $(call PlatformDir,$1)
	$(AT)echo -n For $(Worker) on $1: creating NGD '(Xilinx Native Generic Database)' file using '"ngdbuild"'.
	$(AT)$(call DoXilinx,ngdbuild,$1,\
	        -aul -aut -uc $(HdlPlatformsDir)/$1/$1.ucf -p $(HdlPart_$1) \
                $$(foreach d, ../target-$(call HdlGetFamily,$(call HdlGetPart,$1)) \
		              $$(foreach l,$$(ComponentLibraries),$$(strip \
			       $$(call FindRelative,$(call PlatformDir,$1), \
				  $$(call HdlComponentLibrary,$$l,$(HdlPart_$1))))), \
			-sd $$d) \
	        $$(call FindRelative,$(call PlatformDir,$1),$(call TopNgcName,$1)) $(notdir $(call NgdName,$1)))


# This is unnecessary
#	$(AT)$(call DoXilinx,ngcbuild,$1,\
	        -aul -aut -uc $(HdlPlatformsDir)/$1/$1.ucf -p $(HdlPart_$1) -sd ../target-$(call HdlGetFamily,$(call HdlGetPart,$1)) \
	        $$(call FindRelative,$(call PlatformDir,$1),$(call TopNgcName,$1)) $(notdir $(call NgcName,$1)))
# Map to physical elements
$(call MapName,$1): $(call NgdName,$1)
	$(AT)echo -n For $(Worker) on $1: creating mapped NCD '(Native Circuit Description)' file using '"map"'.
	$(AT)$(call DoXilinx,map,$1,-p $(HdlPart_$1) -w -logic_opt on -xe c -mt on -t $(or $(OCPI_PAR_SEED),1) -register_duplication on \
	                         -global_opt off -ir off -pr off -lc off -power off -o $(notdir $(call MapName,$1)) \
	                         $(notdir $(call NgdName,$1)) $(notdir $(call PcfName,$1)))

# Place-and-route, and generate timing report
$(call ParName,$1): $(call MapName,$1) $(call PcfName,$1)
	$(AT)echo -n For $(Worker) on $1: creating PAR\'d NCD file using '"par"'.
	$(AT)$(call DoXilinx,par,$1,-w -xe c $(notdir $(call MapName,$1)) \
		$(notdir $(call ParName,$1)) $(notdir $(call PcfName,$1)))
	$(AT)echo -n Generating timing report '(TWR)' for $1 platform design.
	$(AT)-$(call DoXilinx,trce,$1,-v 20 -fastpaths -xml fpgaTop.twx \
		-o fpgaTop.twr \
		$(notdir $(call ParName,$1)) $(notdir $(call PcfName,$1)))

# Generate bitstream
$(call BitName,$1): $(call ParName,$1) $(call PcfName,$1)
	$(AT)echo -n For $(Worker) on $1: Generating bitstream file $$@.
	$(AT)$(call DoXilinxPat,bitgen,$1,-f $(HdlPlatformsDir)/common/bitgen_bit.ut \
                $(notdir $(call ParName,$1)) $(notdir $(call BitName,$1)) \
		$(notdir $(call PcfName,$1)), 'DRC detected 0 errors')

ifdef HdlPromArgs_$1
$(call PromName,$1): $(call BitName,$1)
	$(AT)echo -n For $(Worker) on $1: Generating PROM file $$@.
	$(AT)$(call DoXilinxPat,promgen,$1, -w -p mcs -c FF $$(HdlPromArgs_$1) $(notdir $(call BitName,$1)),'.*')
#$$(info have prom for $1=$$(HdlPromArgs_$1)=)
all: $(call PromName,$1)
endif
endef
