# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2011
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

# A component library, consisting of different models built for different targets
# The expectation is that a library has spec XML in the top level "specs" dir,
# and subdirectories for each implementation.
# Active implementations are lists in the Implementations variable
# Thus this makefile just names the library and lists implementations to be built.
# The name of an implementation subdirectory includes its authoring model as the
# file extension.
# We also list the targets per model.
$(if $(wildcard $(OCPI_CDK_DIR)),,$(error OCPI_CDK_DIR environment variable not set properly.))

ifdef Workers
  ifdef Implementations
    $(error You cannot set both Workers and Implementations variables.)
  else
    Implementations := $(Workers)
  endif
endif
unexport Workers

include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
include $(OCPI_CDK_DIR)/include/ocl/ocl-make.mk
ifndef LibName
LibName=$(CwdName)
endif
include $(OCPI_CDK_DIR)/include/package.mk
ifndef Implementations
Implementations=$(foreach m,$(Models),$(wildcard *.$m))
endif
# we need to factor the model-specifics our of here...
XmImplementations=$(filter %.xm,$(Implementations))
RccImplementations=$(filter %.rcc,$(Implementations))
HdlImplementations=$(filter %.hdl,$(Implementations))
OclImplementations=$(filter %.ocl,$(Implementations))
TestImplementations=$(filter %.test,$(Implementations))
LibDir=$(OutDir)lib
GenDir=$(OutDir)gen
#LibDirs=$(foreach m,$(CapModels),$(foreach ht,$($(m)Targets),$(LibDir)/$(call UnCapitalize,$(m))/$(ht)))
XmlIncludeDirs+=specs
# default is what we are running on

build_targets := speclinks

ifneq "$(XmTargets)" ""
build_targets += xm
endif

ifneq ($(RccImplementations),)
build_targets += rcc
endif

ifneq ($(TestImplementations),)
build_targets += test
endif

ifneq ($(OclImplementations),)
build_targets += ocl
endif

ifneq ($(HdlImplementations),)
build_targets += hdl
endif

$(call OcpiDbgVar,build_targets)
# function to build the targets for an implemention.
#  First arg is model
#  second is implementation directory
ifdef OCPI_OUTPUT_DIR
PassOutDir=OCPI_OUTPUT_DIR=$(call AdjustRelative,$(OutDir:%/=%))
endif
MyMake=$(MAKE) --no-print-directory
#BuildImplementation=\
#    set -e; \
#    tn="$(call Capitalize,$(1))Targets"; \
#    t="$(or $($(call Capitalize,$1)Target),$($(call Capitalize,$(1))Targets))"; \
#    $(ECHO) =============Building $(call ToUpper,$(1)) implementation $(2) for targets: $$t; \
#    $(MyMake) -C $(2) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
#               $$tn="$$t" \

BuildImplementation=\
    set -e; \
    t="$(or $($(call Capitalize,$1)Target),$($(call Capitalize,$(1))Targets))"; \
    $(ECHO) =============Building $(call ToUpper,$(1)) implementation $(2) for targets: $$t; \
    $(MyMake) -C $(2) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
	       LibDir=$(call AdjustRelative,$(LibDir)/$(1)) \
	       GenDir=$(call AdjustRelative,$(GenDir)/$(1)) \
	       $(PassOutDir) \
	       $(and $(filter hdl,$1),HdlLibraries="$(foreach l,$(HdlLibraries),$(if $(findstring /,$l),$(call AdjustRelative,$l),$l))"\
               VerilogIncludeDirs=$(call AdjustRelative,$(VerilogIncludeDirs))) \
               XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirs))";\

BuildModel=\
$(AT)set -e;if test "$($(call Capitalize,$(1))Implementations)"; then \
  for i in $($(call Capitalize,$(1))Implementations); do \
    if test ! -d $$i; then \
      echo Implementation \"$$i\" has no directory here.; \
      exit 1; \
    else \
      $(call BuildImplementation,$(1),$$i) \
    fi;\
  done; \
fi

CleanModel=\
  $(AT)if test "$($(call Capitalize,$(1))Implementations)"; then \
    for i in $($(call Capitalize,$(1))Implementations); do \
      if test -d $$i; then \
	tn=$(call Capitalize,$(1))Targets; \
        t="$(or $(CleanTarget),$($(call Capitalize,$(1))Targets))"; \
        $(ECHO) Cleaning $(call ToUpper,$(1)) implementation $$i for targets: $$t; \
	$(MyMake) -C $$i $(PassOutDir) \
           OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) $$tn="$$t" clean; \
      fi;\
    done; \
  fi; \
  rm -r -f lib/$1 gen/$1


all: workers
workers: $(build_targets)

$(OutDir)lib:
	$(AT)mkdir $@
speclinks: | $(OutDir)lib
	$(AT)$(foreach f,$(wildcard specs/*.xml),$(call MakeSymLink,$(f),$(OutDir)lib);)

$(Models:%=$(OutDir)lib/%): | $(OutDir)lib
	$(AT)mkdir $@

$(Models:%=$(OutDir)gen/%): | $(OutDir)gen
	$(AT)mkdir $@

xm: speclinks $(XmImplementations)

rcc: speclinks $(RccImplementations)

test: speclinks $(TestImplementations)

checkocl:
	$(AT)if ! $(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_HOST)/ocpiocl test; then echo Error: OpenCL is not available; exit 1; fi

ocl: checkocl speclinks $(OclImplementations)

# this is HDL-specific:  for some HDL targets, we need to build a stub library
# so that higher level builds can reference cores using such a library.
# (e.g. xilinx xst).
# We have all the empty module definitions in the "gen/hdl" directory.
MyHdlMake=$(MyMake) $(and $(HdlTargets),HdlTargets="$(HdlTargets)")
hdlstubs: $(HdlImplementations)
	$(AT)echo =============Building HDL stub libraries for this component library \($(LibName)\)
	$(AT)(echo SourceFiles=$(foreach v,$(wildcard lib/hdl/*.v*),../../$v); \
              echo OcpiDynamicMakefile=yes; \
              echo include $(call AdjustRelative2,$(OCPI_CDK_DIR))/include/hdl/hdl-lib.mk \
	     ) > $(GenDir)/hdl/Makefile
	$(AT)$(MyHdlMake) -C $(GenDir)/hdl -L LibName=$(LibName) \
		HdlLibraries="$(foreach l,$(HdlLibraries),$(if $(findstring /,$l),$(call AdjustRelative2,$l),$l)) ocpi" \
		OCPI_CDK_DIR=$(call AdjustRelative2,$(OCPI_CDK_DIR)) \
		HdlInstallLibDir=$(call AdjustRelative2,$(LibDir)/hdl/stubs) \
		stublibrary

# hdlstubs - no longer
hdl: speclinks $(HdlImplementations)

cleanxm:
	$(call CleanModel,xm)

cleanrcc:
	$(call CleanModel,rcc)

cleantest:
	$(call CleanModel,test)

cleanocl:
	$(call CleanModel,ocl)

cleanhdl:
	$(call CleanModel,hdl)

clean:: cleanxm cleanrcc cleanocl cleantest
	$(AT)echo Cleaning library directory for all targets.
	$(AT)rm -fr $(OutDir)lib $(OutDir)gen $(OutDir)

$(HdlImplementations): | $(OutDir)lib/hdl $(OutDir)gen/hdl
	$(AT)$(call BuildImplementation,hdl,$@)

$(RccImplementations): | $(OutDir)lib/rcc
	$(AT)$(call BuildImplementation,rcc,$@)

$(TestImplementations): | $(OutDir)/$@
	$(AT)$(call BuildImplementation,test,$@)

$(OclImplementations): | $(OutDir)lib/ocl
	$(AT)$(call BuildImplementation,ocl,$@)

$(XmImplementations): | $(OutDir)lib/xm
	$(AT)$(call BuildImplementation,xm,$@)

.PHONY: $(XmImplementations) $(RccImplementations) $(TestImplementations) $(OclImplementations) $(HdlImplementations) speclinks

# Worker should only be specified when the target is "new".
ifeq ($(origin Worker),command line)
  ifneq ($(MAKECMDGOALS),new)
    $(error You cannot set the "Worker" variable unless the make goal/target is "new")
  endif
  Words:=$(subst ., ,$(Worker))
  $(if $(or $(word 3,$(Words)),$(strip \
            $(if $(word 2,$(Words)),,ok))), \
     $(error The Worker must be of the form "Worker=name.model"))
  Model:=$(call ToLower,$(word 2,$(Words)))
  ifeq ($(findstring $(Model),$(Models)),)
    $(error The suffix of "$(Worker)", which is "$(Model)" does not match any known model.)
  endif
  ifneq ($(wildcard $(Worker)),)
    $(error The worker "$(Worker)" already exists)
  endif
  Name:=$(word 1,$(Words))
  UCModel=$(call ToUpper,$(Model))
  ifeq ($(origin SpecFile),command line)
    ifeq ($(SpecFile),none)
      OcpiSpecFile:=
    else
      ifeq ($(filter %.xml,$(SpecFile)),)
        override SpecFile:=$(SpecFile).xml
      endif
      ifeq ($(wildcard $(SpecFile)),)
        ifneq ($(if $(filter /%,$(SpecFile)),,$(wildcard specs/$(SpecFile))),)
          override SpecFile:=specs/$(SpecFile)
        else
          $(error The indicated spec file for the new worker: "$(SpecFile)" does not exist.)
        endif
      endif
      ifneq ($(filter specs/$(Name)-spec.xml specs/$(Name)_spec.xml $(Name)-spec.xml $(Name)_spec.xml,$(SpecFile)),)
        OcpiSpecFile:=$(notdir $(SpecFile))
      else
        ifeq ($(dir $(SpecFile)),specs/)
          OcpiSpecFile:=$(notdir $(SpecFile))
        else
          ifeq ($(wildcard $(dir $(SpecFile))../lib/package-name),)
            $(error The given spec file, "$(SpecFile)" must be in a built component library)
          endif
          OcpiSpecFile:=$(call AdjustRelative,$(SpecFile))
        endif
      endif
    endif
  else
    # the default will be using an underscore or hypen, whichever exists
    MySpecFile:=$(or $(wildcard specs/$(Name)_spec.xml),specs/$(Name)-spec.xml)
    ifeq ($(wildcard $(MySpecFile)),)
      $(error The spec file: specs/$(Name)-spec.xml does not exist. Create it or use SpecFile=<file>)
    endif
    OcpiSpecFile:=$(notdir $(MySpecFile))
  endif
  OcpiLanguage:=
  ifdef Language
    OcpiLanguage:=$(call ToLower,$(Language))
    ifndef Suffix_$(Model)_$(OcpiLanguage)
      $(error Language "$(Language)" not supported for the $(Model) model.)
    endif
    ifeq ($(OcpiLanguage),$(Language_$(Model)))
      ifeq ($(Languages_$(Model)),)
        OcpiLanguage:=
      endif
    endif
  else
    ifeq ($(Languages_$(Model)),)
      OcpiLanguage:=$(Language_$(Model))
    else
      $(error Must specify Language=<lang> for worker using the $(Model) model.  Choices are: $(Languages_$(Model)))
    endif
  endif
  ifdef OcpiLanguage
    LangAttr:=language="$(OcpiLanguage)"
  endif
else ifdef Worker
  $(error Worker definition is invalid: it can only be on the command line with "new")
endif

# On creating a worker, the OWD is automatically generated here
# if it has non-default content: a non-default spec file or
# a non-default language.  Otherwise it is NOT created,
# which means it acts like the skeleton does:
# - A default version is created in the gen directory under "make skeleton"
# - This version is copied up to the top level (like the code skeleton)
# - This version is nuked upon make clean.



new:
	$(AT)$(if $(Worker),,\
	       $(error The "Worker=" variable must be specified when "new" is specified.)) \
	     echo Creating worker subdirectory named $(Worker).
	$(AT)mkdir -p $(Worker) && \
	     (\
              echo \# Put Makefile customizations for worker $(Worker) -$(OcpiLanguage)- here:; \
              echo;\
	      echo \# Uncomment the following line to enable cilk; \
	      echo \# Libraries = cilkrts; \
	      echo include '$$(OCPI_CDK_DIR)/include/worker.mk' \
	     ) > $(Worker)/Makefile
	$(AT)$(and $(or $(OcpiSpecFile),$(OcpiLanguage)), \
	     (\
	      echo '<$(CapModel)Worker$(and $(LangAttr),'' $(LangAttr))$(and $(OcpiSpecFile),'' spec="$(OcpiSpecFile)")>'; \
              echo '  <!-- Insert any other implementation-specific information here -->'; \
              echo '</$(CapModel)Worker>' \
	     ) > $(Worker)/$(Name).xml)
	$(AT)echo Running \"make skeleton\" to make initial skeleton in $(Worker)/$(Name).$(Suffix_$(Model)_$(or $(OcpiLanguage),$(Language_$(Model))))
	$(AT)$(MAKE) --no-print-directory -C $(Worker) \
		OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
		XmlIncludeDirs=../specs Worker= Workers= \
		skeleton; \
	     if test $$? != 0; then echo rm -r -f $(Worker); fi


