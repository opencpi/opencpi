# This Makefile is used narrowly for the autotools-based reconf/configure/build process.
# It shows the explicit dependencies for the bootstrapping process
# The only dependencies it does not track are additions to the local/ and imported/ directories.
# Thus this file is basically a "bootstrap.sh" or "reconf" implemented using "make".
# It does rely on autoreconf knowing autotools dependencies
#
# There are three phases to this build/configure process
# The first (and its clean goal) is platform agnostic)
# After the first, the Platform(s) variable indicates platforms and build options
#
# 1. bootstrap/reconf/preconfigure which creates a configurable platform-agnostic package:
#    It is a wrapper around the typical autotools "autoreconf"
#    It is invoked by "make preconfigure" and cleaned by "make cleanbootstrap"
#    It is NOT platform-specific - it prepares the tree to build for all platforms
#    Its dependencies that are authored are:
#      -- The "places" file describing OpenCPI source directories
#      -- The generate-automake.sh script that generates the Makefile.am file
#      -- The configure.ac script
#    It also depends on the imported autoconf macros in the imports subdirectory
#    All results are in the "generated" subdirectory.
#
# 2. configuration for a platform:
#    It is a wrapper around the typical autotools "configure"
#    It is invoked by "make configure [Platform[s]=<platform>...]", cleaned by "cleanconfigure"
#    It defaults to the currently running platform with default build options
#    It imports platform information from OpenCPI <platform>-target.mk files
#    It runs configure in "VPATH" mode to keep build trees separate from source trees
#
# 3. build for a platform
#    It is a convenience for "go into the platform's build dir and make"
#    It is invoked by "make build [Platform=<platform>]"
#    Thus it simply does "make -C target-<platform>"
#    Cleaning is a specific platform is "make -C target-<platform> clean"
#
# 4. install for a platform
#    It is a convenience for "go into the platform's build dir and make install"
#    This is just a local "finalize what you built" step and doesn't install anything elsewhere

# The results of the per-platform configure process are entirely in directories
# named: target-<platform> (including build option suffix)

# Export bootstrapping
# For predictability for now we force OCPI_CDK_DIR to be the ../../bootstrap directory since this
# will make it very clear what is needed for the framework to build from scratch
# The worst case is when registered projects have rcc platforms of course
PLATFORM_AGNOSTIC:=cleanplatforms preconfigure bootstrap cleanbootstrap cleaneverything distclean
PLATFORM_SPECIFIC:=configure build clean cleanconfigure install
.PHONY: $(PLATFORM_AGNOSTIC) $(PLATFORM_SPECIFIC) show help
.DELETE_ON_ERROR:
export AT=@
ifeq ($V,1)
  Verbose=-v
  $(info VERBOSE:$(Verbose))
endif
#####################################################################################
# Determine our platform (once) if we're not cleaning, so each sub-make can assume it
#####################################################################################
ifeq ($(filter help clean% distclean%,$(MAKECMDGOALS)),)
  OcpiThisPlatformArgs:=$(shell cd ../.. && ./bootstrap/scripts/getPlatform.sh 2> /dev/null)
  $(if $(filter 6,$(words $(OcpiThisPlatformArgs))),,\
    $(error Cannot determine platform we are running on.))
  OcpiThisPlatform:=$(word 5,$(OcpiThisPlatformArgs))
  OcpiThisPlatformDir:=$(word 6,$(OcpiThisPlatformArgs))
  export OcpiThisPlatformArgs OcpiThisPlatform OcpiThisPlatformDir
  export OcpiToolDir:=$(or $(OCPI_TOOL_DIR),$(OcpiThisPlatform))
endif
# Allow singular or plural, and default to the currently running platform, status and debug
ifndef Platforms
  override Platforms:=$(Platform)
  ifndef Platforms
    override Platforms:=$(OcpiThisPlatform) # maybe look in env later
  endif
endif
include ../platform-defaults.mk

################################################################################
# Platform agnostic goals
################################################################################

help:
	@echo "This makefile has no real default goal (except this help message)"
	@echo The goals for this make file are in two categories:
	@echo 1. Those that are platform-agnostic.
	@echo 2. Those that do things for a particular software platform.
	@echo
	@echo The platform agnostic ones are, in order of typical use:
	@echo
	@echo   make cleaneverything'    #' make sure everything is as clean as it can be
	@echo   make preconfigure'       #' do one-time work for all platforms, using autoreconf
	@echo   make cleanpreconfigure'  #' undo/clean what make preconfigure did
	@echo   make cleanplatforms'     #' clean all platform-specific directories and files
	@echo
	@echo The platform-specific goals are below.
	@echo "They all operate in target-* subdirectories"
	@echo Set the Platform\(s\) variable for targeting specific platforms, including the
	@echo current one.  The default is the current one.  If not the current one, you are
	@echo cross-configuring/cross-compiling.
	@echo
	@echo   make configure' [Platforms=<platform>...]'
	@echo   make build' [Platforms=<platform>...]'
	@echo   make clean' [Platforms=<platform>...]'

show: help

MAKEFILE_AM=generated/Makefile.am
MAKEFILE_IN=generated/Makefile.in
OPTIONAL_WARNINGS=generated/optional-warnings.m4

$(MAKEFILE_IN) generated/configure: \
             $(OPTIONAL_WARNINGS) $(wildcard imports/*.m4) configure.ac $(MAKEFILE_AM) \
             | generated
	$(AT) echo Running the autotools reconfiguration tool in the generated/ directory
	$(AT) cd generated && autoreconf -v -i # no -s so it works across systems
	$(AT) touch $@ # sometimes autoreconf tries to be clever

preconfigure bootstrap: generated/configure $(MAKEFILE_IN)

# I wish this was not required so early...
$(OPTIONAL_WARNINGS): ../platform-defaults.mk | generated
	$(AT) ($(foreach v,$(OcpiAllOptionalWarningsVars), echo "define([$v],[$($v)])";)) > $@

$(MAKEFILE_AM): ../places generate-automake.sh | generated
	$(AT) echo Generating the Makefile.am file.
	$(AT) cd generated && \
	      ../generate-automake.sh $(Verbose) < ../../places > $(@F)

generated:
	$(AT) mkdir -p generated && cd generated && \
	      mkdir -p acinclude && \
	      (cd acinclude && shopt -s nullglob && ln -f -s ../../imported/*.m4 .) && \
	      ln -f -s ../{configure.ac,ocpi-config-platform.h.in} . && \
	      for i in tools tests runtime os; do ln -f -s ../../../$$i $$i; done

cleanpreconfigure:
	$(AT) echo Removing the generated directory and all platform-agnostic files.
	$(AT) rm -r -f generated

cleanplatforms:
	$(AT) echo Cleaning all platforms and their configurations
	$(AT) rm -r -f target-*

distclean cleaneverything: cleanplatforms cleanpreconfigure

#################################################################################
# Platform specific goals, optionally using Platform= and Dynamic= and Optimize=
#################################################################################
$(PLATFORM_SPECIFIC): do-platform.mk
# A macro to do a platform, called for specified platforms
MKARGS= -r -f ../do-platform.mk -C target-$1 Platform=$1
define DoPlatform

target-$1:
	mkdir target-$1
configure_$1: preconfigure do-platform.mk | target-$1
	$(AT)$(MAKE) $(MKARGS) configure
build_$1: configure_$1
	$(AT)$(MAKE) $(MKARGS) build
install_$1: build_$1 # necessary?
	$(AT)$(MAKE) $(MKARGS) install
clean_$1: # necessary?
	$(AT)$(MAKE) $(MKARGS) clean

.PHONY: configure_$1 build_$1 install_$1 clean_$1
configure: configure_$1
build: build_$1
install: install_$1
clean: clean_$1

endef

# Delegate all platform-specific goals to the submake so we iterate here
$(foreach p,$(Platforms),$(eval $(call DoPlatform,$p)))
