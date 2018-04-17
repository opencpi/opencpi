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

# A component library, consisting of different models built for different targets
# The expectation is that a library has spec XML in the top level "specs" dir,
# and subdirectories for each implementation.
# Active implementations are lists in the Implementations variable
# Thus this makefile just names the library and lists implementations to be built.
# The name of an implementation subdirectory includes its authoring model as the
# file extension.
# We also list the targets per model.
$(if $(wildcard $(OCPI_CDK_DIR)),,$(error OCPI_CDK_DIR environment variable not set properly.))

include $(OCPI_CDK_DIR)/include/util.mk
# Include library settings for this library, which are available here and for workers
# Thus library settings can depend on project settings
$(OcpiIncludeAssetAndParent)

ifneq ($(origin Workers),undefined)
  ifneq ($(origin Implementations),undefined)
    $(error You cannot set both Workers and Implementations variables.)
  else
    Implementations := $(Workers)
  endif
endif
unexport Workers

HdlInstallDir=lib
include $(OCPI_CDK_DIR)/include/hdl/hdl-make.mk
$(eval $(HdlPreprocessTargets))
$(infox HP2:$(HdlPlatform) HPs:$(HdlPlatforms) HT:$(HdlTarget) HTs:$(HdlTargets):$(CURDIR))
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
ifeq ($(OCPI_HAVE_OPENCL),1)
  include $(OCPI_CDK_DIR)/include/ocl/ocl-make.mk
endif
ifndef LibName
LibName=$(CwdName)
endif
ifeq ($(origin Implementations),undefined)
Implementations=$(foreach m,$(Models),$(wildcard *.$m))
endif
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
$(shell mkdir -p lib; \
        workers_content="$(filter-out %.test, $(Implementations))"; \
        if [[ ! -e $(LibDir)/workers || "$$workers_content" != "$$(cat lib/workers)" ]]; then \
          echo $$workers_content > lib/workers; \
        fi)
endif
# We define this empty make rule so that workers can generate the "workers" file
# by calling "make workersfile -C ../". Doing so will trigger the code block above
# which is executed for all make rules except clean%
workersfile:

# we need to factor the model-specifics our of here...
XmImplementations=$(filter %.xm,$(Implementations))
RccImplementations=$(filter %.rcc,$(Implementations))
HdlImplementations=$(filter %.hdl,$(Implementations))
ifeq ($(OCPI_HAVE_OPENCL),1)
OclImplementations=$(filter %.ocl,$(Implementations))
endif
TestImplementations=$(filter %.test,$(Implementations))
AssyImplementations=$(filter %.assy,$(Implementations))

# if no tests are in the explicit workers list...
# must eval here hence ifeq
ifeq ($(TestImplementations),)
  ifeq ($(origin Tests),undefined)
    TestImplementations:=$(subst %/,%,$(dir $(wildcard *.test/Makefile)))
  else
    TestImplementations:=$(Tests)
  endif
endif
override LibDir=$(OutDir)lib
override GenDir=$(OutDir)gen
# In case this library is a subdirectory that might receive XmlIncludeDirs from the
# parent (e.g. when a platform directory has a "devices" library as a subdirectory
override XmlIncludeDirs+=$(XmlIncludeDirsInternal)

# Utility to show what WOULD be built (e.g. for packaging)
.PHONY:  showxm showrcc showhdl showocl showtest showassy showall workersfile
.SILENT: showxm showrcc showhdl showocl showtest showassy showall workersfile
showxm:
	echo $(XmImplementations)

showrcc:
	echo $(RccImplementations)

showhdl:
	echo $(HdlImplementations)

showocl:
	echo $(OclImplementations)

showtest:
	echo $(TestImplementations)

showassy:
	echo $(AssyImplementations)

# Do NOT sort these or proxies may be out-of-order:
showall:
	echo $(XmImplementations) $(RccImplementations) $(HdlImplementations) $(OclImplementations) $(TestImplementations) $(AssyImplementations)

# default is what we are running on

build_targets := speclinks

ifneq "$(XmTargets)" ""
build_targets += xm
endif

ifneq ($(RccImplementations),)
build_targets += rcc
endif

ifeq ($(OCPI_HAVE_OPENCL),1)
ifneq ($(OclImplementations),)
build_targets += ocl
endif
endif

ifneq ($(HdlImplementations),)
build_targets += hdl
endif

ifneq ($(AssyImplementations),)
build_targets += assy
endif

#ifneq ($(TestImplementations),)
#build_targets += test
#endif

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

HdlLibrariesCommand=$(call OcpiAdjustLibraries,$(HdlLibraries))
RccLibrariesCommand=$(call OcpiAdjustLibraries,$(RccLibraries))
TestTargets:=$(call Unique,$(HdlPlatforms) $(HdlTargets) $(RccTargets))
# set the directory flag to make, and use the desired Makefile
GoWorker=-C $1 -f $(or $(realpath $1/Makefile),$(OCPI_CDK_DIR)/include/worker.mk)
BuildImplementation=$(infox BI:$1:$2:$(call HdlLibrariesCommand):$(call GoWorker,$2)::)\
    set -e; \
    t="$(foreach t,$(or $($(call Capitalize,$1)Target),$($(call Capitalize,$1)Targets)),\
         $(call $(call Capitalize,$1)TargetDirTail,$t))";\
    $(ECHO) =============Building $(call ToUpper,$(1)) implementation $(2) for target'(s)': $$t; \
    $(MyMake) $(call GoWorker,$2) OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
	       LibDir=$(call AdjustRelative,$(LibDir)/$(1)) \
	       GenDir=$(call AdjustRelative,$(GenDir)/$(1)) \
	       $(PassOutDir) \
	       ComponentLibrariesInternal="$(call OcpiAdjustLibraries,$(ComponentLibraries))" \
	       $(call Capitalize,$1)LibrariesInternal="$(call OcpiAdjustLibraries,$($(call Capitalize,$1)Libraries))" \
	       $(call Capitalize,$1)IncludeDirsInternal="$(call AdjustRelative,$($(call Capitalize,$1)IncludeDirs))" \
               XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirs))" $3;\

BuildModel=\
$(AT)set -e;if test "$($(call Capitalize,$(1))Implementations)"; then \
  for i in $($(call Capitalize,$(1))Implementations); do \
    if test ! -d $$i; then \
      echo Implementation \"$$i\" has no directory here.; \
      exit 1; \
    else \
      $(call BuildImplementation,$(1),$$i,$2) \
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
	$(AT)if ! test -x $(ToolsDir)/ocpiocltest || ! $(ToolsDir)/ocpiocltest test; then echo Error: OpenCL is not available; exit 1; fi

ifeq ($(OCPI_HAVE_OPENCL),1)
ocl: checkocl speclinks $(OclImplementations)
else
ocl:
	$(AT)echo No OpenCL installed so no OCL workers built.
endif

.PHONY: hdl
hdl: speclinks $(HdlImplementations)
	$(AT)for i in $(HdlTargets); do mkdir -p lib/hdl/$$i; done

assy: speclinks $(AssyImplementations)
	$(AT)for i in $(HdlTargets); do mkdir -p lib/hdl/$$i; done

cleanxm:
	$(call CleanModel,xm)

cleanassy:
	$(call CleanModel,assy)

cleanrcc:
	$(call CleanModel,rcc)

cleantest:
	$(call CleanModel,test)

cleanocl:
	$(call CleanModel,ocl)

cleanhdl:
	$(call CleanModel,hdl)

clean:: cleanocl

clean:: cleanxm cleanrcc cleanhdl cleantest
	$(AT)echo Cleaning \"$(CwdName)\" component library directory for all targets.
	$(AT)find . -depth -name gen -exec rm -r -f "{}" ";"
	$(AT)find . -depth -name "target-*" -exec rm -r -f "{}" ";"
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

$(AssyImplementations): | $(OutDir)lib/assy
	$(AT)$(call BuildImplementation,assy,$@)

.PHONY: $(XmImplementations) $(RccImplementations) $(TestImplementations) $(OclImplementations) $(HdlImplementations) $(AssyImplementations) speclinks hdl

# Worker should only be specified when the target is "new".
ifeq ($(origin Device),command line)
  ifdef Worker
    $(error You can not set both the Worker and the Device variables.)
  endif
  Worker:=$(Device)
else
  ifdef Worker
    ifneq ($(origin Worker),command line)
      $(error Worker definition is invalid: it can only be on the command line with "new")
    endif
  endif
endif
ifdef Worker
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
    ifdef Emulate
      $(error Cannot specify Emulate= variable with SpecFile);
    endif
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
          ifeq ($(wildcard $(dir $(SpecFile))../lib/package-id),)
            $(error The given spec file, "$(SpecFile)" must be in a built component library)
          endif
          OcpiSpecFile:=$(call AdjustRelative,$(SpecFile))
        endif
      endif
    endif
  else ifdef Emulate
    ifeq ($(wildcard $(Emulate)),)
      $(error There is no worker named '$(Emulate)' as specified in the Emulate variable.)
    endif
    ifndef Device
      Device:=$(Worker)
    endif
    OcpiSpecFile:=emulator-spec
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
	       $(error The "Worker=" or "Device=" variable must be specified when "new" is specified.)) \
	     echo Creating worker subdirectory named $(Worker).
	$(AT)mkdir -p $(Worker) && \
	     (\
              echo \# Put Makefile customizations for worker $(Worker) here:; \
              echo;\
	      echo include '$$(OCPI_CDK_DIR)/include/worker.mk' \
	     ) > $(Worker)/Makefile
	$(AT)$(and $(OcpiSpecFile)$(OcpiLanguage)$(Device)$(Emulate), \
	     (\
	      echo '<$(if $(Device),HdlDevice,$(CapModel)Worker)$(and $(LangAttr),'' $(LangAttr))$(and $(OcpiSpecFile),'' spec="$(OcpiSpecFile)")$(and $(Emulate),'' emulate="$(Emulate)" spec="emulator")>'; \
	      $(if $(OcpiSpecFile),,echo '  <componentspec$(and $(Emulate),'' nocontrol="1")/>';) \
              echo '  <!-- Insert any other implementation-specific information here -->'; \
              echo '</$(if $(Device),HdlDevice,$(CapModel)Worker)>' \
	     ) > $(Worker)/$(Name).xml)
	$(AT)echo Running \"make skeleton\" to make initial skeleton in $(Worker)/$(Name).$(Suffix_$(Model)_$(or $(OcpiLanguage),$(Language_$(Model))))
	$(AT)$(MAKE) --no-print-directory -C $(Worker) \
		OCPI_CDK_DIR=$(call AdjustRelative,$(OCPI_CDK_DIR)) \
                XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirs)) ../lib/hdl"\
		Worker= Workers= \
		skeleton; \
	     if test $$? != 0; then echo You should probably do: rm -r -f $(Worker); fi


cleanall:
	$(AT)find . -depth -name gen -exec rm -r -f "{}" ";"
	$(AT)find . -depth -name "target-*" -exec rm -r -f "{}" ";"
	$(AT)rm -r -f lib
