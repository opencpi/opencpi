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

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

# if never, this statement exists so that the directory evaluates as a test directory for ocpidev
ifneq (,)
  include $(OCPI_CDK_DIR)/include/test.mk
endif

# Custom code is included in test_XX/localinclude.mk
# There, for example, you can set SIM_TIME or PRE_HOOK.
# PRE_HOOK / POST_HOOK should end / start with "&&"
# SKIP_TEST if the test should not run
-include localinclude.mk

ECHO=/bin/echo
WORKER=advanced_pattern
ifneq (,$(filter testrun,$(MAKECMDGOALS)))
ifeq (,$(VG))
SIM_TIME?=10
else
SIM_TIME?=30
$(warning SIM_TIME is currently $(SIM_TIME)s; this might not be sufficient under valgrind!)
endif
endif

RCCWORKER=$(WORKER).rcc
space:=
space+=
SUPPORT_PATH:=$(subst $(space),:,$(foreach worker, $(RCCWORKER) file_write_demux.rcc,$(wildcard $(shell pwd)/../../$(worker)/target-*)))
SUPPORT_PATH_REMOTE:=export OCPI_LIBRARY_PATH=\$${OCPI_CDK_DIR}/components/lib/rcc:../lib/rcc
EXPORT_PATH := export OCPI_LIBRARY_PATH=:$(OCPI_LIBRARY_PATH):\${OCPI_CDK_DIR}/components/lib/rcc:$(SUPPORT_PATH) && set -o pipefail && $(PRE_HOOK)

ifneq (,$(filter verify,$(MAKECMDGOALS)))
export NO_AUTOCONFIG_WARNING=1
endif

# include ../zero_padding.test/setup.mk

RCC_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c rcc)

DEBUG=-d -v
# DEBUG+=-l 11

.SILENT: tests test verify testgen testrun testverify clean generate.sh verify.sh all runtest runtests run verifytest generate build prepare testrun_wrapper testverify_wrapper testgen_wrapper
.PHONY: tests test verify testgen testrun testverify clean all runtest runtests run verifytest generate build prepare testrun_wrapper testverify_wrapper testgen_wrapper

# Map to "standard" targets:
all: build
test: build
# Conflict -- tests: build
runtest: run
runtests: run
run: testrun_wrapper verify
verifytest: verify
verify: testverify_wrapper
generate: testgen_wrapper
build: testgen_wrapper
prepare: testgen_wrapper

testgen_wrapper:
	$(foreach tst, $(TESTS), $(ECHO) -n "Generating $(tst): " && \
	  cat $(tst)/description && \
	  $(MAKE) -f ../Makefile -sC $(tst) --no-print-directory testgen && \
	  ) true

testrun_wrapper:
ifneq (,$(IP))
	@echo "wrapper not written to handle remote!"
	@false
endif
	$(foreach tst, $(TESTS), $(ECHO) -n "Running $(tst): " && \
	  cat $(tst)/description && \
	  $(MAKE) -f ../Makefile -sC $(tst) --no-print-directory testrun OCPI_HDL_SIMULATORS= && \
	  ) true

testverify_wrapper:
	$(foreach tst, $(TESTS), $(ECHO) -n "Verifying $(tst): " && \
	  cat $(tst)/description && \
	  $(MAKE) -f ../Makefile -sC $(tst) --no-print-directory testverify && \
	  ) true

TESTS?=$(sort $(wildcard test_[0-9]*))

define generate_sh
#!/bin/bash
[ -x ./generate.pl ] && ./generate.pl
[ -x ./generate.py ] && ./generate.py
true
endef
export generate_sh

# Default test generation:
generate.sh:
# 	echo Creating default generate.sh
	echo "$${generate_sh}" > $@
	chmod a+x $@

# Default test verification:
verify.sh:
# 	echo Creating default verify.sh
	echo "#!/bin/bash" > $@
	echo "set -e" >> $@
	echo "grep -L \"finished after waiting\" ocpirun.log | grep ocpirun.log" >> $@
	echo md5sum -c golden.md5 >> $@
	chmod a+x $@

# The generate / run / verify are split into three because they might be platform-dependent:

ifeq (,$(SKIP_TEST))
testgen: generate.sh verify.sh
	rm -rf odata
	mkdir odata
	./generate.sh > generate.log

testrun:
ifeq (,$(REMOTE))
	($(EXPORT_PATH) time $(VG) ocpirun $(DEBUG) -m UUT=rcc -t -$(SIM_TIME) application.xml $(POST_HOOK)) > ocpirun.log 2>&1
else
	echo "Remotely run: \" ($(SUPPORT_PATH_REMOTE) && cd $(TEST) && $(PRE_HOOK) time ocpirun $(DEBUG) -m UUT=rcc -t -$(SIM_TIME) application.xml $(POST_HOOK) > ocpirun.log 2>&1) \""
	read -p "Press Enter when ocpirun is finished: " x
endif

testverify:
	./verify.sh > verify.log 2>&1
else
testgen:
	echo "Skipping testgen step. SKIP_TEST is set."
testrun:
	echo "Skipping testrun step. SKIP_TEST is set."
testverify:
	echo "Skipping testverify step. SKIP_TEST is set."
endif

tests: clean
ifneq (,$(IP))
	$(foreach tst, $(TESTS), $(ECHO) -n "Executing $(tst): " && \
	  $(MAKE) -f ../Makefile -sC $(tst) testgen && \
	  $(MAKE) -f ../Makefile -sC $(tst) testrun REMOTE=1 TEST=$(tst) && \
	  $(MAKE) -f ../Makefile -sC $(tst) testverify && \
	  ) true
else
ifneq ($(RCC_CONTAINERS),0)
	$(foreach tst, $(TESTS), $(ECHO) -n "Executing ${tst}: " && \
	  $(MAKE) -f ../Makefile -sC $(tst) testgen && \
	  $(MAKE) -f ../Makefile -sC $(tst) testrun OCPI_HDL_SIMULATORS= && \
	  $(MAKE) -f ../Makefile -sC $(tst) testverify && \
	  ) true
else
	echo "This component only has an RCC implementation and no RCC containers specified: CONTAINERS($(CONTAINERS)) RCC_CONTAINERS($(RCC_CONTAINERS))"
endif
endif
	true

# fae095b227844f62a606df328578ec45 is original verify.sh (just do md5sum)
# 91d2bc67425922abb7cbaae4cdd293e3 is "new" verify.sh that also ensures the log doesn't say it aborted
# 742e17b3e6c5d44a511cde58305ae995 is "newer" with new (AV 1.2 string) concerning abort
clean::
	rm -rf {i,o}data run_*.log ocpirun.log timeData.raw
	for tst in $(TESTS); do \
	  if [ X`md5sum $${tst}/generate.sh 2>/dev/null | awk '{print $$1}'` == X06bbadb59dd0431fe21de30a5f0a64fb ]; then rm $${tst}/generate.sh; fi; \
	  if [ X`md5sum $${tst}/verify.sh 2>/dev/null | awk '{print $$1}'` == Xfae095b227844f62a606df328578ec45 ]; then rm $${tst}/verify.sh; fi; \
	  if [ X`md5sum $${tst}/verify.sh 2>/dev/null | awk '{print $$1}'` == X91d2bc67425922abb7cbaae4cdd293e3 ]; then rm $${tst}/verify.sh; fi; \
	  if [ X`md5sum $${tst}/verify.sh 2>/dev/null | awk '{print $$1}'` == X742e17b3e6c5d44a511cde58305ae995 ]; then rm $${tst}/verify.sh; fi; \
	  rm -rf $${tst}/*.{log,dump,out} $${tst}/timeData.raw $${tst}/odata; \
	  if compgen -G $${tst}/generate.* >/dev/null; then rm -rf $${tst}/idata; fi; \
	done
