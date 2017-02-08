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

include $(OCPI_CDK_DIR)/include/util.mk

ifneq ($(Model),test)
  $(error This file, in $(Cwd), must only be used in .test directories)
endif

# We need the project dir because remote system mount dirs point to the project
$(call OcpiIncludeProject,error)

# This is to allow the spec to be found and any protocols it depends on
ifneq ($(if $(MAKECMDGOALS),$(filter build all generate generated,$(MAKECMDGOALS)),1),)
  $(call OcpiSetXmlIncludes)
endif

# Primary goals for this Makefile, with "build" being the default (all)
.PHONY: generate build prepare run runtests runonly verify verifyonly cleanrun cleansim

# Compatility
.PHONY: test tests generated
# "make test" is a synonym for "making the tests", which is "build", which is the default
test: build
tests: build
# runtests is the high level UI goal.  Inside here it is "run"
all: build
generated: generate


# This is the representative result file that says things are properly generated.
CASEXML:=gen/cases.xml

# This is the input file describing this test suite
TESTXML:=$(CwdName)-test.xml

$(CASEXML): $(TESTXML)
	$(AT)echo ========= Generating test assemblies, inputs and applications.
	$(AT)$(OcpiGen) -v -T $< && chmod a+x gen/applications/*.sh
generate: $(CASEXML)

.PHONY: testxml
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
	$(AT)echo Preparing for execution on available platforms with available built workers and assemblies.
	$(AT)$(OCPI_CDK_DIR)/scripts/testrunprep.sh $(call FindRelative,$(realpath $(OCPI_PROJECT_DIR)),$(realpath $(CURDIR)))

runonly:
	$(AT)echo Executing tests on available or specified platforms:
	$(AT)if [ ! -e run/runtests.sh ]; then \
	       echo Execution has not been prepared.  Use make prepare. ;\
	       exit 1; \
	     fi
	$(AT)./run/runtests.sh run

run runtests: prepare
	$(AT)echo Running and verifying test outputs on available platforms: 
	$(AT)./run/runtests.sh run verify $(and $(View),view)

# only for verify only so we can use wildcard
RunPlatforms=$(foreach p,$(filter-out $(ExcludePlatforms),$(notdir $(wildcard run/*))),\
               $(if $(OnlyPlatforms),$(filter $p,$(OnlyPlatforms)),$p))
verifyonly:
	$(AT)echo ========= Verifying test outputs on prepared platforms
	$(AT)for d in $(RunPlatforms); do \
	       echo Considering platform $$d; \
	       [ ! -x run/$$d/run.sh ] || \
                (cd run/$$d; \
	         echo ================== Verifying test cases for platform $$d && \
	         ./run.sh verify $(and $(View),view)); \
	     done

#	$(AT)./run/runtests.sh verify $(and $(View),view)

view:
	$(AT)echo View test outputs on available platforms: 
	$(AT)./run/runtests.sh view

verify: run verifyonly

cleanrun:
	$(AT)rm -r -f run

cleansim:
	$(AT)rm -r -f run/*/*.simulation

clean: cleanrun
	$(AT)rm -r -f gen *.pyc
