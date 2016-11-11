# This make file fragment establishes environment variables for user Makefiles,
# all based on OCPI_CDK_DIR, which might be set already, but may not.
# If not set, it is assumed that this file is being included from its proper location
# in a CDK and the CDK location is inferred from that.
# This means that a user makefile could simply include this file however it wants and
# OCPI_CDK_DIR and friends will all be set properly.
# The goal here to have *no* environment setup requirement for a user app on the 
# development system with default settings.
ifndef OCPISETUP_MK
export OCPISETUP_MK:=1
OcpiThisFile=$(lastword $(MAKEFILE_LIST))

################################################################################
# Set and verify OCPI_CDK_DIR (unless RPM building where all bets are off)
ifndef OCPI_CDK_DIR
  # Remember to use abs path here, not real path, to preserve links for UI
  export OCPI_CDK_DIR:= $(abspath $(dir $(abspath $(OcpiThisFile)))/..)
else
  # Just to be sure it is exported
  export OCPI_CDK_DIR
endif
ifndef RPM_BUILD_ROOT
# We run the OCPI_CDK_DIR through the shell to handle ~ (at least).
OCPI_CDK_DIR:=$(shell echo $(OCPI_CDK_DIR))
ifeq ($(realpath $(OCPI_CDK_DIR)),)
  $(error The OCPI_CDK_DIR variable, "$(OCPI_CDK_DIR)", points to a nonexistent directory)
endif
ifneq ($(realpath $(OCPI_CDK_DIR)/include/ocpisetup.mk),$(realpath $(OcpiThisFile)))
  $(error Inconsistent usage of this file ($(OcpiThisFile)->$(realpath $(OcpiThisFile))) vs. OCPI_CDK_DIR ($(realpath $(OCPI_CDK_DIR)/include/ocpisetup.mk)))
endif
$(info OCPI_CDK_DIR has been set to $(OCPI_CDK_DIR) and verified to be sane.)
endif
include $(OCPI_CDK_DIR)/include/util.mk
endif # The end of processing this file once - ifndef OCPISETUP_MK
################################################################################
# OCPI_CDK_DIR has been established and verified.
# Now complete the other aspects of environment setup.

# Defaults for target tools
CC = gcc
CXX = c++
LD = c++
CXXFLAGS=-g -Wall -D__STDC_LIMIT_MACROS -D__STDC_FORMAT_MACROS
ARSUFFIX=a

ifeq ($(wildcard $(OCPI_CDK_DIR)/include/autoconfig_import*),)
  # Native non-autotools/RPM CDK
  # Determine unset variables dynamically here, again with the goal of sane default
  # behavior with no environment requirements at all.
  # Setting just OCPI_TARGET_PLATFORM will do cross builds
  $(eval $(OcpiEnsureToolPlatform))
  ifdef OCPI_USE_TOOL_MODES
    # Determine OCPI_TOOL_MODE if it is not set already
    ifndef OCPI_TOOL_MODE
      $(foreach o,$(if $(filter 1,$OCPI_DEBUG),d,o),\
        $(foreach s,$(if $(filter 1,$(OCPI_DYNAMIC),d,s)),\
          $(if $(wildcard $(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_HOST)/$o$s/ocpirun),\
             $(eval OCPI_TOOL_MODE:=$o$s)\
             $(info Using tool mode $(OCPI_TOOL_MODE)))))
      ifndef OCPI_TOOL_MODE
        $(foreach i,sd so dd do,\
          $(if $(OCPI_TOOL_MODE),,\
             $(if $(wildcard $(OCPI_CDK_DIR)/bin/$(OCPI_TOOL_HOST)/$i/ocpirun),\
                $(eval OCPI_TOOL_MODE:=$i)\
                $(info Choosing tool mode "$i" since there are tool executables for it.))))
        ifndef OCPI_TOOL_MODE
          $(warning No tools found to determine or verify tool mode.)
          override export OCPI_USE_TOOL_MODE=
        endif
      endif
    endif
    export OCPI_TOOL_DIR:=$(OCPI_TOOL_HOST)/$(OCPI_TOOL_MODE)
  else
    export OCPI_TOOL_DIR:=$(OCPI_TOOL_HOST)
  endif
  p:=$(OCPI_CDK_DIR)/platforms/$(OCPI_TOOL_PLATFORM)
  f:=$p/$(OCPI_TOOL_PLATFORM)-tool.mk
  ifeq ($(wildcard $f),)
#    $(warning There is no tool setup file ($f) for platform $(OCPI_TOOL_PLATFORM).  This may be ok.)
  else
   include $f
  endif
  ifndef OCPI_TARGET_PLATFORM
    export OCPI_TARGET_PLATFORM:=$(OCPI_TOOL_PLATFORM)
  endif
  ifndef OCPI_TARGET_HOST
    f=$(OCPI_CDK_DIR)/platforms/$(OCPI_TARGET_PLATFORM)/target
    ifeq ($(wildcard $f),)
      $(error OCPI_TARGET_PLATFORM is $(OCPI_TARGET_PLATFORM).  File $f is missing.)
    endif
    t:=$(shell cat $f)
    export OCPI_TARGET_HOST:=$t
    t:=$(subst -, ,$t)
    export OCPI_TARGET_OS:=$(word 1,$t)
    export OCPI_TARGET_OS_VERSION:=$(word 2,$t)
    export OCPI_TARGET_ARCH:=$(word 3,$t)
    $(infox OCPI_TARGET_HOST:$(OCPI_TARGET_HOST))
    $(infox OCPI_TARGET_OS:$(OCPI_TARGET_OS))
    $(infox OCPI_TARGET_OS_VERSION:$(OCPI_TARGET_OS_VERSION))
    $(infox OCPI_TARGET_ARCH:$(OCPI_TARGET_ARCH))
  endif
else
  # RPM-based options:
  -include $(OCPI_CDK_DIR)/include/autoconfig_import-$(OCPI_TARGET_PLATFORM).mk
  ifneq (1,$(OCPI_AUTOCONFIG_IMPORTED))
  -include $(OCPI_CDK_DIR)/include/autoconfig_import.mk
  endif

endif

################################################################################
# Run the target-specific make setup script
ifndef RPM_BUILD_ROOT
  p:=$(OCPI_CDK_DIR)/platforms/$(OCPI_TARGET_PLATFORM)
else
  p:=exports/platforms/$(OCPI_TARGET_PLATFORM)
endif
# Note that this script has access to OCPI_TOOL_xxx if the settings vary by tool host
f:=$p/$(OCPI_TARGET_PLATFORM)-target.mk
ifeq ($(wildcard $f),)
  $(error There is no target setup file ($f) for platform $(OCPI_TARGET_PLATFORM).)
else
 include $f
# $(warning File $f included for the target platform.)
endif

################################################################################
# Set up OCPI_TARGET_DIR, perhaps using target modes
ifdef OCPI_DYNAMIC
 override OCPI_BUILD_SHARED_LIBRARIES:=$(OCPI_DYNAMIC)
else
 ifdef OCPI_BUILD_SHARED_LIBRARIES
   OCPI_DYNAMIC=$(OCPI_BUILD_SHARED_LIBRARIES)
  else
   OCPI_DYNAMIC:=0
   OCPI_BUILD_SHARED_LIBRARIES:=0
  endif
endif
export OCPI_BUILD_SHARED_LIBRARIES 
export OCPI_DYNAMIC
ifdef OCPI_TARGET_DIR
  # $(warning OCPI_TARGET_DIR is unexpectedly set.)
else
 ifdef OCPI_USE_TARGET_MODES
   export OCPI_TARGET_MODE:=$(if $(filter 1,$(OCPI_DYNAMIC)),d,s)$(if $(filter 1,$(OCPI_DEBUG)),d,o)
 endif
 export OCPI_TARGET_DIR=$(OCPI_TARGET_HOST)$(and $(OCPI_TARGET_MODE),/$(OCPI_TARGET_MODE))
endif

################################################################################
# Set up Prerequisites
ifndef OCPI_PREREQUISITES_DIR
  export OCPI_PREREQUISITES_DIR:=/opt/opencpi/prerequisites
endif

#FIXME  this registration should be somewhere else.
ifndef OCPI_PREREQUISITES_LIBS
  export OCPI_PREREQUISITES_LIBS:=lzma gmp
endif

# FIXME is this necessary?
ifndef OCPI_PREREQUISITES_INSTALL_DIR
  export OCPI_PREREQUISITES_INSTALL_DIR:=/opt/opencpi/prerequisites
endif
ifndef OCPI_LZMA_DIR
  export OCPI_LZMA_DIR=$(OCPI_PREREQUISITES_DIR)/lzma
endif
ifndef OCPI_GTEST_DIR
  export OCPI_GTEST_DIR=$(OCPI_PREREQUISITES_DIR)/gtest
endif
ifndef OCPI_PATCHELF_DIR
  export OCPI_PATCHELF_DIR=$(OCPI_PREREQUISITES_DIR)/patchelf
endif
################################################################################
# Figure out if we should have OPENCL support
ifeq ($(origin OCPI_HAVE_OPENCL),undefined)
  ifeq ($(OCPI_TARGET_HOST),$(OCPI_TOOL_HOST))
    OCPI_HAVE_OPENCL:=$(if $(realpath $(OCPI_BIN_DIR)/ocpiocl),$(shell $(OCPI_BIN_DIR)/ocpiocl test && echo 1),)
  endif
endif
################################################################################
# From here down is specifically for user makefiles
################################################################################
# Set up directory pointers for user makefiles
export OCPI_LIB_DIR:=$(OCPI_CDK_DIR)/lib/$(OCPI_TARGET_DIR)
export OCPI_BIN_DIR:=$(OCPI_CDK_DIR)/bin/$(OCPI_TARGET_DIR)
export OCPI_INC_DIR:=$(OCPI_CDK_DIR)/include/aci

# Which libraries should be made available to user executables?
export OCPI_API_LIBS=application container library transport rdma_driver_interface rdma_utils rdma_smb util  msg_driver_interface os

# This is appropriate for static linking.
# It forces the use of the static prerequisites libraries even if there is a dynamic one also.
ifneq ($(OCPI_DYNAMIC),1)
export OCPI_LD_FLAGS=\
  -L"$(OCPI_LIB_DIR)" $(OCPI_API_LIBS:%=-locpi_%) $(OCPI_EXTRA_LIBS:%=-l%) \
  $(foreach l,$(OCPI_PREREQUISITES_LIBS),\
    $(OCPI_PREREQUISITES_DIR)/$l/$(OCPI_TARGET_HOST)/lib/lib$l.$(ARSUFFIX))
else
# This is appropriate for dynamic linking using dynamic libraries
# It creates an executable that will execute in the developmen environment,
# i.e., looking for dynamic libraries where they will be here.
# For deployed environments the OCPI_TARGET_CDK_DIR can be overridden
# A given application can prepend to the LD_FLAGS with other rpath/origin stuff
ifndef OCPI_TARGET_CDK_DIR
OCPI_TARGET_CDK_DIR=$(OCPI_CDK_DIR)
endif
ifndef OCPI_TARGET_PREREQUISITES_DIR
OCPI_TARGET_PREREQUISITES_DIR=$(OCPI_PREREQUISITES_DIR)
endif
export OCPI_LD_FLAGS=\
  -L"$(OCPI_LIB_DIR)" $(OCPI_API_LIBS:%=-locpi_%) $(OCPI_EXTRA_LIBS:%=-l%) \
  $(foreach l,$(OCPI_PREREQUISITES_LIBS),\
    $(OCPI_TARGET_PREREQUISITES_DIR)/$l/$(OCPI_TARGET_DIR)/lib/lib$l.$(OcpiDynamicSuffix)) \
  -Xlinker -rpath -Xlinker $(OCPI_TARGET_CDK_DIR)/lib/$(OCPI_TARGET_DIR) \
  -Xlinker -rpath -Xlinker $(OCPI_TARGET_PREREQUISITES_DIR)/lib/$(OCPI_TARGET_DIR) \

endif


all:
$(OCPI_TARGET_DIR):
	mkdir -p $@

target-$(OCPI_TARGET_DIR):
	mkdir -p $@

