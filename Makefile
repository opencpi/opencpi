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
.NOTPARALLEL:
ifneq ($(filter-out cleandriver,$(filter show help clean% distclean%,$(MAKECMDGOALS))),)
  $(if $(and $(OCPI_CDK_DIR),$(realpath $(OCPI_CDK_DIR))),,\
    $(if $(OCPI_CDK_DIR),\
      $(foreach p,$(realpath $(CURDIR)),\
        $(if $(filter $(realpath $(OCPI_CDK_DIR)),$p/cdk $p/exports),\
           $(warning Missing exports link when performing: $(MAKECMDGOALS).)\
	   $(warning Setting OCPI_CDK_DIR temporarily to $(CURDIR)/bootstrap.))))\
    $(eval export OCPI_CDK_DIR:=$(CURDIR)/bootstrap))
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
    $(and $(call DoShell,./scripts/makeExportLinks.sh -b -,Error),$(error $(Error)))
  endif
endif
include $(OCPI_CDK_DIR)/include/util.mk

##########################################################################################
# Process all platform info specified (and default RccPlatforms if not set at all).
# FIXME someday: we need to treat models more uniformly so we can more easily add them
$(eval $(OcpiEnsureToolPlatform))
override Platforms:=$(call Unique,$(strip $(Platforms) $(Platform)))
export Platforms # why?
# Read in the database of actual RCC and HDL, setting RccPlatforms if it has not been set.
# We use rcc-make.mk rather than rcc-targets.mk because it ALSO sets a default RCC platform etc.
# We also do not want to use the default RCC platform if Platforms/Platform is set
ifdef Platforms
  ifeq ($(origin RccPlatforms),undefined)
    RccPlatforms:=
  endif
endif
include $(OCPI_CDK_DIR)/include/rcc/rcc-make.mk
include $(OCPI_CDK_DIR)/include/hdl/hdl-targets.mk

# Now check all platforms for validity, even the hdl:rcc pairs
$(foreach p,$(subst :, ,$(Platform)),$(if $(filter $p,$(RccAllPlatforms) $(HdlAllPlatforms)),,\
  $(error Platform $p is specified but non-existent, RCC or HDL.  HDL platforms may not be built yet)))

# Check that all RCC platforms and the second of pairs are valid RCC platforms
$(foreach p,$(RccPlatforms),$(if $(filter $p,$(RccAllPlatforms)),,\
  $(error RCC platform $p is specified but not a known RCC platform)))

# Check that all HDL platforms and the first of pairs are valid HDL platforms
HdlPlatforms:=$(call Unique,$(strip $(HdlPlatforms) $(HdlPlatform)))
$(foreach h,$(HdlPlatforms) $(foreach p,$(Platforms),$(and $(findstring :,$p),$(word 1,$(subst :, ,$p)))),\
  $(if $(filter $h,$(HdlAllPlatforms)),,\
    $(error HDL platform $h is specified but not a known HDL platform)))
# Add any RCC platforms in $(Platforms) to the RCC platforms list (not second of pairs though)
override \
RccPlatforms:=$(call Unique,$(RccPlatforms)\
                $(foreach p,$(Platforms),$(if $(filter $p,$(RccAllPlatforms)),$p)))
# Add any HDL platforms in $(Platforms) to the HDL platforms list including first of pairs
override \
HdlPlatforms:=$(call Unique,$(HdlPlatforms)\
                $(foreach h,$(Platforms),$(if $(filter $p,$(HdlAllPlatforms)),$p)))
export Platforms # why?
export RccPlatforms
export HdlPlatforms

# Macro to use when platform pairs (<hdl>:<rcc>) are allowed
# Take a platform arg that might be <hdl>, <rcc>, or <hdl>:<rcc> return <rcc>:<hdl>, with
# either possibly being -, and the implicit <rcc> for <hdl> retrieved when not explicit,
# And ensure that all things specified are valid platforms
GetRccHdlPlatform=$(strip\
  $(foreach f,$(word 1,$(subst :, ,$1)),\
    $(foreach s,$(or $(word 2,$(subst :, ,$1)),-),\
      $(if $(filter-out -,$s),\
        $(if $(filter $f,$(HdlAllPlatforms)),,$(error Unknown/unbuilt HDL platform: $f))\
        $(if $(filter $s,$(RccAllPlatforms)),,$(error Unknown RCC platform: $s)),\
        $(if $(filter $f,$(HdlAllPlatforms) $(RccAllPlatforms)),,\
           $(error Platform $f is neither an RCC platform or a (built) HDL platform)))\
      $(foreach r,$(or $(filter $f,$(RccAllPlatforms)),$(filter-out -,$s),$(HdlRccPlatform_$f),-),\
        $(foreach h,$(or $(filter $f,$(HdlAllPlatforms)),-),$r:$h)))))

##########################################################################################
# Goals that are not about projects

# The exports script makeExportLinks.sh needs to know what we already know about the platforms.
# Feed the required info into makeExportLinks on a silver platter.
DoExports=\
  $(foreach p,$(or $(Platforms),$(RccPlatforms) $(HdlPlatforms)),\
    $(foreach x,$(call GetRccHdlPlatform,$p),\
      $(foreach r,$(word 1,$(subst :, ,$x)),\
        $(foreach h,$(word 2,$(subst :, ,$x)),\
          $(infox x:$x r:$r h:$h)\
          $(if $(and $(filter-out -,$h),$(filter -,$r)),\
            $(warning The HDL platform "$h" has no RCC platform.  It will be ignored.))\
          ./scripts/makeExportLinks.sh $r $(RccPlatformDir_$r) $h $(HdlPlatformDir_$h) &&)))) :

.PHONY: exports      framework      driver      testframework cleanpackaging \
        cleanexports cleanframework cleanprojects cleandriver clean distclean cleaneverything
all framework:
	$(AT)$(MAKE) -C build/autotools install Platforms="$(RccPlatforms)"
	$(AT)$(DoExports)

cleanframework:
	$(AT)$(MAKE) -C build/autotools clean Platforms="$(RccPlatforms)"

# This still relies on the projects being built, and runs lots of things,
# but does not run unit tests in the non-core projects
testframework:
	$(AT)for p in $(RccPlatforms); do ./scripts/test-opencpi.sh --platform $$p; done

exports:
	$(AT)echo Updating exports for platforms: $(or $(Platforms),$(RccPlatforms)) >&2
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
	             $(MAKE) -C os/$o/driver AT=$(AT) OCPI_TARGET_PLATFORM=$p;\
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
	             echo Cleaning the $o kernel driver for $(call RccRealPlatforms,$p); \
		     $(MAKE) -C os/$o/driver OCPI_TARGET_PLATFORM=$p topclean; \
	           else \
	             echo There is no kernel driver for the OS '"'$o'"', so none cleaned. ; \
	           fi;))) \

# Clean that respects platforms
clean: cleanprojects cleanframework

# Super clean, but not git clean, based on our patterns, not sub-make cleaning
cleaneverything distclean: clean cleandriver cleanpackaging
	$(AT)$(call DoProjects,cleaneverything)
	$(AT)find . -depth -a ! -name .git  -a ! -path "./prerequisites*" -a \( \
             -name '*~' -o -name '*.dSym' -o -name timeData.raw -o -name 'target-*' -o \
             -name "*.lo" -o -name "*.o" -o \( -type d -a -name simulations \) -o \
	     \( -name lib -a -type d -a \
	       ! -path "*/rcc/platforms/*" -a ! -path "./prerequisites*" \) -o \
	     \( -name gen -a -type d -a \
	       ! -path "*/rcc/platforms/*" -a ! -path "./prerequisites*" \)  \
             \) -exec rm -r {} \;
	$(AT)rm -r -f exports

# Documentation (AV-4402)
.PHONY: doc
.SILENT: doc
# This hack determines how many jobs you are allowing by pulling bytes out of the make
# jobserver https://stackoverflow.com/a/48865939/836748
doc:
	$(AT)rm -rf doc/{pdfs,html}
	$(AT)+[[ "${MAKEFLAGS}" =~ --jobserver[^=]+=([0-9]+),([0-9]+) ]] && ( J=""; while read -t0 -u $${BASH_REMATCH[1]}; do read -N1 -u $${BASH_REMATCH[1]}; J="$${J}$${REPLY}"; done; JOBS="$$(expr 1 + $${#J})" doc/generator/genDocumentation.sh; echo -n $$J >&$${BASH_REMATCH[2]} ) || doc/generator/genDocumentation.sh


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
name=$(base)$(and $(call cross,$1),-$2-platform-$(call cross,$1))
release=$(or $(OcpiRelease),$(Release),snapshot$(tag)$(git_tag))
# This changes every 6 minutes which is enough for updated releases (snapshots).
# It is rebased after a release so it is relative within its release cycle
# FIXME:automate this...
timestamp:=_$(shell printf %05d $(shell expr `date -u +"%s"` / 360 - 4273900))
##### Set variables based on what git can tell us
# Get the git branch and clean it up from various prefixes and suffixes tacked on
# If somebody checks in between Jenkins builds, it will sometimes get "develop^2~37" etc,
# The BitBucket prefix of something like "bugfix--" is also stripped if present.
git_branch :=$(notdir $(shell git name-rev --name-only HEAD | \
                              perl -pe 's/~[^\d]*$$//' | perl -pe 's/^.*?--//'))
git_version:=$(shell echo $(git_branch) | perl -ne '/^v[\.\d]+$$/ && print')
git_hash   :=$(shell h=`(git tag --points-at HEAD | grep github | head -n1) 2>/dev/null`;\
                     [ -z "$$h" ] && h=`git rev-list --max-count=1 HEAD`; echo $$h)
# git_tag is used in *.spec files for RPM release tag.
# Any non alphanumeric (or .) strings converted to single _
git_tag    :=$(if $(git_version),,$(strip\
               $(if $(BUILD_NUMBER),_J$(BUILD_NUMBER)))$(strip\
               $(if $(filter-out undefined develop,$(git_branch)),\
                    _$(shell echo $(git_branch) | sed -e 's/[^A-Za-z0-9.]\+/_/g'))))
##### Set final variables that depend on git variables
# This could be nicer, but at least it gets it from the true source, which should be places
version:=$(or $(Version),$(git_version),$(strip\
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
	            -$(version)$(if $(git_version),,-)$(release))
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

.PHONY: rpm rpm_runtime rpm_devel
real_platforms:=$(call Unique,$(call RccRealPlatforms,$(RccPlatforms)))
# Build rpms or deployment packages for all platforms in Platforms
# If you want to do a hardware platform for a specific software package
# use this syntax <hw-platform>:<specific-sw-platform>
# e.g.: make rpm Platforms="zed:xilinx13_4"

# Call the right rpm packaging script with the right arguments.  This macro takes
# one argument which if set indicates hw deployment rather than RPM building
DoRpmOrDeployHw=\
  $(foreach arg,$(or $(Platforms),$(OCPI_TOOL_PLATFORM)),\
    $(foreach pair,$(call GetRccHdlPlatform,$(arg)),\
      $(foreach r,$(word 1,$(subst :, ,$(pair))),\
        $(foreach h,$(word 2,$(subst :, ,$(pair))),\
          $(foreach p,$(if $(filter-out -,$h),$h,$r),\
            $(if $(filter -,$h),\
              ./packaging/make-sw-rpms.sh $(and $(RpmVerbose),-v) $p \
                 "$(and $(call cross,$p),1)" $(Package) $(base) $(call name,$p,sw) \
                  $(release) $(version) $(git_hash),\
              $(if $1,\
                ./packaging/make-hw-deploy.sh $(and $(RpmVerbose),-v) $p \
                  "$(and $(call cross,$p),1)" $r,\
                ./packaging/make-hw-rpms.sh $(and $(RpmVerbose),-v) $p \
                  "$(and $(call cross,$p),1)" $(Package) $(base) $(call name,$p,hw) \
                   $(release) $(version) $(git_hash) $r)) && ))))) :
rpm: exports
	$(AT)$(call DoRpmOrDeployHw,)

deploy: exports
	$(AT)$(call DoRpmOrDeployHw,1)

cleanpackaging:
	$(AT)rm -r -f packaging/target-*

# The following are used by jenkins to identify hw-platforms that can be used
# for a given sw-platform
real_platforms:=$(filter-out $(strip $(OCPI_TARGET_PLATFORM) $(OCPI_TOOL_PLATFORM)),\
                             $(call Unique,$(call RccRealPlatforms,$(RccPlatforms))))
DoHw=$(strip\
  $(if $(filter 1,$(words $(real_platforms))),\
    $(foreach h,$(HdlAllPlatforms),$(if $(filter $(HdlRccPlatform_$h) $(HdlAllRccPlatforms_$h),$(real_platforms)),$h)),\
    $(error Cannot show hardware platforms or directories for more than one platform at a time)))

showhw:
	$(AT)echo $(DoHw)
showhwdir:
	$(AT)echo $(foreach h,$(DoHw),$(HdlPlatformDir_$h))

##########################################################################################
# Goals that are about prerequisites
# Here in the Makefile to enable install-prerequisites.sh for multiple platforms
# Use Force=1 to rebuild even if it appears it was all done before.
# There is currently no dependency on prerequisites from building the framework.
.PHONY: prerequisites cleanprerequisites
prerequisites:
	$(AT)for p in $(call RccRealPlatforms,$(RccPlatforms)); do \
                ./scripts/install-prerequisites.sh $(and $(filter 1,$(Force)),-f) $$p || exit 1; \
             done
cleanprerequisites:
	$(AT)rm -r -f prerequisites-build prerequisites
##########################################################################################
# Goals that are about projects
# A convenience to run various goals on all the projects that are here
# Unfortunately, we need to know the order here.
Projects=core assets inactive assets_ts
ProjectGoals=cleanhdl cleanrcc cleanocl rcc ocl hdl applications run runtest hdlprimitives \
             hdlportable components cleancomponents test
# These are not done in parallel since we do not know the dependencies
DoProjects=set -e; . cdk/opencpi-setup.sh -r; $(foreach p,$(Projects),\
                     echo Performing $1 on project $p && \
                     $(MAKE) -C projects/$p $(if $(filter build,$1),,$1) &&) :
.PHONY: $(ProjectGoals) testprojects
$(ProjectGoals):
	$(AT)$(call DoProjects,$@)

.PHONY: projects
projects:
	$(AT)$(call DoProjects,build)
	$(AT)$(call DoProjects,test)
	$(AT)$(DoExports)

testprojects:
	$(AT)$(call DoProjects,runtest)

cleanprojects:
	$(AT)$(call DoProjects,clean)

rcc ocl hdl: exports

##########################################################################################
# Help
define help
This top-level Makefile builds and/or tests the framework and built-in projects ($(Projects))

The valid goals that accept platforms (using RccPlatform(s) or Platforms(s)) are:
   Make goals for framework (core of Opencpi):
     framework(default) - Build the framework for platforms and export them
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
     doc                - Creates PDFs from LaTeX and Open/LibreOffice source.
                        - Requires additional software and tries to help identify missing reqs.

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
