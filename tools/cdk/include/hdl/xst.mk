
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
BF:=.ngc
# Options that the user should not specify
XstBadOptions=\
    -ifn -ofn -top -xsthdpini -p -ifmt -iobuf -sd -vlgincdir -vlgpath -lso
# Options that the user may specify/override
XstGoodOptions=-bufg -opt_mode -opt_level -read_cores
# Our default options, some of which may be overridden
XstDefaultOptions=\
    -ifmt mixed -bufg 0 -iobuf no -opt_mode speed -opt_level 2 -ofmt NGC \
    -keep_hierarchy soft -netlist_hierarchy rebuilt -read_cores optimize

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

ifneq ($(origin XSTOPTIONS),undefined)
# User has specified all options
$(eval $(call XstCheckOptions,$(XSTOPTIONS)))
XstOptions=$(XSTOPTIONS)
else
# User has not specified all options
ifdef XSTEXTRAOPTIONS
# User has specified extra options to add-to or override our defaults
$(eval $(call XstCheckOptions,$(XSTEXTRAOPTIONS)))
# Accept or reject first option and then recurse.  List already checked
XstPruneOption=\
    $(if $(filter $(word 1,$(1)),$(XSTEXTRAOPTIONS)),,$(wordlist 1,2,$(1))) \
    $(if $(word 3,$(1)),\
        $(call XstPruneOption,$(wordlist 3,$(words $(1)),$(1))))
XstOptions = $(strip $(call XstPruneOption,$(XstDefaultOptions)))
XstOptions += $(XSTEXTRAOPTIONS)
else # neither XSTOPTIONS nor XSTEXTRAOPTIONS is set
XstOptions=$(XstDefaultOptions)
endif
endif
# Options and temp files if we have any libraries
# Note that "Libraries" is for precompiled libraries, whereas "OcpiLibraries" are for cores (in the -sd path),
# BUT! the empty module declarations must be in the lso list so to get cores we need the bb in a library too.
ifneq ($(Libraries)$(OcpiLibraries)$(OcpiCores),)
Family=$(if $(findstring xc5,$(Target))$(findstring virtex5,$(Target)),virtex5,virtex6)
XstLsoFile=$(Worker).lso
XstIniFile=$(Worker).ini
XstMakeIni=\
  ($(foreach l,$(Libraries) $(OcpiCores),echo $(notdir $(l))=$(strip $(call FindRelative,$(TargetDir),$(l)/$(call LibraryAccessTarget,$(Target))));)\
   $(foreach l,$(OcpiLibraries),echo $(notdir $(l))=$(strip $(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(call LibraryAccessTarget,$(Target))));)\
  ) > $(XstIniFile);
XstMakeLso=\
  (echo work;$(foreach l,$(Libraries) $(OcpiLibraries) $(OcpiCores),echo $(notdir $(l));)) > $(XstLsoFile);
XstOptions += -lso $(XstLsoFile) 
endif
XstPrjFile=$(Core).prj
XstMakePrj=($(foreach f,$(CompiledSourceFiles),echo verilog $(if $(filter $(WorkLibrarySources),$(f)),work,$(LibName)) '"$(call FindRelative,$(TargetDir),.)/$(f)"';)) > $(XstPrjFile);
XstScrFile=$(Core).scr
XstMakeScr=(echo set -xsthdpdir . -xsthdpini $(XstIniFile);echo run $(XstOptions)) > $(XstScrFile);
# The options we directly specify
Top=$(if $(Application),ocpi_app,$(Core))
XstOptions += -ifn $(XstPrjFile) -ofn $(Core) -top $(Top) -p $(Target) \
               -vlgincdir { $(foreach d,$(VerilogIncludeDirs),$(call FindRelative,$(TargetDir),$(d))) }
XstOptions += -sd { .. \
                    $(foreach l,$(OcpiLibraries),$(call FindRelative,$(TargetDir),$(l)/lib/hdl/$(Target)))\
		    $(foreach c,$(OcpiCores),$(call FindRelative,$(TargetDir),$(c)/$(Target))) \
                  }
XstNgcOptions += $(foreach c,$(OcpiCores),-sd $(call FindRelative,$(TargetDir),$(c)/$(Target)))

#$(foreach l,$(Libraries:%=%/$(call LibraryAccessTarget,$(Target))),$(if $(wildcard $(l)),,$(error Error: Specified library: "$(l)", in the "Libraries" variable, was not found.)))
#$(foreach l,$(OcpiCores:%=%/$(call LibraryAccessTarget,$(Target))),$(if $(wildcard $(l)),,$(error Error: Specified library: "$(l)", in the "OcpiCores" variable, was not found.)))

ifndef OCPI_XILINX_TOOLS_DIR
XilinxVersions=$(shell echo $(wildcard /opt/Xilinx/*/ISE_DS) | tr ' ' '\n' | sort -r)
OCPI_XILINX_TOOLS_DIR=$(firstword $(XilinxVersions))
endif
Xilinx=. $(OCPI_XILINX_TOOLS_DIR)/settings64.sh
Compile=\
  $(foreach l,$(Libraries:%=%/$(call LibraryAccessTarget,$(Target))),\
     $(if $(wildcard $(l)),,\
          $(error Error: Specified library: "$(l)", in the "Libraries" variable, was not found.))) \
  $(foreach l,$(OcpiCores:%=%/$(call LibraryAccessTarget,$(Target))),\
     $(if $(wildcard $(l)),,\
          $(error Error: Specified library: "$(l)", in the "OcpiCores" variable, was not found.))) \
  $(AT)echo Building $@  with top == $(Top)\; details in $(TargetDir)/xst.out.;\
  cd $(TargetDir);$(XstMakePrj)$(XstMakeLso)$(XstMakeIni)$(XstMakeScr)\
  ($(Xilinx) ; $(TIME) xst -ifn $(XstScrFile)) > xst.out;\
  grep -i error xst.out|grep -v '^WARNING:'|grep -i -v '[_a-z]error'; \
  if grep -q 'Number of errors   :    0 ' xst.out; then \
    exit 0; \
  else \
    exit 1; \
  fi

# in case we need to run ngcbuild after xst
#    echo ngcbuild $(XstNgcOptions) temp.ngc $(Core).ngc; \
#    echo ngcbuild -verbose $(XstNgcOptions) temp.ngc $(Core).ngc; \
#