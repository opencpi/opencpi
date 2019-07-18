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

# This file is the generic Makefile for .test directories in component libraries.
# The makefile for all unit test directories (ending in .test in component libraries)

$(if $(wildcard $(OCPI_CDK_DIR)),,$(error OCPI_CDK_DIR environment variable not set properly.))

ifdef Case
  override Cases+= $(Case)
endif
ifdef OnlyPlatform
  override OnlyPlatforms+= $(OnlyPlatform)
endif
ifdef ExcludePlatform
  override ExcludePlatforms+= $(ExcludePlatform)
endif
export OnlyPlatforms
export ExcludePlatforms
export Cases
export KeepSimulations
export TestTimeout
include $(OCPI_CDK_DIR)/include/util.mk

ifneq ($(Model),test)
  $(error This file, in $(Cwd), must only be used in .test directories)
endif


# This is the representative result file that says things are properly generated.
CASEXML:=gen/cases.xml

# Support build C++ executables, which may use the OpenCPI ACI, along with subsidiary
# source files.  The two variables are:
# TestApplications for the names of the apps with top level eponymous C++ source files
# SourceFiles for any subsidiary source files that can be C or C++
# Then we need to arrange for scripts mentioned in the -test.xml file to call them,
# even though they will of course be built in the target-* subdirectories.
ifdef TestApplications
  OcpiApps:=$(TestApplications)
  include $(OCPI_CDK_DIR)/include/aci.mk
  # allow the C++ executables to be found when running generate or verify scripts
  export PATH:=$(CURDIR)/target-$(OCPI_TOOL_DIR):$(PATH)
  $(CASEXML): aciapps
endif
# We include this to know the universe of possible platforms
include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk
include $(OCPI_CDK_DIR)/include/rcc/rcc-targets.mk
include $(OCPI_CDK_DIR)/include/ocl/ocl-make.mk

# We need the project dir because remote system mount dirs point to the project
#
# Tests do not have their own package-id file
# They inherit the containing library's package-id
$(call OcpiIncludeAssetAndParent,..,,error)

# This is to allow the spec to be found and any protocols it depends on
ifneq ($(if $(MAKECMDGOALS),$(filter test build all generate generated,$(MAKECMDGOALS)),1),)
  $(call OcpiSetXmlIncludes)
endif

# Primary goals for this Makefile, with "build" being the default (all)
# First global test goals
.PHONY: $(OcpiTestGoals)
# Next, generic goals
.PHONY: run clean generate build
# Next local test-only goals
.PHONY: prepare runonly verify runnoprepare view
# Map global goals to local goals
test: build
runtests: run
tests: build
verifytest: verify
all: build
runonlytest: runonly
runtest: run

# This is the input file describing this test suite
TESTXML:=$(CwdName)-test.xml

$(CASEXML): $(TESTXML)
	$(AT)echo ========= Generating test assemblies, inputs and applications for $(CwdName):
	$(AT)$(call OcpiGen, -v -T $<)

-include gen/*.deps
generate: $(CASEXML)

$(TESTXML):
	$(AT)[ -f $@ ] || echo '<tests/>' > $@

# This is the target to build test-related artifacts using tools
build: generate
	$(AT)[ ! -d gen/assemblies ] || \
		$(MAKE) -C gen/assemblies \
			ComponentLibrariesInternal="../../.. $(call OcpiAdjustLibraries,$(ComponentLibraries))" \
			LibrariesInternal="$(call OcpiAdjustLibraries,$(Libraries))" \
			HdlLibrariesInternal="$(call OcpiAdjustLibraries,$(HdlLibraries))" \
			IncludeDirsInternal="$(call AdjustRelative,$(IncludeDirs))" \
			XmlIncludeDirsInternal="$(call AdjustRelative,$(XmlIncludeDirs))";

# Prepare to run by looking for available containers and generating run scripts for the
# current platform environment - this is context/platform sensitiive
# FIXME: there should be a function that simply returns the relative position within the project
#        so we don't (here or elsewhere) have to recompute it
prepare:
	$(AT)echo ======== Preparing for execution on available platforms with available built workers and assemblies for $(CwdName):
	$(AT)if [ -d gen/applications ]; then \
	       $(OCPI_CDK_DIR)/scripts/testrunprep.sh \
                    $(call FindRelative,$(realpath $(OCPI_PROJECT_DIR)),$(realpath $(CURDIR))); \
	     else \
	       echo No tests generated here so preparation skipped.; \
	     fi

runnoprepare:
	$(AT)echo ======== Executing tests on available or specified platforms for $(CwdName):
	$(AT)if [ ! -e run/runtests.sh ]; then \
	       echo Execution has not been prepared.  Use make prepare. ;\
	       exit 1; \
	     fi
	$(AT)./run/runtests.sh run

runonly: prepare runnoprepare
# runtests is for compatibility
# run is generic (not just for tests)
run: prepare
	$(AT)echo ======== Running and verifying test outputs on available platforms for $(CwdName):
	$(AT)if [ -d gen/applications ]; then \
	       ./run/runtests.sh run verify $(and $(View),view); \
	     else \
	       echo No tests generated here so none run.; \
	     fi

# only for verify only so we can use wildcard
RunPlatforms=$(foreach p,$(filter-out $(ExcludePlatforms),$(notdir $(wildcard run/*))),\
               $(if $(OnlyPlatforms),$(filter $p,$(OnlyPlatforms)),$p))
ifneq ($(origin HdlPlatforms),undefined)
  ifdef OnlyPlatforms
    OnlyPlatforms:=$(filter-out $(filter-out $(HdlPlatforms),$(HdlAllPlatforms)),$(OnlyPlatforms))
    # If we had OnlyPlatforms, and we are building for none of them, we are building for nothing
    ifndef OnlyPlatforms
      override HdlPlatforms:=
      MAKEOVERRIDES:=$(filter-out HdlPlatforms=% HdlPlatform=%,$(MAKEOVERRIDES))
    endif
  else ifdef ExcludePlatforms
    ExcludePlatforms:=$(call Unique,$(filter-out $(HdlPlatforms),$(HdlAllPlatforms)) $(ExcludePlatforms))
  else
    ExcludePlatforms:=$(filter-out $(HdlPlatforms),$(HdlAllPlatforms))
  endif
endif

verify:
	$(AT)if [ ! -d run ]; then \
	       echo No tests have been run so none can be verified.; \
	       exit 0; \
	     fi; \
	     echo ======== Verifying test outputs on prepared platforms for $(CwdName): ; \
	     for d in $(RunPlatforms); do \
	       [ ! -x run/$$d/run.sh ] || \
                (cd run/$$d; \
		 props=(*.props); \
		 if [ "$$props" = '*.props' ]; then \
		   echo ============ No test case executions to verify for platform $$d; \
		 else \
	           echo ============ Verifying test cases for platform $$d && \
	           ./run.sh verify $(and $(View),view); \
		 fi); \
	     done

#	$(AT)./run/runtests.sh verify $(and $(View),view)

view:
	$(AT)echo View test outputs on available platforms: 
	$(AT)./run/runtests.sh view

cleanrun:
	$(AT)rm -r -f run

cleansim:
	$(AT)rm -r -f run/*/*.simulation

clean:: cleanrun
	$(AT)rm -r -f gen *.pyc
##########################################################################################
# Help
define help
The make command is used with the following goals to invoke the phases:
  build — (the default goal if none is specified) build locally generated HDL
           assemblies, implies generate if needed
  run   — perform execution and verification, interleaved per subcase
  clean — clean all generated, built and execution directories and files

The following goals control the process at a finer granularity:
  generate     — perform all offline generation tasks: scripted input data and property
                 values, applications, HDL assemblies, generation report
  prepare      — discover platforms and artifacts and generate execution scripts per
                 platform, assumes required artifacts are built
  runnoprepare — execute tests assuming a previous prepare was done
  runonly      — prepare and execute tests, but do not perform any verification
  verify       — perform verification for whatever platforms have been executed
  view         — run the view scripts and put the results in separate windows
  cleanrun     — clean all run results
  cleansim     — clean all simulation output (but not run results)

The following variables can be set, usually on the command line:
  Verbose              - set to 1 for putting the execution logs on the console
  TestAccumulateErrors - set to 1 to continue after errors until all platforms/cases are done
  Cases                - set to specific cases, including wildcards for execute and/or verify
  KeepSimulations      - set to 1 to preserve simulation outputs rather than delete on success
  TestTimeout          - set to number of seconds to limit execution for any case
  OnlyPlatforms        - set to platforms to run tests on, rather than those available
  View                 - set to 1 to enable the "view" script during verify
  TestApplications     - set to C++ programs to build (set only in the Makefile)

endef
$(OcpiHelp)
