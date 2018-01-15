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

# This file calls make on the underlying unit test hdl assemblies and is used to
# test the respective component. This file handles hdl simulation and hardware
# workers/assemblies. This has been tested on the Matchstiq hardware platform,
# as well as isim/modelsim/xsim on x86_64.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

# add HDL assemblies to OCPI_LIBRARY_PATH
EXPORT_PATH = export OCPI_LIBRARY_PATH=$(OCPI_CDK_DIR):$(PWD);

MSG_EXE_HW =  ==== Target Platform :
PLOT?=0

all:
	$(MAKE) build

# build the HDL unit test assemblies
build:
	$(MAKE) -C components
	$(MAKE) -C assemblies

# skip if executing a 'clean' or 'show'
ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter show,$(MAKECMDGOALS)),)
# "make tests" runs all tests below based upon running containers, and if isim
# or modelsim is found to be installed.
# This can be overriden on the command line, e.g. "make tests RCC_CONTAINERS=0 MODELSIM_CONTAINERS=0"
ifeq (,$(CONTAINERS))
# This should run ONCE
CONTAINERS:=$(shell ocpirun -C)
endif
RCC_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c rcc)
ML605_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c ml605)
ALST4_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c alst4)
ISIM_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c isim)
XSIM_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c xsim)
MODELSIM_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c modelsim)
endif

export CONTAINERS RCC_CONTAINERS ML605_CONTAINERS ALST4_CONTAINERS ISIM_CONTAINERS MODELSIM_CONTAINERS

.SILENT: tests lime_dac_ts show
.PHONY: tests lime_dac_ts test all run clean show

run:
ifneq ($(ML605_CONTAINERS),0)
	$(MAKE) test P=ml605 OAS=app_lime_tx_test_zipper PIN_CONTROL=1 MAX_COUNT_VALUE=100000000 TIME=1
endif
ifneq ($(ALST4_CONTAINERS),0)
	$(MAKE) test P=alst4 OAS=app_lime_tx_test_zipper PIN_CONTROL=1 MAX_COUNT_VALUE=100000000 TIME=1
endif
ifneq ($(ISIM_CONTAINERS),0)
	$(MAKE) test P=isim OAS=app_lime_tx_test_sim_pin_control_false MAX_COUNT_VALUE=2000 TIME=10
	$(MAKE) verify GOLDEN_FILE=golden_pin_control_false.dat OUTPUT_FILE=odata/out_app_lime_tx_sim_pin_control_false.dat
	$(MAKE) test P=isim OAS=app_lime_tx_test_sim_pin_control_true  MAX_COUNT_VALUE=2000 TIME=10
	$(MAKE) verify GOLDEN_FILE=golden_pin_control_true.dat OUTPUT_FILE=odata/out_app_lime_tx_sim_pin_control_true.dat
endif
ifneq ($(MODELSIM_CONTAINERS),0)
	$(MAKE) test P=modelsim OAS=app_lime_tx_test_sim_pin_control_false MAX_COUNT_VALUE=2000 TIME=10
	$(MAKE) verify GOLDEN_FILE=golden_pin_control_false.dat OUTPUT_FILE=odata/out_app_lime_tx_sim_pin_control_false.dat
	$(MAKE) test P=modelsim OAS=app_lime_tx_test_sim_pin_control_true MAX_COUNT_VALUE=2000 TIME=10
	$(MAKE) verify GOLDEN_FILE=golden_pin_control_true.dat OUTPUT_FILE=odata/out_app_lime_tx_sim_pin_control_true.dat
endif
ifneq ($(XSIM_CONTAINERS),0)
	$(MAKE) test P=xsim OAS=app_lime_tx_test_sim_pin_control_false MAX_COUNT_VALUE=2000 TIME=10
	$(MAKE) verify GOLDEN_FILE=golden_pin_control_false.dat OUTPUT_FILE=odata/out_app_lime_tx_sim_pin_control_false.dat
	$(MAKE) test P=xsim OAS=app_lime_tx_test_sim_pin_control_true MAX_COUNT_VALUE=2000 TIME=10
	$(MAKE) verify GOLDEN_FILE=golden_pin_control_true.dat OUTPUT_FILE=odata/out_app_lime_tx_sim_pin_control_true.dat
endif
	true

test: lime_tx

lime_tx:
	# make test platform OAS TIME enable_plotting
	#Ex: "make test P=xsim OAS=app_lime_tx_test"
	$(eval P=xsim)
	$(eval OAS=app_lime_tx_test)
	$(eval ASSEMBLY_NAME=Lime tx $(OAS))
	echo "**** UUT: $(ASSEMBLY_NAME) - Using file_read/write HDL OAS to pass data ****"
	# Remove old test output and sim data files
	#rm -rf odata/* simulations/*
	# Setup environment path per platform and execute application assembly
	echo $(MSG_EXE_HW) $(P) ; \
	$(EXPORT_PATH) \
	ocpirun -v -Ptest_tx_event=$(P) -Pfile_write=$(P) -Prf_tx=$(P) -Plime_tx_em=$(P) -Plime_spi=$(P) -Plime_spi_em=$(P) -ptest_tx_event=max_count_value=$(MAX_COUNT_VALUE) -t $(TIME) $(OAS) ;

verify:
	cmp $(GOLDEN_FILE) $(OUTPUT_FILE)
clean::
	rm -f odata/* && rm -rf simulations/*
	$(MAKE) -C components clean
	$(MAKE) -C assemblies clean

show:
	echo "$$showhelp"

generate:
	$(info There is no generate step for this unit test)

prepare:
	$(info There is no prepare step for this unit test)

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq, Zedboard)'
These steps may be repeated to test each available HDL assembly
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Setup:
----------------------------------------------------------------------------
Zynq-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {IPADDRESS}
cd /mnt/ocpi_baseproject/hdl/devices/lime_tx.test

HDL (Matchstiq-Z1): 'export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib/components:/mnt/ocpiassets/hdl/platforms/matchstiq_z1/devices/lib/rcc:/mnt/ocpi_baseproject/exports/lib/devices/rcc:/mnt/ocpiassets/hdl/exports/lib/devices/rcc:assemblies/lime_tx_test_assembly'

HDL (Zed/Zipper): 'export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib/components:/mnt/ocpi_baseproject/exports/lib/devices/rcc:/mnt/ocpiassets/exports/lib/devices/rcc:assemblies/lime_tx_test_assembly'
#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.

HDL: 'rm -rf odata/* simulations/*'
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute
HDL (Matchstiq-Z1): 'ocpirun -v -t 10 -ptest_tx_event=max_count_value=100000000 app_lime_tx_test_matchstiq_z1'
HDL (Zed/Zipper): 'ocpirun -v -t 10 -ptest_tx_event=max_count_value=100000000 app_lime_tx_test_zipper'
----------------------------------------------------------------------------
endef
export showhelp
