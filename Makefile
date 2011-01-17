
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


#$(info $(shell pwd))
#empty:=
#space:=$(empty) $(empty)
#$(info $(notdir a/b$$(space)v/h))


ifneq ($(OCPI_OS),)
SYSTEMOPTION="OCPI_OS=$(OCPI_OS)"
endif

# defaults
OCPI_BASE_DIR :=
OCPI_ABS_DIR := $(shell pwd|sed 's/ /\\ /g')
CLIENT_IDL_ONLY := 1
LD_LIBRARY_PATH := $(LD_LIBRARY_PATH):$(OCPI_ABS_DIR)/lib
export LD_LIBRARY_PATH
export CLIENT_IDL_ONLY
export OCPI_BASE_DIR
include MakeVars.ocpi

#
# ----------------------------------------------------------------------
# Build up a list of $(PACKAGES) to build.  This list is carefully
# ordered according to package dependencies.
# ----------------------------------------------------------------------
#

#
# Basic packages.
#

PACKAGES += adapt/os/ocpios core/local/logger core/local/util
PACKAGES += core/control/wci_api

PACKAGES += \
	 core/$(DATAPLANE)/rdma_driver_interface \
	 core/$(DATAPLANE)/rdma_drivers \
	 core/$(DATAPLANE)/transport \
	 core/container/interfaces \
	 core/container/cp289_container \
	 core/container/rpl_container \
	 core/container/ctests \
	 core/$(DATAPLANE)/tests

ifeq ($(OCPI_HAVE_CORBA),1)
PACKAGES += core/corba/orb_services core/corba/corba_util
PACKAGES += core/sca/cf core/sca/cf_util core/sca/sgac
PACKAGES += core/sca/gpped
PACKAGES += core/sca/srpl
endif

PACKAGES += tools/local/binder tools/local/tester
PACKAGES += tools/cdk/ocpigen
PACKAGES += tools/cdk/ocpixm
PACKAGES += test

PACKAGES += tests/facilities/test_os
PACKAGES += tests/facilities/test_logger
PACKAGES += tests/framework/framework

#
# ----------------------------------------------------------------------
# A list of all packages for make clean.  Sorted alphabetically.
# ----------------------------------------------------------------------
#


ALLPACKAGES = \
	adapt/os/ocpios \
	core/local/logger \
	core/local/util \
	core/control/wci_api \
	core/corba/corba_util \
	core/corba/orb_services \
	core/$(DATAPLANE)/rdma_driver_interface \
	core/$(DATAPLANE)/rdma_drivers \
	core/$(DATAPLANE)/transport \
	core/container/interfaces \
	core/container/cp289_container \
	core/container/rpl_container \
	core/container/tests \
	core/sca/cf \
	core/sca/cf_util \
	core/sca/gpped \
	core/sca/sgac \
	core/sca/srpl \
	test \
	tests/facilities/test_os \
	tests/facilities/test_logger \
	tests/framework/framework \
	tools/local/binder \
	tools/local/tester \
	tools/cdk/ocpigen \
	tools/cdk/ocpixm \
	core/$(DATAPLANE)/tests

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
	$(MAKE) -C hdl applications

hdl:
	$(MAKE) -C hdl

hdlclean:
	$(MAKE) -C hdl clean

rcc:
	make -C components rcc

.PHONY: prims
hdlprims:
	$(MAKE) -C hdl primitives

.PHONY: packages tar diff diff.q test $(PACKAGES)

everything: packages rcc hdl
compile build: $(PACKAGES)
packages: $(PACKAGES)

$(PACKAGES):
	$(AT)if test -f $@/Makefile.ocpi ; then \
		$(MAKE) $(call DescendMake,$@) $(SYSTEMOPTION) -f Makefile.ocpi ; \
	else \
		$(MAKE) $(call DescendMake,$@) $(SYSTEMOPTION) -f $(call AdjustRelative,$@,)Makefile.ocpi.for-pkg ; \
	fi

clean distclean:
	$(AT)$(foreach p,$(ALLPACKAGES),\
		if test -f $p/Makefile.ocpi ; then \
			$(MAKE) $(call DescendMake,$p) -f Makefile.ocpi $@ ; \
		else \
			$(MAKE) $(call DescendMake,$p) -f $(call AdjustRelative,$p,)Makefile.ocpi.for-pkg $@ ; \
		fi ; \
	)
	find . -name '*~' -exec rm {} \;
	-rm -f diff diff.q
	-rm -f *.exe *.obj *.o *.ilk *.sbr *.suo *.sln *.pdb *.bsc *~
	-rm -r -f lib

cleaneverything: clean
	make -C hdl/prims clean
	make -C components clean
	make -C hdl/apps clean

tar:
	tar cvf ocpi.tar MakeVars.ocpi Makefile.ocpi Makefile.ocpi.for-* $(ALLPACKAGES)
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

core/local/logger: adapt/os/ocpios
core/local/util: core/local/logger
core/$(DATAPLANE)/tests: \
	core/container/cp289_container core/$(DATAPLANE)/transport \
	core/$(DATAPLANE)/rdma_driver_interface core/control/wci_api
core/corba/corba_util: core/corba/orb_services core/local/util
core/sca/cf_util: core/sca/cf core/corba/corba_util core/corba/orb_services \
	core/local/util
core/sca/sgac: core/sca/cf_util
core/sca/gpped: core/sca/cf_util
tools/local/binder: core/local/util
tools/local/tester: \
	core/container/cp289_container core/$(DATAPLANE)/transport \
	core/$(DATAPLANE)/rdma_driver_interface core/control/wci_api
test: tools/local/binder tools/local/tester
