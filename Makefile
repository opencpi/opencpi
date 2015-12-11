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

ProjectPrefix=ocpi
ifeq ($(wildcard exports),)
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    $(info Exports have never been set up for this tree  Doing it now.)
  endif
  $(info $(shell ./scripts/makeExportLinks.sh - $(ProjectPrefix)_ xxx))
endif
export OCPI_CFLAGS=-Wall -Wfloat-equal -Wextra  -fno-strict-aliasing -Wconversion -std=c99
export OCPI_CXXFLAGS=-Wextra -Wall -Wfloat-equal -fno-strict-aliasing -Wconversion
include exports/include/ocpisetup.mk
export OCPI_OS=$(OCPI_TARGET_OS)
ifneq ($(OCPI_OS),)
SYSTEMOPTION="OCPI_OS=$(OCPI_OS)"
endif

# defaults
ifndef OCPI_BASE_DIR
export OCPI_BASE_DIR := .
endif
OCPI_ABS_DIR := $(shell pwd|sed 's/ /\\ /g')
CLIENT_IDL_ONLY := 1
LD_LIBRARY_PATH := $(LD_LIBRARY_PATH):$(OCPI_ABS_DIR)/lib
export LD_LIBRARY_PATH
export CLIENT_IDL_ONLY
export OCPI_BASE_DIR
include MakeVars.ocpi

ifndef OCPI_TOOL_HOST
$(error This makefile expects the OCPI_TOOL_HOST environment variable to be set.)
endif
ifndef OCPI_TARGET_HOST
$(error This makefile expects the OCPI_TARGET_HOST environment variable to be set.)
endif
ifneq ($(OCPI_TOOL_HOST),$(OCPI_TARGET_HOST))
  ifneq ($(shell test -x ocpi/bin/$(OCPI_TOOL_HOST)/ocpigen; echo $$?),0)
    $(info To build for $(OCPI_TARGET_HOST), you must first build for $(OCPI_TOOL_HOST))
    $(error Cannot build for $(OCPI_TARGET_HOST), cannot find "ocpigen" for $(OCPI_TOOL_HOST))
  endif
endif
#
# ----------------------------------------------------------------------
# Build up a list of $(PACKAGES) to build.  This list is carefully
# ordered according to package dependencies.
# ----------------------------------------------------------------------
#

#
# Basic packages.
#

RDMA_DRIVERS=datagram dma ofed pio sockets udp

#	 runtime/dataplane/rdma_drivers \

PACKAGES += os runtime/util

PACKAGES += \
	 runtime/dataplane/rdma_utils \
	 runtime/dataplane/rdma_driver_interface \
	 runtime/dataplane/rdma_smb \
	 $(foreach d,$(RDMA_DRIVERS),runtime/dataplane/rdma_drivers/$d) \
	 runtime/dataplane/transport \
	 runtime/dataplane/msg_driver_interface \
	 runtime/dataplane/msg_drivers \
	 runtime/dataplane/rdma_tests \
	 runtime/library \
	 runtime/container \
	 runtime/hdl \
	 runtime/hdl-support \
	 runtime/ocl \
	 runtime/rcc \
	 runtime/remote \
	 runtime/ctests \
         runtime/application


PACKAGES += tools/cdkutils
PACKAGES += tools/ocpigen
PACKAGES += tools/cdk/ocpidds
PACKAGES += tools/astyle
# if we don't have opencl and we are building statically, don't bother with this
#ifeq ($(OCPI_BUILD_SHARED_LIBRARIES),1)
#PACKAGES += tools/cdk/ocpiocl
#else
#ifeq ($(OCPI_HAVE_OPENCL),1)
#PACKAGES += tools/cdk/ocpiocl
#endif
#endif

PACKAGES += tests

#PACKAGES += tests/test_logger
#PACKAGES += tests/bin

#
# ----------------------------------------------------------------------
# A list of all packages for make clean.  Sorted alphabetically.
# ----------------------------------------------------------------------
#

#	runtime/control/wci_api
ALLPACKAGES = \
	os \
	runtime/util \
	runtime/dataplane/rdma_utils \
	runtime/dataplane/rdma_driver_interface \
	runtime/dataplane/rdma_smb \
	runtime/dataplane/rdma_drivers \
	runtime/dataplane/transport \
	runtime/dataplane/msg_driver_interface \
	runtime/dataplane/msg_drivers \
	runtime/dataplane/rdma_tests \
	runtime/library \
	runtime/container \
	runtime/hdl \
	runtime/hdl-support \
	runtime/ocl \
	runtime/rcc \
	runtime/remote \
	runtime/ctests \
        runtime/application \
	tools/cdkutils \
	tools/ocpigen \
	tools/cdk/ocpidds \
	tools/astyle \
	tests \

#
# ----------------------------------------------------------------------
# Rules.
# ----------------------------------------------------------------------
#

all: packages exports
.PHONY: exports


exports:
	$(AT)./scripts/makeExportLinks.sh $(OCPI_TARGET_DIR) $(ProjectPrefix)_

.PHONY: hdl hdlcomps hdlapps hdlclean
hdlcomps:
	$(MAKE) -C hdl components

hdlapps: hdlcomps hdl
	$(MAKE) -C hdl assemblies

hdl:
	$(MAKE) -C hdl

hdlbase:
	$(MAKE) -C hdl base

hdlportable:
	$(MAKE) -C hdl portable

cleanhdl:
	$(MAKE) -C hdl clean

#	$(MAKE) -C components cleanhdl this happens in the hdl subdir (using ../components)

rcc ocl hdl: exports

rcc:
	make -C components rcc

cleanrcc:
	make -C components cleanrcc

ocl:
	make -C components ocl

cleanocl:
	make -C components cleanocl

.PHONY : examples
examples: exports
	make -C examples

cleanexamples:
	make -C examples clean

runexamples:
	make -C examples run

runtests:
	make -C tests run

cleancomponents:
	make -C components clean

.PHONY: prims
hdlprimitives: exports
	$(MAKE) -C hdl primitives

driver:
	$(AT)if test -d os/$(OCPI_OS)/driver; then \
	  $(MAKE) -C os/$(OCPI_OS)/driver; \
	else \
	  echo No driver for the OS '"'$(OCPI_OS)'"', so none built. ; \
	fi

cleandriver:
	$(AT)$(and $(wildcard os/$(OCPI_OS)/driver),$(MAKE) -C os/$(OCPI_OS)/driver topclean)

cleandrivers:
	for d in os/*/driver; do $(MAKE) -C $$d topclean; done

.PHONY: packages tar diff diff.q test $(PACKAGES)

everything: packages rcc hdl
compile build: $(PACKAGES)
packages: $(PACKAGES)

$(PACKAGES):
	$(AT)if test -f $@/Makefile.ocpi ; then \
		$(MAKE) $(call DescendMake,$@) $(SYSTEMOPTION) -f Makefile.ocpi ; \
	else \
		$(MAKE) $(call DescendMake,$@) $(SYSTEMOPTION) -f $(call AdjustRelative,$@,)/Makefile.ocpi.for-pkg ; \
	fi

clean: cleancomponents cleanexamples
	$(AT)rm -r -f lib
	$(AT)$(foreach p,$(ALLPACKAGES),\
		if test -f $p/Makefile.ocpi ; then \
			$(MAKE) --no-print-directory $(call DescendMake,$p) -f Makefile.ocpi $@ ; \
		else \
			$(MAKE) --no-print-directory $(call DescendMake,$p) -f $(call AdjustRelative,$p,)/Makefile.ocpi.for-pkg $@ ; \
		fi ; \
	)

cleanexports:
	$(AT)rm -r -f exports

distclean: clean
	find . -name '*~' -exec rm {} \;
	find . -depth -name '*.dSym' -exec rm -r {} \;
	-rm -f diff diff.q
	-rm -f *.exe *.obj *.o *.ilk *.sbr *.suo *.sln *.pdb *.bsc *~
	-rm -r -f lib

cleaneverything: distclean cleandrivers
	make -C components cleanhdl
	-find . -depth -name 'timeData.raw' -exec rm -f '{}' ';'
	-find . -depth -name 'target-*' -exec rm -r '{}' ';'
	-find . -depth -name 'gen' -exec rm -r '{}' ';'
	-find . -depth -name "lib" -a ! -path "*export*" -a ! -path "*/platforms/*" -a -type d -a -exec rm -r "{}" ";"
	$(AT)rm -r -f exports

tar:
	tar cvf ocpi.tar Makefile MakeVars.ocpi Makefile.ocpi.for-* scripts platforms $(ALLPACKAGES)
	gzip -9 ocpi.tar

#
# Shallow package dependencies.
#

runtime/dataplane/tests: \
	runtime/rcc runtime/dataplane/transport \
	runtime/dataplane/rdma_driver_interface

export_cdk:
	mydate=`date +%G%m%d%H%M%S`; \
	file=opencpi-cdk-$$mydate.tgz; \
	(cd exports; find . -follow -type l) > $$file.exclude; \
	if [ -s $$file.exclude ] ; then \
	  echo ==== These symlinks are broken for this export: ; \
	  cat $$file.exclude ; \
	  echo ==== End of broken symlinks; \
	fi; \
	echo Creating export file: $$file; \
	tar -z -h -f $$file -c -X $$file.exclude -C exports .


etags: 
    ETAGS=etags ; \
    rm TAGS \
    find . -name '*.cpp' -o -name '*.h' -o -name '*.c' -print0 \
    | xargs $(ETAGS) --extra=+q --fields=+fksaiS --c++-kinds=+px --append
