# Copyright (c) 2009 Mercury Federal Systems.
# 
# This file is part of OpenCPI.
# 
# OpenCPI is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# OpenCPI is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

ifneq ($(SYSTEM),)
SYSTEMOPTION="SYSTEM=$(SYSTEM)"
endif

# defaults
CPIDIR := $(shell pwd)
CLIENT_IDL_ONLY := 1
LD_LIBRARY_PATH := $(LD_LIBRARY_PATH):$(CPIDIR)/lib
export LD_LIBRARY_PATH
export CLIENT_IDL_ONLY
export CPIDIR
include MakeVars.cpi

#
# ----------------------------------------------------------------------
# Build up a list of $(PACKAGES) to build.  This list is carefully
# ordered according to package dependencies.
# ----------------------------------------------------------------------
#

#
# Basic packages.
#

PACKAGES += adapt/os/cpios core/local/logger core/local/util
PACKAGES += core/control/wci_api

PACKAGES += \
	 core/$(DATAPLANE)/rdma_driver_interface \
	 core/$(DATAPLANE)/rdma_drivers \
	 core/$(DATAPLANE)/transport \
	 core/container/interfaces \
	 core/container/cp289_container \
	 core/container/rpl_container \
	 core/container/tests

ifeq ($(HAVE_CORBA),1)
PACKAGES += core/corba/orb_services core/corba/corba_util
PACKAGES += core/sca/cf core/sca/cf_util core/sca/sgac
PACKAGES += core/sca/gpped
PACKAGES += core/sca/srpl
endif

PACKAGES += tools/local/binder tools/local/tester
PACKAGES += tools/sdk/ocpigen
PACKAGES += test

#
# ----------------------------------------------------------------------
# A list of all packages for make clean.  Sorted alphabetically.
# ----------------------------------------------------------------------
#


ALLPACKAGES = \
	adapt/os/cpios \
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
	tools/local/binder \
	tools/local/tester \
	tools/sdk/ocpigen

#
# ----------------------------------------------------------------------
# Rules.
# ----------------------------------------------------------------------
#

all: packages

MODELS=rcc
.PHONY: components hdlapps
components:
	make -C components $(MODELS)

hdlapps: MODELS=hdl
hdlapps: components
	make -C hdl/apps

hdl: MODELS=hdl
hdl: hdlapps

rcc: MODELS=rcc
rcc: components

ifeq ($(filter hdl,$(MODELS)),hdl)
components: prims

.PHONY: prims
prims:
	make -C hdl/prims
	make -C hdl/prims install

endif


.PHONY: packages tar diff diff.q test $(PACKAGES)

compile build: $(PACKAGES)
packages: $(PACKAGES)

$(PACKAGES):
	if test -f $@/Makefile.cpi ; then \
		$(MAKE) -C $@ $(SYSTEMOPTION) -f Makefile.cpi ; \
	else \
		$(MAKE) -C $@ $(SYSTEMOPTION) -f ../../../Makefile.cpi.for-pkg ; \
	fi

clean distclean:
	for package in $(ALLPACKAGES) ; do \
		if test -f $$package/Makefile.cpi ; then \
			$(MAKE) -C $$package -f Makefile.cpi $@ ; \
		else \
			$(MAKE) -C $$package -f ../../../Makefile.cpi.for-pkg $@ ; \
		fi ; \
	done
	find . -name '*~' -exec rm {} \;
	-rm -f diff diff.q
	-rm -f *.exe *.obj *.o *.ilk *.sbr *.suo *.sln *.pdb *.bsc *~

tar:
	tar cvf cpi.tar MakeVars.cpi Makefile.cpi Makefile.cpi.for-* $(ALLPACKAGES)
	gzip -9 cpi.tar

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

core/local/logger: adapt/os/cpios
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
