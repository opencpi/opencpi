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

################################################################################
# This file sets up the environment variables for a target platform.
# It either called by ocpitarget.sh to help it set up a shell environment, or
# included in Make environments that need this setup.
# The OcpiPlatform variable is set before including it to indicate which platform to build
# Or the OCPI_TARGET_PLATFORM environment variable can be used if set.
$(if $(OCPI_CDK_DIR),,\
  $(error the file $(lastword $(MAKEFILE_LIST)) included without OCPI_CDK_DIR set))
ifndef OcpiPlatform
  ifdef OCPI_TARGET_PLATFORM
    OcpiPlatform:=$(OCPI_TARGET_PLATFORM)
  else
    $(error the file $(lastword $(MAKEFILE_LIST)) included without OcpiPlatform or OCPI_TARGET_PLATFORM set))
  endif
endif
# This may be already included
include $(OCPI_CDK_DIR)/include/util.mk
# separate the build options from the platform.
OcpiPlatformArgs:=$(shell cd ../../.. && \
                    $(OCPI_CDK_DIR)/scripts/getPlatform.sh $(OcpiPlatform) 2> /dev/tty)
$(if $(filter 6,$(words $(OcpiPlatformArgs))),,\
    $(error Cannot find platform $(OcpiPlatform) for cross-building))
OcpiPlatformOs:=$(word 1,$(OcpiPlatformArgs))
OcpiPlatformOsVersion:=$(word 2,$(OcpiPlatformArgs))
OcpiPlatformArch:=$(word 3,$(OcpiPlatformArgs))
# words 4 and 5 are redundant
OcpiPlatformDir:=$(word 6,$(OcpiPlatformArgs))
# This can be called more than once to reset back to defaults (needed ?)
include $(OCPI_CDK_DIR)/include/platform-defaults.mk
ifneq ($(wildcard $(OcpiPlatformDir)/$(OcpiPlatform).mk),)
  OcpiPlatformPrevars:=$(.VARIABLES)
  include $(OcpiPlatformDir)/$(OcpiPlatform).mk
  $(foreach v,$(filter Ocpi% OCPI%,$(.VARIABLES)),\
    $(if $(strip $(filter $v,OcpiPlatformPrevars $(OcpiPlatformPrevars))\
		 $(filter $v,$(OcpiAllPlatformVars))),,\
       $(error Software platform definition file $(OcpiPlatformDir)/$(OcpiPlatform).mk has illegal variable: $v)))
else
  $(error Missing $(OcpiPlatformDir)/$(OcpiPlatform).mk file)
endif
# We switch to environment variables when moving these values from the make world to bash world
# FIXME: Do we need to do this?
# FIXME: Do we need to export them in the make environment?
export OCPI_TARGET_PLATFORM:=$(OcpiPlatform)
export OCPI_TARGET_DIR:=$(OcpiPlatform)
export OCPI_TARGET_OS:=$(OcpiPlatformOs)
export OCPI_TARGET_OS_VERSION:=$(OcpiPlatformOsVersion)
export OCPI_TARGET_ARCH:=$(OcpiPlatformArch)
export OCPI_TARGET_PLATFORM_DIR:=$(OcpiPlatformDir)
export OCPI_TARGET_CROSS_COMPILE:=$(OcpiCrossCompile)
export OCPI_TARGET_PREREQUISITES:=$(OcpiPlatformPrerequisites)
export OCPI_TARGET_DYNAMIC_FLAGS:=$(OcpiDynamicLibraryFlags)
export OCPI_TARGET_DYNAMIC_SUFFIX:=$(OcpiDynamicLibrarySuffix)
export OCPI_TARGET_EXTRA_LIBS:=$(OcpiExtraLibs)
ifndef OCPI_TARGET_KERNEL_DIR
  export OCPI_TARGET_KERNEL_DIR:=$(strip \
    $(foreach d,$(OcpiKernelDir),$(if $(filter /%,$d),$d,$(OcpiPlatformDir)/$d)))
endif
# This will export shell variables to replace the original platform-target.sh scripts:
ifdef ShellTargetVars
$(foreach v,\
  OS OS_VERSION ARCH DIR PLATFORM PLATFORM_DIR KERNEL_DIR CROSS_COMPILE PREREQUISITES \
  DYNAMIC_FLAGS DYNAMIC_SUFFIX EXTRA_LIBS,\
  $(info OCPI_TARGET_$v="$(OCPI_TARGET_$v)"; export OCPI_TARGET_$v;))
endif
