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
ThisMake := $(lastword $(MAKEFILE_LIST))

# We have two situations for building the linux kernel driver.
# 1. One is simply part of the source tree, where building the driver happens in a CDK environment
# where all the software platforms are defined in projects etc.
# This can target the native system or a cross-build with accessible kernel headers.
# 2. The other is when we are running in an installed standalone driver package where the driver
# is rebuilt based on the kernel installed on the system.  In this situation we want to be
# independent from the rest of OpenCPI so that it can be the minimal thing that is *not*
# sharable on a network-shared CDK etc. where everything else is.
# This is clearly only a native build.

# This Makefile decides that it is "standalone" (#2) simply by the absence of the
# OCPI_TARGET_PLATFORM, meaning it is not in the framework build environment,
# and thus cannot get the platform description from a project-defined software platform, and
# thus must be a native build.
ifdef OCPI_TARGET_PLATFORM
 $(info Building driver in a CDK environment)
 # Get the info about the target platform from the CDK
 include $(OCPI_CDK_DIR)/include/setup-target-platform.mk
else
 $(info Building driver in a standalone environment)
 # Set the variables that we use to constant values we know for self-hosted platforms
 # If this isn't good enough we can easily cache them in the RPM.
 OcpiCrossCompile=
 OcpiCC=gcc 
 OcpiCXX=g++
 OCPI_TARGET_DIR=$(OCPI_TOOL_DIR)
endif
# Set the global make variables that the kernel Makefiles want to see set in global variables
CC=$(OcpiCrossCompile)$(OcpiCC)
CXX=$(OcpiCrossCompile)$(OcpiCXX)
LD=$(OcpiCrossCompile)ld

.PHONY: all

# AV-1571 - map arm_cs => arm
ifeq ($(OCPI_TARGET_ARCH),arm_cs)
  override OCPI_TARGET_ARCH=arm
endif

# (No longer blocks, but code left in as example if we need to block other platforms/drivers)
#    ifeq ($(OCPI_CROSS_HOST),arm-none-linux-gnueabi)
#      $(warning Driver support not available for this platform (yet))
#      .DEFAULT_GOAL := platform_not_supported
#      all: platform_not_supported
#    else
all: installed
#    endif

# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile there then includes this
# Makefile once again.  This is the expected mode of operation.
# FIXME: add dependencies on the actual kernel stuff so when we update
# a kernel the rebuild is automatic?  Perhaps via "deps"?

AT?=@
# This conditional selects whether we are being included from the
# kernel Makefile or not. (this in-kernel building is not regularly tested)
ifdef KERNELRELEASE
  # called from kernel build system: just declare what our modules are
  obj-m := opencpi.o
else
ifdef OCPI_TARGET_KERNEL_DIR
  KERNELDIR := $(OCPI_TARGET_KERNEL_DIR)
  ifeq ($(wildcard $(KERNELDIR)),)
    $(error OCPI_TARGET_KERNEL_DIR indicates '$(OCPI_TARGET_KERNEL_DIR)', which does not exist)
  endif
  ifdef OcpiCrossCompile
    override CROSS_COMPILE:=$(OcpiCrossCompile)
    override ARCH:=$(OCPI_TARGET_ARCH)
    export CROSS_COMPILE
    export ARCH
  endif
  $(info OCPI_TARGET_KERNEL_DIR was specified, we are now using: KERNELDIR=$(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE))
else
  ifdef OcpiCrossCompile
    $(error This is a cross-compiled platform, but OCPI_TARGET_KERNEL_DIR is not set, which is not supported.)
  endif

  ifeq ($(filter clean topclean,$(MAKECMDGOALS)),)
    # Find the kernel headers - they can be found:
    #  1. Specified KERNELDIR (always chosen if given)
    #  2. /usr/src/kernels/(uname -r)
    #  3. /usr/src/kernels/(specific RPM-specified location based on the kernel-headers package)
    OcpiKernelRelease:=$(shell uname -r)
    # this command should be platform-specific (e.g. different for debian etc.)
    OcpiKernelHeadersVersion:=$(shell rpm -q --qf=%{version}-%{release}.%{arch} kernel-headers)
    $(info --- The running kernel is release: $(OcpiKernelRelease))
    $(info --- The current kernel-headers package is version: $(OcpiKernelHeadersVersion))
    ifdef KERNELDIR
      $(info --- The requested kernel header location (via KERNELDIR) is: $(KERNELDIR))
      ifeq ($(wildcard $(KERNELDIR)),)
        $(error The requested kernel header location ($(KERNELDIR)) is not found)
      endif
    else
      KERNELDIR=/usr/src/kernels/$(OcpiKernelRelease)
      ifeq ($(wildcard $(KERNELDIR)),)
        try1:=$(KERNELDIR)
        KERNELDIR=/usr/src/kernels/$(OcpiKernelHeadersVersion)
        ifneq ($(wildcard $(KERNELDIR)),)
          $(info --- Using the kernel headers version at: $(KERNELDIR))
        else
          $(info XXX: $(shell ls -l /lib/modules ))
          $(error Cannot find a kernel headers directory.  Tried $(try1) and $(KERNELDIR))
        endif
      else
        $(info --- Using the kernel release at: $(KERNELDIR))
      endif
    endif
    $(info No OCPI_TARGET_KERNEL_DIR specified, so using: KERNELDIR=$(KERNELDIR))
  endif
endif
ocpi_release:=$(shell cat $(OCPI_TARGET_KERNEL_DIR)/ocpi-release 2>/dev/null)
# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)

MODNAME = opencpi

.DELETE_ON_ERROR:
MyDir:=$(dir $(realpath $(ThisMake)))
MyTargetDir?=target-$(OCPI_TARGET_DIR)
MyIncs?= \
         -I$(realpath $(MyDir)/../../../runtime/hdl/include) \
         -I$(realpath $(MyDir)/../../interfaces/include)
MyLib:=$(realpath $(OCPI_CDK_DIR)/$(OCPI_TARGET_DIR)/lib)
$(infox x:$(MyTargetDir) $(MyDir))
ifneq ($(MAKECMDGOALS),topclean)
  ifdef OCPI_TARGET_PLATFORM
    ifeq ($(strip $(MyLib)),)
      $(error OpenCPI libraries must be built before the driver is built)
    endif
  endif
endif
# This is what works with RHEL5
export EXTRA_CFLAGS := $(MyIncs) -Wformat=2
# This is what works with Centos6
export KCPPFLAGS := $(MyIncs) -Wformat=2
MODFILE=$(MyTargetDir)/$(MODNAME)$(if $(ocpi_release),-$(ocpi_release)).ko
URULESFILE=99-$(MODNAME).rules
MRULESFILE=mdev-$(MODNAME).rules
TMP_FILES=Module.markers Module.symvers .opencpi* opencpi.mod.* opencpi.o .tmp* modules.order

$(MODFILE): opencpi.c $(shell echo $$(cd $(KERNELDIR); pwd -P)) $(KERNELDIR)/include/config/auto.conf
	$(AT)echo Building kernel module $(MODFILE)
	$(and $(KERNELDIR),$(MAKE) -C $(KERNELDIR) M=$(PWD) V=2 modules)
	$(AT)mkdir -p $(MyTargetDir)
	$(AT)mv $(MODNAME).ko $(MODFILE)
	$(AT)rm -rf $(TMP_FILES)

.PHONY: installed modules modules_install load unload
modules: $(MODFILE)

installed: $(MODFILE) $(URULESFILE) $(MRULESFILE)
	$(AT)echo Linux driver built, and available in $(MyTargetDir)

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

load:
	$(AT)./ocpi_linux_driver reload x

unload:
	$(AT)./ocpi_linux_driver unload x

.PHONY: all clean topclean distclean
# This clean is target specific
clean:
ifneq ($(KERNELDIR),)
	$(AT)find $(KERNELDIR)/scripts | xargs file | grep "ELF 64-bit" | cut -f1 -d: | xargs -r -n1 rm
	$(AT)rm -f $(KERNELDIR)/include/config/auto.conf
	$(MAKE) -C $(KERNELDIR) M=$(PWD) clean
endif
	$(AT)rm -rf $(TMP_FILES) $(MODFILE) $(MODFILE).unsigned
ifneq ($(MyTargetDir),.)
	$(AT)rm -rf $(MyTargetDir)
endif

# This clean is target specific
topclean: clean
	rm -f $(MyLib)/$(MODFILE) $(MyLib)/$(URULESFILE) $(MyLib)/$(MRULESFILE)

# This clean will clean locally for all targets
distclean: clean
	rm -r -f target-*

.PHONY: platform_not_supported
.SILENT: platform_not_supported

platform_not_supported:
	true

# This file may be generated from an imported .config file.
# In centos6/7 kernel headers it is already part of the yum-maintained kernel-headers package
# and thus will not ever need to be built this way.
# So this rule is saying that for some kernels, you can introduce a new or missing .config
# file and generate the needed auto.conf file.
# It is a bit scary to go into another package and cause changes there, but this simplifies
# the process by just causing the right thing to happen as a side-effect of building our driver.
# This rule also allows the local host tools (e.g. /scripts/basic/fixdep) to be compiled, if required
.SILENT: $(KERNELDIR)/include/config/auto.conf
$(KERNELDIR)/include/config/auto.conf:
	# make -C $(KERNELDIR) clean V=2 ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	# make -C $(KERNELDIR) oldconfig V=2 ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	# make -C $(KERNELDIR) prepare V=2 ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	# make -C $(KERNELDIR) V=2 ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	# make -C $(KERNELDIR) modules V=2 ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
	make -C $(KERNELDIR) modules_prepare V=2 ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE)
endif # ifdef KERNELRELEASE else..
