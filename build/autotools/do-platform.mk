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

################################################################################################
# Do per-platform goals in the target directory for a specific platform and build options
# It is assumed to be called as a sub-make with the currently running platform already
# established using the (exported) OcpiThisPlatform* variables.
# It expects the Platform variable to be set on the command line
.DELETE_ON_ERROR:
AT=@
.PHONY: configure build clean cleanconfigure install

# unexpected
all:
	$(AT)exit 1
OcpiRelativeTop=../../..
# Gather target information if we're not cleaning
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
  # separate the build options from the platform.
  OcpiPlatform=$(word 1,$(subst -, ,$(Platform)))
  OcpiPlatformDynamic=$(findstring d,$(word 2,$(subst -, ,$(Platform))))
  OcpiPlatformOptimize=$(findstring o,$(word 2,$(subst -, ,$(Platform))))

  ifeq ($(OcpiPlatform),$(OcpiThisPlatform))
    OcpiPlatformArgs=$(OcpiThisPlatformArgs)
  else
    # Cross Compilation is indicated
    $(info Cross compiling on "$(OcpiThisPlatform)" for targetting "$(OcpiPlatform)")
    $(if $(wildcard $(OcpiRelativeTop)/exports/$(OcpiToolDir)/bin/ocpigen),,\
      $(error $(CURDIR)Cannot cross-build for "$(Platform)" when the running platform, \
        "$(OcpiThisPlatform)" is not built))
    # This usage of getPlatform.sh is finding the directory of a specified platform,
    # *not* figuring out what platform we are running on.
    OcpiPlatformArgs:=$(shell cd $(OcpiRelativeTop) && \
                        ./bootstrap/scripts/getPlatform.sh $(OcpiPlatform) 2> /dev/tty)
    $(if $(filter 6,$(words $(OcpiPlatformArgs))),,\
      $(error Cannot find platform $(OcpiPlatform) for cross-building))
  endif

  OcpiPlatformOs:=$(word 1,$(OcpiPlatformArgs))
  OcpiPlatformOsVersion:=$(word 2,$(OcpiPlatformArgs))
  OcpiPlatformArch:=$(word 3,$(OcpiPlatformArgs))
  # words 4 and 5 are redundant
  OcpiPlatformDir:=$(word 6,$(OcpiPlatformArgs))
  $(info Target platform "$(OcpiPlatform)" exists and appears valid.)

  include $(OcpiRelativeTop)/bootstrap/include/platform-defaults.mk
  include $(OcpiPlatformDir)/$(OcpiPlatform).mk
  ifneq ($(OcpiPlatform),$(OcpiThisPlatform))
    ifndef OcpiCrossCompile
      $(error The platform $(OcpiPlatform) does not appear to support cross compilation))
    endif
  endif
endif

platform-variables.sh: \
           $(OcpiRelativeTop)/bootstrap/include/platform-defaults.mk $(OcpiPlatformDir)/$(OcpiPlatform).mk \
           ../do-platform.mk
	$(AT)($(foreach v,$(OcpiAllPlatformVars), echo $v=\"$($v)\";)) > platform-variables.sh

configure: Makefile

Makefile: ../gen/configure platform-variables.sh ../do-platform.mk ../gen/Makefile.in
	$(AT) echo Configuring for platform $(Platform) in $(basename $(CURDIR)).
	$(AT) build=$$(../gen/build-aux/config.guess); \
              [ -n "$$build" ] || exit 1; \
	      $(and $(OcpiCrossCompile),\
	        PATH=$(patsubst %/,%,$(dir $(OcpiCrossCompile))):$$PATH;) \
              ../gen/configure --build=$$build --enable-silent-rules \
	        $(and $(OcpiPlatformDynamic),--enable-dynamic=yes) \
	        $(and $(OcpiPlatformOptimize),--enable-debug=no) \
	        prerequisite_dir=$(or $(PrerequisitesDir),$(OcpiRelativeTop)/prerequisites) \
                $(and $(OcpiCrossCompile),\
                   --host=$(patsubst %-,%,$(notdir $(OcpiCrossCompile))))
	        CC=$(OcpiCrossCompile)$(OcpiCC) \
	        CXX=$(OcpiCrossCompile)$(OcpiCXX) \
	        LD=$(OcpiCrossCompile)$(OcpiLD) \
	        AR=$(OcpiCrossCompile)$(OcpiAR) \
	        STRIP=$(OcpiCrossCompile)$(OcpiSTRIP) 

# OcpiPlatformArch)-none-$(OcpiPlatformOs)

build:
	$(AT) echo Building platform $(Platform) in $(basename $(CURDIR)).
	$(AT) $(and $(OcpiCrossCompile),\
	        PATH=$(patsubst %/,%,$(dir $(OcpiCrossCompile))):$$PATH;) $(MAKE)

install:
	$(AT) echo Installing for platform $(OcpiPlatform) into $(TargetDir)/staging
	$(AT) rm -r -f staging
	$(AT) $(and $(OcpiCrossCompile),\
	        PATH=$(patsubst %/,%,$(dir $(OcpiCrossCompile))):$$PATH;) $(MAKE) install

clean:
	$(AT) make clean
