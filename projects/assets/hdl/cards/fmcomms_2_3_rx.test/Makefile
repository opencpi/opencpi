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

# this is an ACI Makefile modified to act simlar to $(OCPI_CDK_DIR/include/test.mk

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

OcpiAppNoRun=1
OcpiApp=fmcomms_2_3_rx_test_app
RccIncludeDirs=../fmcomms_2_3_rx.rcc/include/
RccOnlyPlatforms+=centos7 xilinx13_3 # we only include tested rcc platforms

include $(OCPI_CDK_DIR)/include/application.mk

# this is necessary to allow 'ocpidev run' to correctly run the unit test (without this
# line you can still run the test but must use make, e.g. 'make run')
ifneq (,)
include $(OCPI_CDK_DIR)/include/test.mk
endif

# !!!!!!!!!!!!!!! if the below make -C line changes to add HPC, be sure to update ocpidev run commands in the component data sheet
asm:
	make -C assemblies Containers='cnt_1rx_0tx_bypassasm_fmcomms_2_3_lpc_LVDS_ml605 cnt_1rx_0tx_bypassasm_fmcomms_2_3_lpc_LVDS_zed' # only supporting ML605 LPC slot for now

# quick, software-only test

buildswonly:

runswonly: prepareswonly
	$(AT)./run/runtests.sh runswonly verifyswonly $(and $(View),view)

verifyswonly: buildswonly
	$(AT)./run/runtests.sh verifyswonly $(and $(View),view)

prepareswonly:

verifyswonly: build

# Map global goals to local goals
test: build
runtests: run
tests: build
verifytest: verify
all: build
runonlytest: runonly
runtest: run

# mimicking targets defined in OpenCPI_Component_Development.pdf Section 13.9

# "The make command is used with the following goals to invoke the phases:"
build: asm

run: prepare
	$(AT)./run/runtests.sh run verify $(and $(View),view)

# "The following goals control the process at a finer granularity:"

generate:

prepare:

runnoprepare:
	$(AT)./run/runtests.sh run

runonly: prepare runnoprepare

verify: build
	$(AT)./run/runtests.sh verify $(and $(View),view)

view:
	$(AT)./run/runtests.sh view

cleanrun:

cleansim:
	$(AT)rm -r -f run/*/*.simulation

