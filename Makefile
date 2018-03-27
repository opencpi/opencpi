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

ProjectPrefix=ocpi
ifndef OCPI_CDK_DIR
  export OCPI_CDK_DIR:=$(CURDIR)/cdk
endif

ifeq ($(wildcard exports),)
  ifeq ($(filter clean%,$(MAKECMDGOALS)),)
    $(info Exports have never been set up for this tree  Doing it now.)
  endif
  $(info $(shell ./scripts/makeExportLinks.sh - $(ProjectPrefix)_ - xxx))
endif
ifeq ($(filter clean%,$(MAKECMDGOALS)),)
include exports/include/ocpisetup.mk
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
ifneq ($(OCPI_TOOL_PLATFORM),$(OCPI_TARGET_PLATFORM))
  ifneq ($(shell test -x exports/bin/$(OCPI_TOOL_DIR)/ocpigen; echo $$?),0)
    $(info To build for $(OCPI_TARGET_PLATFORM), you must first build for $(OCPI_TOOL_PLATFORM))
    $(error Cannot build for $(OCPI_TARGET_PLATFORM), cannot find "ocpigen" for $(OCPI_TOOL_PLATFORM))
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

RDMA_DRIVERS=datagram dma ofed pio socket

#	 runtime/dataplane/rdma_drivers \

PACKAGES += os runtime/util

PACKAGES += \
	 runtime/dataplane/xfer/base \
	 runtime/dataplane/xfer/tests \
	 $(foreach d,$(RDMA_DRIVERS),runtime/dataplane/xfer/drivers/$d) \
	 runtime/dataplane/transport \
	 runtime/dataplane/msg_driver_interface \
	 runtime/dataplane/msg_drivers \
	 runtime/library \
	 runtime/container \
	 runtime/remote \
	 runtime/remote-support \
	 runtime/rcc \
	 runtime/ocl \
	 runtime/ocl-support \
	 runtime/hdl \
         runtime/application \
	 runtime/hdl-support \
	 runtime/ctests

PACKAGES += tools/cdkutils
PACKAGES += tools/ocpigen
PACKAGES += tools/ocpisca
#PACKAGES += tools/ocpirh
PACKAGES += tools/ocpigr
PACKAGES += tools/ocpidds
#PACKAGES += tools/astyle
PACKAGES += tests

#
# ----------------------------------------------------------------------
# A list of all packages for make clean.  Sorted alphabetically.
# ----------------------------------------------------------------------
#

#	runtime/control/wci_api
ALLPACKAGES = \
	os \
	runtime/util \
	runtime/dataplane/xfer/base \
	runtime/dataplane/xfer/tests \
	runtime/dataplane/xfer/drivers \
	runtime/dataplane/transport \
	runtime/dataplane/msg_driver_interface \
	runtime/dataplane/msg_drivers \
	runtime/library \
	runtime/container \
	runtime/remote \
	runtime/remote-support \
	runtime/rcc \
	runtime/ocl \
	runtime/ocl-support \
	runtime/ctests \
	runtime/hdl \
	runtime/hdl-support \
        runtime/application \
	tools/cdkutils \
	tools/ocpigen \
	tools/ocpisca \
	tools/ocpigr \
	tools/ocpidds \
	tests \

#	tools/ocpirh \
#	tools/astyle \

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
	$(MAKE) -C projects/core cleanhdl && $(MAKE) -C projects/assets cleanhdl

#	$(MAKE) -C components cleanhdl this happens in the hdl subdir (using ../components)

rcc ocl hdl: exports

rcc:
	make -C projects/core/components rcc && make -C projects/assets/components rcc


cleanrcc:
	make -C projects/core/components cleanrcc && make -C projects/assets/components rcc

ocl:
	make -C projects/core/components ocl && make -C projects/assets/components ocl

cleanocl:
	make -C projects/core/components cleanocl && make -C projects/assets/components cleanocl

.PHONY : examples
examples: exports
	$(MAKE) -C projects/assets/applications

cleanexamples:
	make -C projects/assets/applications clean

runexamples:
	make -C projects/assets/applications run

runtests:
	make -C tests run

cleancomponents:
	make -C projects/core/components clean && make -C projects/assets/components clean

.PHONY: prims
hdlprimitives: exports
	$(MAKE) -C hdl primitives

driver:
	$(AT)set -e; if test -d os/$(OCPI_TARGET_OS)/driver; then \
	  $(MAKE) -C os/$(OCPI_TARGET_OS)/driver; \
	  $(MAKE) exports; \
	else \
	  echo No driver for the OS '"'$(OCPI_TARGET_OS)'"', so none built. ; \
	fi

cleandriver:
	$(AT)$(and $(wildcard os/$(OCPI_TARGET_OS)/driver),$(MAKE) -C os/$(OCPI_TARGET_OS)/driver topclean)

cleandrivers:
	for d in os/*/driver; do $(MAKE) -C $$d topclean; done

.PHONY: packages tar diff diff.q test $(PACKAGES)

everything: packages rcc hdl ocl
compile build: $(PACKAGES)
packages: $(PACKAGES)

$(PACKAGES):
	$(AT)if test -f $@/Makefile.ocpi ; then \
		$(MAKE) $(call DescendMake,$@) -f Makefile.ocpi ; \
	else \
		$(MAKE) $(call DescendMake,$@) -f $(call AdjustRelative,$@,)/Makefile.ocpi.for-pkg ; \
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
	$(AT)make -C projects/core clean && make -C projects/assets clean
	$(AT)rm -r -f exports

cleanexports:
	$(AT)rm -r -f exports

distclean: clean
	find . -name '*~' -exec rm {} \;
	find . -depth -name '*.dSym' -exec rm -r {} \;
	-rm -f diff diff.q
	-rm -f *.exe *.obj *.o *.ilk *.sbr *.suo *.sln *.pdb *.bsc *~
	-rm -r -f lib

cleaneverything: distclean cleandrivers
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
	runtime/dataplane/xfer

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
