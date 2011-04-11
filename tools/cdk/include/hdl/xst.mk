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
HdlToolLibraryFile=$(strip \
  $(2)/$(strip \
    $(if $(filter virtex5,$(call HdlGetFamily,$(1))),hdllib.ref,$(2).sdbl)))
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
HdlToolCoreLibName=work
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
XstLibraryFileTarget=$(if $(filter virtex5,$(call HdlGetFamily,$(1))),hdllib.ref,$(2).sdbl)
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
    -ifn -ofn -top -xsthdpini -p -ifmt -sd -vlgincdir -vlgpath -lso
# Options that the user may specify/override
XstGoodOptions=-bufg -opt_mode -opt_level -read_cores -iobuf
# Our default options, some of which may be overridden
# This is the set for building cores, not chips.
XstDefaultOptions=\
    -ifmt mixed -bufg 0 -iobuf no -opt_mode speed -opt_level 2 -ofmt NGC \
    -keep_hierarchy soft -netlist_hierarchy rebuilt -read_cores optimize \
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
# BUT! the empty module declarations must be in the lso list so to get cores we need the bb in a library too.
ifneq ($(Libraries)$(ComponentLibraries)$(Cores),)

XstLsoFile=$(Core).lso
XstIniFile=$(Core).ini
XstMakeIni=\
  ($(foreach l,$(Libraries),\
      echo $(lastword $(subst -, ,$(notdir $(l))))=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$(l),$(HdlTarget)))));) \
   $(foreach l,$(Cores),\
      echo $(lastword $(subst -, ,$(notdir $(l))))_bb=$(strip \
        $(call FindRelative,$(TargetDir),$(strip \
           $(call HdlLibraryRefDir,$(l)_bb,$(HdlTarget)))));) \
   $(foreach l,$(ComponentLibraries),echo $(notdir $(l))=$(strip \
     $(call FindRelative,$(TargetDir),$(l)/lib/hdl/stubs/$(call HdlToolLibRef,,$(HdlTarget))));))\
   > $(XstIniFile);
XstMakeLso=\
  (echo $(LibName);$(foreach l,$(Libraries) $(ComponentLibraries),\
                      echo $(lastword $(subst -, ,$(notdir $(l))));)\
  $(foreach l,$(Cores),\
            echo $(lastword $(subst -, ,$(notdir $(l))_bb));)) > $(XstLsoFile);
XstOptions += -lso $(XstLsoFile) 
endif
XstPrjFile=$(Core).prj
XstMakePrj=($(foreach f,$(HdlSources),echo verilog $(if $(filter $(WorkLibrarySources),$(f)),work,$(LibName)) '"$(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f))"';)) > $(XstPrjFile);
XstScrFile=$(Core).scr

XstMakeScr=(echo set -xsthdpdir . -xsthdpini $(XstIniFile);echo run $(strip $(XstOptions))) > $(XstScrFile);
# The options we directly specify
#$(info TARGETDIR: $(TargetDir))
XstOptions +=\
-ifn $(XstPrjFile) -ofn $(Core).ngc -top $(Top) -p $(or $(HdlExactPart),$(HdlTarget)) \
 $(and $(VerilogIncludeDirs),$(strip\
   -vlgincdir { \
     $(foreach d,$(VerilogIncludeDirs),$(call FindRelative,$(TargetDir),$(d))) \
    })) \
  $(and $(ComponentLibraries)$(Cores),-sd { \
     $(foreach l,$(ComponentLibraries),$(strip \
       $(call FindRelative,$(TargetDir),\
         $(l)/lib/hdl/$(call HdlToolLibRef,$(LibName),$(HdlTarget)))))\
     $(foreach c,$(Cores),$(sort \
	$(foreach d,$(sort \
	   $(and $(HdlPart),$(call HdlLibraryRefDir,$(c),$(HdlPart),$(HdlPart))) \
	   $(if $(findstring $(HdlTarget),$(HdlAllFamilies)),,$(call HdlLibraryRefDir,$(c),$(HdlTarget),$(HdlTarget))) \
           $(call HdlLibraryRefDir,$(c),$(HdlTarget))),\
	   $(and $(realpath $d),$(call FindRelative,$(TargetDir),$d)))))\
      })

XstNgcOptions=\
  $(foreach l,$(ComponentLibraries), -sd \
    $(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(or $(HdlPart) $(HdlTarget))))\
  $(foreach c,$(Cores), -sd \
    $(call FindRelative,$(TargetDir),$(call CoreRefDir,$(c),$(or $(HdlPart),$(HdlTarget))))) 

#$(info lib=$(Libraries)= cores=$(Cores)= Comps=$(ComponentLibraries)= td=$(TargetDir)= @=$(@))

HdlToolCompile=\
  $(foreach l,$(Libraries),\
     $(if $(wildcard $(call HdlLibraryRefDir,$(l),$(HdlTarget))),,\
          $(error Error: Specified library: "$(l)", in the "Libraries" variable, was not found for $(HdlTarget).))) \
  $(foreach l,$(Cores),\
     $(if $(wildcard $(call HdlLibraryRefDir,$(l)_bb,$(or $(HdlTarget),$(info NONE2)))),,\
	  $(info Error: Specified core library "$l", in the "Cores" variable, was not found.) \
          $(error Error:   after looking for "$(call HdlLibraryRefDir,$l_bb,$(HdlTarget))"))) \
  echo '  'Creating $@ with top == $(Top)\; details in $(TargetDir)/xst-$(Core).out.;\
  rm -f $(notdir $@);\
  $(XstMakePrj)\
  $(XstMakeLso)\
  $(XstMakeIni)\
  $(XstMakeScr)\
  $(Xilinx) xst -ifn $(XstScrFile)

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