# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile there then includes this
# Makefile once again.  This is the expected mode of operation.
# FIXME: add dependencies on the actual kernel stuff so when we update
# a kernel the rebuild is automatic?  Perhaps via "deps"?

# This conditional selects whether we are being included from the
# kernel Makefile or not. (this in-kernel building is not regularly tested)
ifdef KERNELRELEASE
  # called from kernel build system: just declare what our modules are
  obj-m := opencpi.o
else
ifdef OCPI_KERNEL_DIR
  KERNELDIR := $(OCPI_KERNEL_DIR)
  ifeq ($(wildcard $(KERNELDIR)),)
    $(error OCPI_KERNEL_DIR indicates '$(OCPI_KERNEL_DIR)', which does not exist)
  endif
  ifdef OCPI_CROSS_BUILD_BIN_DIR
    override CROSS_COMPILE:=$(OCPI_CROSS_BUILD_BIN_DIR)/$(OCPI_CROSS_HOST)-
    override ARCH:=$(OCPI_ARCH)
    export CROSS_COMPILE
    export ARCH
  endif
  $(info OCPI_KERNEL_DIR was specified, we are now using: KERNELDIR=$(KERNELDIR) ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE))
else
  ifdef OCPI_CROSS_BUILD_BIN_DIR
   $(error OCPI_CROSS_BUILD_BIN_DIR is set, but OCPI_KERNEL_DIR is not set, which is not supported.)
  endif
  # Assume the source tree is where the running kernel was built
  # You should set KERNELDIR in the environment if it's elsewhere
  ifeq ($(filter clean topclean,$(MAKECMDGOALS)),)
    KERNELDIR ?= /lib/modules/$(shell uname -r)/build
    ifeq ($(wildcard $(KERNELDIR)),)
      $(error Cannot figure out where the KERNELDIR should be.)
    endif
    $(info No OCPI_KERNEL_DIR specified, so using: KERNELDIR=$(KERNELDIR))
  endif
endif
# The current directory is passed to sub-makes as argument
PWD := $(shell pwd)

MODNAME = opencpi

.DELETE_ON_ERROR:
MyDir:=$(dir $(realpath $(lastword $(MAKEFILE_LIST))))
MyIncs= \
         -I$(realpath $(MyDir)/../../../../../core/container/hdl/include) \
         -I$(realpath $(MyDir)/../../interfaces/include)
MyLib:=$(realpath $(MyDir)/../../../../../lib/target-$(OCPI_TARGET_HOST))
ifeq ($(strip $(MyLib)),)
  ifneq ($(MAKECMDGOALS),topclean)
    $(error OpenCpi libraries must be built before the driver is built)
  endif
endif
# This is what works with RHEL5
export EXTRA_CFLAGS := $(MyIncs) -Wformat=2
# This is what works with Centos6
export KCPPFLAGS := $(MyIncs) -Wformat=2
MODFILE=$(MODNAME).ko
URULESFILE=99-$(MODNAME).rules
MRULESFILE=mdev-$(MODNAME).rules
JUNK=Module.markers Module.symvers .opencpi* opencpi.mod.* opencpi.o .tmp*
INSTALLED=$(MyLib)/$(MODFILE) $(MyLib)/$(URULESFILE) $(MyLib)/$(MRULESFILE)
all:	$(INSTALLED)

$(MODFILE): opencpi.c
	$(and $(KERNELDIR),$(MAKE) -C $(KERNELDIR) M=$(PWD) modules)
	@rm -rf	$(JUNK)

modules: $(MODFILE)

$(INSTALLED): $(MODFILE) $(URULESFILE) $(MRULESFILE)
	@cp $(URULESFILE) $(MyLib)
	@cp $(MRULESFILE) $(MyLib)
	@cp $(MODFILE) $(MyLib)
	@echo Linux driver built, and a copy placed in $(MyLib)


modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

load:
	@./ocpi_linux_driver reload x

unload:
	@./ocpi_linux_driver unload x

clean:	
	$(and $(KERNELDIR),$(MAKE) -C $(KERNELDIR) M=$(PWD) clean)
	rm -rf	$(JUNK) $(MODFILE) $(MODFILE).unsigned modules.order

topclean: clean
	rm -f $(MyLib)/$(MODFILE) $(MyLib)/$(URULESFILE) $(MyLib)/$(MRULESFILE)

.PHONY: all clean topclean

endif
