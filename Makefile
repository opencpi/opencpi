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

##########################################################################################
ifneq ($(filter show help clean% distclean%,$(MAKECMDGOALS)),)
  ifndef OCPI_CDK_DIR
    export OCPI_CDK_DIR:=$(CURDIR)/bootstrap
  endif
else
  ifndef OCPI_CDK_DIR
    export OCPI_CDK_DIR:=$(CURDIR)/cdk
  endif
  # Note that if we are running this makefile, we are in a source tree, thus we force a local
  # prerequisites dir if it is not set.
  ifndef OCPI_PREREQUISITES_DIR
    export OCPI_PREREQUISITES_DIR=$(CURDIR)/prerequisites
  endif
  ifeq ($(wildcard exports),)
    include $(CURDIR)/bootstrap/include/util.mk
    $(info Exports have never been set up here.  Doing it now for platform-independent items.)
    $(and $(call DoShell,./scripts/makeExportLinks.sh - -,Error),$(error $(Error)))
  endif
endif
include $(OCPI_CDK_DIR)/include/util.mk
$(eval $(OcpiEnsureToolPlatform))
override \
RccPlatforms:=$(call Unique,\
                $(or $(strip $(RccPlatforms) $(RccPlatform) $(Platforms) $(Platform)),$(strip\
                     $(OCPI_TARGET_PLATFORM)),$(strip\
	             $(OCPI_TOOL_PLATFORM))))
export RccPlatforms
DoExports=for p in $(RccPlatforms); do ./scripts/makeExportLinks.sh $$p; done
DoTests=for p in $(RccPlatforms); do ./scripts/test-opencpi.sh $$p; done
# Get macros and rcc platform/target processing, as well as all platforms
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk

##########################################################################################
# Goals that are not about projects
.PHONY: exports      framework      driver      testframework \
        cleanexports cleanframework cleanprojects cleandriver clean distclean cleaneverything
all framework:
	$(AT)$(MAKE) -C build/autotools install Platforms="$(RccPlatforms)"
	$(AT)$(DoExports)

cleanframework:
	$(AT)$(MAKE) -C build/autotools clean Platforms="$(RccPlatforms)"

# This still relies on the projects being built, and runs lots of things,
# but does not run unit tests in the non-core projects
testframework:
	$(AT)$(DoTests)

exports:
	$(AT)echo Updating exports for platforms: $(RccPlatforms) >&2
	$(AT)$(DoExports)

cleanexports:
	$(AT)rm -r -f exports

driver:
	$(AT)set -e;\
	     $(foreach p,$(RccPlatforms),\
	       $(foreach t,$(RccTarget_$p),\
	         $(foreach o,$(call RccOs,$t),\
	           if test -d os/$o/driver; then \
	             echo Building the $o kernel driver for $(call RccRealPlatforms,$p); \
	             $(MAKE) -C os/$o/driver AT=$(AT) OcpiPlatform=$p;\
	           else \
	             echo There is no kernel driver for the OS '"'$o'"', so none built. ; \
	           fi;))) \
	     $(DoExports) \

cleandriver:
	$(AT)set -e;\
	     $(foreach p,$(RccPlatforms),\
	       $(foreach t,$(RccTarget_$p),\
	         $(foreach o,$(call RccOs,$t),\
	           if test -d os/$o/driver; then \
	             echo Cleaning the $o kernel driver for $(call RccRealPlatform,$p); \
		     $(MAKE) -C os/$o/driver OcpiPlatform=$p topclean; \
	           else \
	             echo There is no kernel driver for the OS '"'$o'"', so none cleaned. ; \
	           fi;))) \

# Clean that respects platforms
clean: cleanframework cleanprojects

# Super clean, but not git clean, based on our patterns, not sub-make cleaning
cleaneverything distclean: clean cleandriver
	$(AT)rm -r -f exports
	$(AT)find . -depth -a ! -name .git  -a ! -path "./prerequisites*" -a \( \
             -name '*~' -o -name '*.dSym' -o -name timeData.raw -o -name 'target-*' -o \
             -name "*.lo" -o -name "*.o" -o -name gen -o \
	     \( -name lib -a -type d -a \
	       ! -path "*/rcc/platforms/*" -a ! -path "./prerequisites*" \)  \
             \) -exec rm -r {} \;
	$(AT)for p in projects/*; do [ -d $$p ] && make -C $$p cleaneverything || :; done

##########################################################################################
# Goals, variables and macros that are about packaging the CDK, whether tarball, rpm, etc.
##### Set variables that affect the naming of the release packages
# The general package naming scheme is:
# <base>[-sw-platform][-<platform>]-<version>[-<release>][_<tag>][_J<job>][_<branch>][<dist>]
# where:
# <base> is our core package name: opencpi
# [-sw-platform] is omitted for development platforms
# [-<platform>] is the OpenCPI platform being distributed
#   Omitted in RPMs for dev platforms since it is redundant with the RPM suffix (e.g. el7.x86_64)
# <version> is our normally versioning scheme 1.2.3
# <release> is a label that defaults to "snapshot" if not overridden with OcpiRelease
#          Omitted for an actual official release
# These are only applied if not a specific versioned release:
# <tag> is a sequence number/timestamp within a release cycle (when not a specific release)
# <job> is a jenkins job reference if this process is run under Jenkins
# <branch> is a git branch reference if not "develop" or "undefined"
# <dist> added by RPM building, e.g. el7.x86_64
# And the appropriate suffix is added:  .rpm, or .tar.gz
base=opencpi
# cross: value is the target platform or null if not cross
# arg 1 is a single platform
cross=$(strip $(foreach r,$(call RccRealPlatforms,$1),\
        $(if $(filter $(call RccRealPlatforms,$(OCPI_TOOL_PLATFORM)),$r),,$r)))
name=$(base)$(and $(call cross,$1),-sw-platform-$(call cross,$1))
release=$(or $(OcpiRelease),snapshot)
# This changes every 6 minutes which is enough for updated releases (snapshots).
# It is rebased after a release so it is relative within its release cycle
# FIXME:automate this...
timestamp:=_$(shell printf %05d $(shell expr `date -u +"%s"` / 360 - 4231562))
##### Set variables based on what git can tell us
# Get the git branch and clean it up from various prefixes and suffixes tacked on
# If somebody checks in between Jenkins builds, it will sometimes get "develop^2~37" etc,
# The BitBucket prefix of something like "bugfix--" is also stripped if present.
git_branch :=$(notdir $(shell git name-rev --name-only HEAD | \
                              perl -pe 's/~[^\d]*$$//' | perl -pe 's/^.*?--//'))
git_version:=$(shell echo $(git_branch) | perl -ne '/^v[\.\d]+$$/ && print')
git_hash   :=$(shell h=`(git tag --points-at HEAD | grep github | head -n1) 2>/dev/null`;\
                     [ -z "$$h" ] && h=`git rev-list --max-count=1 HEAD`; echo $$h)
git_tag    :=$(if $(git_version),,$(strip\
               $(if $(BUILD_NUMBER),_J$(BUILD_NUMBER)))$(strip\
               $(if $(filter-out undefined develop,$(git_branch)),_$(subst -,_,$(git_branch)))))
##### Set final variables that depend on git variables
# This could be nicer, but at least it gets it from the true source, which should be places
version:=$(or $(git_version),$(strip\
              $(shell sed -n 's/.*AC_INIT.\[opencpi\],\[\([0-9.]*\)\].*$$/\1/p' \
                      build/autotools/configure.ac)))
tag:=$(if $(git_version),,$(timestamp))
#$(info GIT_VERSION:$(git_version): GIT_TAG:$(git_tag): GIT_HASH:$(git_hash): GIT_BRANCH:$(git_branch):)
#$(info NAME:$(name): TAG:$(tag): VERSION:$(version): RELEASE:$(release): PACKAGE:$(Package):)
Package=runtime
# This name applies generically, but is not used for RPMs since there are other issues about
# being as "normal" as possible in the RPM context.  See the args passed to rpmbuild
package_name=$(name)$(if $(filter runtime,$(Package)),,-$(Package))$(strip\
                    $(foreach p,$(RccPlatforms),-$p))$(strip\
	            -$(version)$(if $(git_version),,-)$(release)$(tag)$(git_tag))
#$(info PACKAGE_NAME:$(package_name):)
Prepare=./packaging/prepare-package-list.sh $(Package) "$(RccPlatforms)" $(call cross,$(word 1,$(RccPlatforms)))

.PHONY: test_packaging
test_packaging: exports
	$(AT)$(Prepare)

# Make a tarball of exports
# We convert the "Prepare" output mappings suitable for cp -R, into tar command transformations
# This should be moved into the packaging subdir...
.PHONY: tar
tar: exports
	$(AT)set -e; file=$(package_name).tar temp=$(mktemp -t tarcmdXXXX); \
	     echo Determining tar export file contents for the $(Package) package: $$file.gz; \
	     (echo "tar -h -f $$file -c \\";\
	      $(Prepare) |\
	        ( while read source dest; do\
	          if [ -n "$$dest" ]; then \
	            [ -d "$$source" ] && \
	              echo ' --xform="s@^'$$source/@$$dest/$$(basename $$source)/'@" \'; \
	            echo '  --xform="s@^'$$source\\\$$@$$dest/$$(basename $$source)'@" \'; \
	          fi; \
	          if [[ $$source == *@ ]]; then \
	            symlinks="$$symlinks $${source/@}"; \
	          else \
	            echo " $$source \\"; \
	          fi; \
	          done ; echo; \
	          echo "tar --append -f $$file $$symlinks") \
	     ) > $$temp; \
	     echo Creating tar export file: $$file.gz; \
	     sh $$temp && rm -f $$file.gz $$temp && gzip $$file

# Create a relocatable RPM from what is exported for the given platforms
# Feed in the various naming components
# redefine _rpmdir and _build_name_fmt to simply output the RPMs here
# This should be moved to the packaging subdir...
.PHONY: rpm rpm_runtime rpm_devel
first_real_platform:=$(word 1,$(RccPlatforms))
ifneq ($(and $(filter rpm,$(MAKECMDGOALS)),$(filter command line,$(origin Package))),)
  $(error You cannot specify a Package when creating RPMs, they are all created together)
endif
rpm: exports
	$(AT)! command -v rpmbuild >/dev/null 2>&1 && \
	  echo "Error: Cannot build an RPM: rpmbuild (rpm-build package) is not available." && \
	  exit 1 || :
	$(AT)[ $(words $(call Unique,$(call RccRealPlatforms,$(RccPlatforms)))) != 1 ] && \
	     echo Error: Cannot build an RPM for more than one platform at a time. && exit 1 || :
	$(AT)echo "Creating RPM file(s) for platform:" $(first_real_platform)
	$(AT)echo "  For the $(if $(filter driver,$(Package)),driver package,runtime and devel packages)."
	$(AT)$(eval first:=$(word 1,$(RccPlatforms))) \
	     target=packaging/target-$(first) && mkdir -p $$target && \
	     source $(OCPI_CDK_DIR)/scripts/ocpitarget.sh $(first) && \
	     p=$$OCPI_TARGET_PLATFORM_DIR/$(first)-packages.sh &&\
             ( [ -f $$p ] && $$p list | head -1 | xargs -n 1 | sed 's/^/Requires:/' || : \
             ) > $$target/devel-requires && \
	     rpmbuild $(if $(RpmVerbose),-vv,--quiet) -bb\
		      --define="RPM_BASENAME    $(base)"\
		      --define="RPM_NAME        $(call name,$(first))"\
		      --define="RPM_RELEASE     $(release)$(tag)$(git_tag)"\
		      --define="RPM_VERSION     $(version)" \
		      --define="RPM_HASH        $(git_hash)" \
		      --define="RPM_PLATFORM    $(first)" \
		      --define="RPM_OPENCPI     $(CURDIR)" \
		      $(foreach c,$(call cross,$(first)),\
		        --define="RPM_CROSS $c") \
		      --define "_rpmdir $(CURDIR)/packaging/target-$(first)"\
		      --define "_build_name_fmt %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm"\
		      packaging/$(if $(filter driver,$(Package)),driver,cdk).spec && \
	     echo "Created RPM file(s) in $$target:" && ls -l $$target

##########################################################################################
# Goals that are about prerequisites
# Here in the Makefile to enable install-prerequisites.sh for multiple platforms
# Use Force=1 to rebuild even if it appears it was all done before.
# There is currently no dependency on prerequisites from building the framework.
.PHONY: prerequisites cleanprerequisites
prerequisites:
	$(AT)for p in $(call RccRealPlatforms,$(RccPlatforms)); do\
                ./scripts/install-prerequisites.sh $(and $(filter 1,$(Force)),-f) $$p;\
             done
cleanprerequisites:
	$(AT)rm -r -f prerequisites-build prerequisites
##########################################################################################
# Goals that are about projects
# A convenience to run various goals on all the projects that are here
# Unfortunately, we need to know the order here.
Projects=core assets inactive
ProjectGoals=cleanhdl cleanrcc cleanocl rcc ocl hdl applications run runtest hdlprimitives \
             components cleancomponents test
# These are not done in parallel since we do not know the dependencies
DoProjects=set -e; $(foreach p,$(Projects),\
                     echo Performing $1 on project $p && \
                     $(MAKE) -C projects/$p $(if $(filter build,$1),,$1) &&) :
.PHONY: $(ProjectGoals)
$(ProjectGoals):
	$(AT)$(call DoProjects,$@)

.PHONY: projects
projects:
	$(AT)$(call DoProjects,build)
	$(AT)$(call DoProjects,test)

cleanprojects:
	$(AT)$(call DoProjects,clean)

rcc ocl hdl: exports

##########################################################################################
# Help
define help
This top-level Makefile builds and/or tests the framework and built-in projects ($(Projects))

The valid goals that accept platforms (using RccPlatform(s) or Platforms(s)) are:
   Make goals for framework (core of Opencpi):
     framework(default) - Build the framework for platfors and export them
     cleanframework     - Clean the specific platforms
     exports            - Redo exports, including for indicated platforms
                        - This is cumulative;  previous exports are not removed
                        - This does not export projects or do exports for projects
     driver             - Build the driver(s) for the platform(s)
     testframework      - Test the framework, requires the projects be built
                        - Runs component unit tests in core project, but not in others
     cleandriver        - Clean the driver(s) for the platform(s)
     tar                - Create the tarball for the current cdk exports (exported platforms)
     rpm                - Create the binary/relocatable CDK RPM for the platforms
   Make goals for projects: (be selective using Projects=...)
     projects           - Build the projects for the platforms
     cleanprojects      - Clean all projects
     exportprojects     - Export all projects
   Other goals:
     clean              - clean framework and projects, respecting Platforms and Projects
     cleaneverything    - Clean as much as we can (framework and projects) without git cleaning
                        - also distclean does this for historical/compatible reasons
                        - ignores the Platform and Projects variables
     prerequisites      - Forces a (re)build of the prerequisites for the specified platforms.
                        - Downloads will be downloaded if they are not present already.
     cleanprerequisites - Clean out all built, downloaded prerequisites.

Variables that are useful for most goals:

Platforms/Platform/RccPlatforms/RccPlatform: all specify software platforms
  -- Useful for goals:  framework(default), exports, cleanframework, projects, exportprojects,
                        driver, cleandriver, prerequisites, tar, rpm
  -- Platforms can have build options/letters after a hyphen: d=dynamic, o=optimized 
     <platform>:    default static, debug build
     <platform>-d:  dynamic library, debug build
     <platform>-o:  static library, optimized build
     <platform>-do: dynamic library, optimized build

Projects: specify projects (default is: $(Projects))
  -- Useful for goals:  projects, cleanprojects, exportprojects, testprojects

The projects for project-related goals use the Projects variable.
The default is all the built-in projects (including inactive) in order: $(Projects)
These various project-related goals simply perform the goal in all projects:
   rcc ocl hdl applications test run runtest hdlprimitives components
   cleanhdl cleanrcc cleanocl
Variables that only affect project building can also be used, like HdlPlatforms.
endef
$(OcpiHelp)
