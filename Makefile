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

PACKAGES += adapt/os/ocpios core/util

PACKAGES += \
	 core/dataplane/rdma_utils \
	 core/dataplane/rdma_driver_interface \
	 core/dataplane/rdma_smb \
	 core/dataplane/rdma_drivers \
	 core/dataplane/transport \
	 core/dataplane/msg_driver_interface \
	 core/dataplane/msg_drivers \
	 core/dataplane/rdma_tests \
	 core/library \
	 core/container \
	 core/hdl \
	 core/rcc \
	 core/ocl \
	 core/remote \
	 core/ctests \
         core/application

ifeq ($(OCPI_HAVE_CORBA),1)
PACKAGES += core/corba/orb_services core/corba/corba_util
PACKAGES += core/sca/cf core/sca/cf_util core/sca/sgac
PACKAGES += core/sca/gpped
PACKAGES += core/sca/srpl
endif

PACKAGES += tools/local/binder tools/local/tester
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
PACKAGES += tools/cdk/ocpixm
PACKAGES += test

PACKAGES += tests

#PACKAGES += tests/test_logger
#PACKAGES += tests/bin

#
# ----------------------------------------------------------------------
# A list of all packages for make clean.  Sorted alphabetically.
# ----------------------------------------------------------------------
#

#	core/control/wci_api
ALLPACKAGES = \
	adapt/os/ocpios \
	core/util \
	core/corba/corba_util \
	core/corba/orb_services \
	core/dataplane/rdma_utils \
	core/dataplane/rdma_driver_interface \
	core/dataplane/rdma_smb \
	core/dataplane/rdma_drivers \
	core/dataplane/transport \
	core/dataplane/msg_driver_interface \
	core/dataplane/msg_drivers \
	core/dataplane/rdma_tests \
	core/library \
	core/container \
	core/hdl \
	core/ocl \
	core/rcc \
	core/remote \
	core/ctests \
        core/application \
	core/sca/cf \
	core/sca/cf_util \
	core/sca/gpped \
	core/sca/sgac \
	core/sca/srpl \
	tools/local/binder \
	tools/local/tester \
	tools/cdkutils \
	tools/ocpigen \
	tools/cdk/ocpidds \
	tools/cdk/ocpixm \
	tools/astyle \
	tests \
	test \

#
# ----------------------------------------------------------------------
# Rules.
# ----------------------------------------------------------------------
#

all: packages

.PHONY: hdl hdlcomps hdlapps hdlclean
hdlcomps:
	$(MAKE) -C hdl components

hdlapps: hdlcomps hdl
	$(MAKE) -C hdl assemblies

hdl:
	$(MAKE) -C hdl

hdlbase:
	$(MAKE) -C hdl base

cleanhdl:
	$(MAKE) -C hdl clean

#	$(MAKE) -C components cleanhdl this happens in the hdl subdir (using ../components)

rcc:
	make -C components rcc

cleanrcc:
	make -C components cleanrcc

ocl:
	make -C components ocl

cleanocl:
	make -C components cleanocl

examples:
	make -C tools/cdk/examples

cleanexamples:
	make -C tools/cdk/examples clean

runexamples:
	make -C tools/cdk/examples run

runtests:
	make -C tests run

cleancomponents:
	make -C components clean

.PHONY: prims
hdlprims:
	$(MAKE) -C hdl primitives

driver:
	$(MAKE) -C adapt/os/ocpios/$(OCPI_OS)/driver

cleandriver:
	$(AT)$(and $(wildcard adapt/os/ocpios/$(OCPI_OS)/driver),$(MAKE) -C adapt/os/ocpios/$(OCPI_OS)/driver topclean)

cleandrivers:
	for d in adapt/os/ocpios/*/driver; do $(MAKE) -C $$d topclean; done

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
	-find . -depth -name "lib" -a ! -path "*export*" -a -type d -a -exec rm -r "{}" ";"

tar:
	tar cvf ocpi.tar Makefile MakeVars.ocpi Makefile.ocpi.for-* scripts platforms $(ALLPACKAGES)
	gzip -9 ocpi.tar

#
# Note: the "sed" command does not work on Solaris; its sed does not
# recognize "\t" as a tab. Works fine on Linux and Cygwin.
#

diff:
	-diff -r -c $(VOBDIR) . > $@
	-grep "^--- \./" diff | sed -e 's/^--- \(\.\/.*\)\t.*$$/\1/' > diff.q

diff.q:
	-grep "^--- \./" diff | sed -e 's/^--- \(\.\/.*\)\t.*$$/\1/' > diff.q

#
# Shallow package dependencies.
#

#core/local/logger: adapt/os/ocpios
#core/util: core/local/logger
core/dataplane/tests: \
	core/rcc core/dataplane/transport \
	core/dataplane/rdma_driver_interface

core/corba/corba_util: core/corba/orb_services core/util
core/sca/cf_util: core/sca/cf core/corba/corba_util core/corba/orb_services \
	core/util
core/sca/sgac: core/sca/cf_util
core/sca/gpped: core/sca/cf_util
tools/local/binder: core/util
tools/local/tester: \
	core/rcc core/dataplane/transport \
	core/dataplane/rdma_driver_interface
test: tools/local/binder tools/local/tester

export_cdk:
	mydate=`date +%G%m%d%H%M%S`; \
	file=opencpi-cdk-$$mydate.tgz; \
	(cd tools/cdk/export; find . -follow -type l) > $$file.exclude; \
	if [ -s $$file.exclude ] ; then \
	  echo ==== These symlinks are broken for this export: ; \
	  cat $$file.exclude ; \
	  echo ==== End of broken symlinks; \
	fi; \
	echo Creating export file: $$file; \
	tar -z -h -f $$file -c -X $$file.exclude -C tools/cdk/export .


etags: 
    ETAGS=etags ; \
    rm TAGS \
    find . -name '*.cpp' -o -name '*.h' -o -name '*.c' -print0 \
    | xargs $(ETAGS) --extra=+q --fields=+fksaiS --c++-kinds=+px --append
