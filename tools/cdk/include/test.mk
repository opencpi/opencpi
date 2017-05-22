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

# We include this to know the universe of possible platforms
include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
export OCPI_ALL_PLATFORMS:=$(HdlAllPlatforms:%=%.hdl) $(RccAllPlatforms:%=%.rcc)
$(infox ALL PLATFORMS ARE: $(OCPI_ALL_PLATFORMS))
# We need the project dir because remote system mount dirs point to the project
$(call OcpiIncludeProject,error)

# This is to allow the spec to be found and any protocols it depends on
ifneq ($(if $(MAKECMDGOALS),$(filter build all generate generated,$(MAKECMDGOALS)),1),)
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

# This is the representative result file that says things are properly generated.
CASEXML:=gen/cases.xml

# This is the input file describing this test suite
TESTXML:=$(CwdName)-test.xml

$(CASEXML): $(TESTXML)
	$(AT)echo ========= Generating test assemblies, inputs and applications for $(CwdName):
	$(AT)OCPI_ALL_PLATFORMS="$(OCPI_ALL_PLATFORMS)" \
	     $(OcpiGen) -v -T $< && chmod a+x gen/applications/*.sh

-include gen/*.deps
generate: $(CASEXML)

$(TESTXML):
	$(AT)[ -f $@ ] || echo '<tests/>' > $@

# This is the target to build test-related artifacts using tools
build: generate
	$(AT)[ ! -d gen/assemblies ] || $(MAKE) -C gen/assemblies

# Prepare to run by looking for available containers and generating run scripts for the
# current platform environment - this is context/platform sensitiive
# FIXME: there should be a function that simply returns the relative position within the project
#        so we don't (here or elsewhere) have to recompute it
prepare:
	$(AT)echo ======== Preparing for execution on available platforms with available built workers and assemblies for $(CwdName):
	$(AT)$(OCPI_CDK_DIR)/scripts/testrunprep.sh $(call FindRelative,$(realpath $(OCPI_PROJECT_DIR)),$(realpath $(CURDIR)))

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
	$(AT)./run/runtests.sh run verify $(and $(View),view)

# only for verify only so we can use wildcard
RunPlatforms=$(foreach p,$(filter-out $(ExcludePlatforms),$(notdir $(wildcard run/*))),\
               $(if $(OnlyPlatforms),$(filter $p,$(OnlyPlatforms)),$p))
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

clean: cleanrun
	$(AT)rm -r -f gen *.pyc
