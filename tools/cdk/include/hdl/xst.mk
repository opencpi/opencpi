
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

include $(OCPI_CDK_DIR)/include/util.mk
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

Family:=$(if $(findstring xc5,$(or $(Part),$(Target)))$(findstring virtex5,$(or $(Part),$(Target))),virtex5,virtex6)
XstLsoFile=$(Core).lso
XstIniFile=$(Core).ini
XstMakeIni=\
  ($(foreach l,$(Libraries) $(Cores),\
      echo $(lastword $(subst -, ,$(notdir $(l))))=$(call FindRelative,$(TargetDir),$(call LibraryRefDir,$(l),$(Family)));) \
   $(foreach l,$(ComponentLibraries),echo $(notdir $(l))=$(strip $(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(call LibraryAccessTarget,$(Family))));)\
  echo) > $(XstIniFile);
XstMakeLso=\
  (echo $(LibName);$(foreach l,$(Libraries) $(ComponentLibraries) $(Cores),echo $(lastword $(subst -, ,$(notdir $(l))));)) > $(XstLsoFile);
XstOptions += -lso $(XstLsoFile) 
endif
XstPrjFile=$(Core).prj
XstMakePrj=($(foreach f,$(filter %.v %.V,$^),echo verilog $(if $(filter $(WorkLibrarySources),$(f)),work,$(LibName)) '"$(call FindRelative,$(TargetDir),$(dir $(f)))/$(notdir $(f))"';)) > $(XstPrjFile);
XstScrFile=$(Core).scr
XstMakeScr=(echo set -xsthdpdir . -xsthdpini $(XstIniFile);echo run $(XstOptions)) > $(XstScrFile);
# The options we directly specify
ifndef Top
Top=$(Core)
endif
#$(info TARGETDIR: $(TargetDir))
XstOptions += -ifn $(XstPrjFile) -ofn $(Core).ngc -top $(Top) -p $(or $(Part),$(Target)) \
               $(and $(VerilogIncludeDirs),-vlgincdir { $(foreach d,$(VerilogIncludeDirs),$(call FindRelative,$(TargetDir),$(d))) }) \
                -sd { \
                    $(foreach l,$(ComponentLibraries),$(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(or $(Part),$(Target))))\
		    $(foreach c,$(Cores),$(call FindRelative,$(TargetDir),$(call CoreRefDir,$(c),$(or $(Part),$(Target)))))\
                  }
XstNgcOptions= $(foreach l,$(ComponentLibraries), -sd \
                 $(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(or $(Part) $(Target))))\
	       $(foreach c,$(Cores), -sd \
                 $(call FindRelative,$(TargetDir),$(call CoreRefDir,$(c),$(or $(Part),$(Target))))) 

ifndef OCPI_XILINX_TOOLS_DIR
XilinxVersions=$(shell echo $(wildcard /opt/Xilinx/*/ISE_DS) | tr ' ' '\n' | sort -r)
OCPI_XILINX_TOOLS_DIR=$(firstword $(XilinxVersions))
endif
Xilinx=. $(OCPI_XILINX_TOOLS_DIR)/settings64.sh
#$(info lib=$(Libraries)= cores=$(Cores)= Comps=$(ComponentLibraries)= td=$(TargetDir)= @=$(@))
Compile=\
  $(foreach l,$(Libraries),\
     $(if $(wildcard $(call LibraryRefDir,$(l),$(Family))),,\
          $(error Error: Specified library: "$(l)", in the "Libraries" variable, was not found.))) \
  $(foreach l,$(Cores),\
     $(if $(wildcard $(call LibraryRefDir,$(l),$(or $(Part) $(Target)))),,\
          $(error Error: Specified core library: "$(call LibraryRefDir,$(l),$(or $(Part) $(Target)))", in the "Cores" variable, was not found.))) \
  $(AT)echo '  'Creating $@  with top == $(Top)\; details in $(TargetDir)/xst-$(Core).out.;\
  cd $(TargetDir);\
  $(XstMakePrj)\
  $(XstMakeLso)\
  $(XstMakeIni)\
  $(XstMakeScr)\
  $(Xilinx)> xst-$(Core).out ; (echo Command: xst -ifn $(XstScrFile); $(TIME) xst -ifn $(XstScrFile)) >> xst-$(Core).out;\
  grep -i error xst-$(Core).out|grep -v '^WARNING:'|grep -i -v '[_a-z]error'; \
  if grep -q 'Number of errors   :    0 ' xst-$(Core).out; then \
    ngc2edif -w $(Core).ngc >> xst-$(Core).out; \
    exit 0; \
  else \
    exit 1; \
  fi

#    mv $(Core).ngc temp.ngc; \
#    echo doing: ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc >> xst-$(Core).out; \
#    ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc >> xst-$(Core).out; \

# in case we need to run ngcbuild after xst
#    echo ngcbuild $(XstNgcOptions) temp.ngc $(Core).ngc; \
#    echo ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc; \
#