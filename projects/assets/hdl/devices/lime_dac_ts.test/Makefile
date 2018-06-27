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

# This file is used to build the Application Control Interface api testbench,
# calls make on the underlying unit test hdl assemblies, and is used to test the
# respective component. This file handles rcc and hdl simulation and hardware
# workers/assemblies. This has been tested on the ml605 and alst4 hardware
# platforms, as well as isim on x86_64.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

# add the RCC component and HDL assemblies to OCPI_LIBRARY_PATH
EXPORT_PATH = export OCPI_LIBRARY_PATH=:$(OCPI_LIBRARY_PATH):$(PWD);

MSG_EXE =  ==== Target Platform :
PLOT?=0
PLATFORM=none
APP=testbench

include $(OCPI_CDK_DIR)/include/application.mk

ifneq (,)
  include $(OCPI_CDK_DIR)/include/test.mk
endif

# build the testbench executable
run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

# build the HDL unit test assemblies
all:
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
XSIM_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c modelsim)
MODELSIM_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c modelsim)
endif

export CONTAINERS RCC_CONTAINERS ML605_CONTAINERS ALST4_CONTAINERS ISIM_CONTAINERS XSIM_CONTAINERS MODELSIM_CONTAINERS

.SILENT: tests lime_dac_ts show
.PHONY: tests lime_dac_ts test all run clean show

tests:
ifneq ($(ISIM_CONTAINERS),0)
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=100e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=80e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=40e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=20e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=10e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e5
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=100e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=80e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=40e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=20e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=10e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e6
	$(MAKE) test P=isim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e5
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=100e6
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=80e6
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=40e6
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=20e6
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=10e6
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=1e6
	$(MAKE) test P=isim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=1e5
endif
ifneq ($(XSIM_CONTAINERS),0)
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=100e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=80e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=40e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=20e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=10e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e5
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=100e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=80e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=40e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=20e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=10e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e6
	$(MAKE) test P=xsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e5
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=100e6
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=80e6
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=40e6
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=20e6
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=10e6
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=1e6
	$(MAKE) test P=xsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=1e5
endif
ifneq ($(MODELSIM_CONTAINERS),0)
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=100e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=80e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=40e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=20e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=10e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=false NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e5
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=100e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=80e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=40e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=20e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=10e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e6
	$(MAKE) test P=modelsim OAS=app FLUSH=false BURST=true  NUM_OUTPUT_SAMPLES=480 CLK_RATE=1e5
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=100e6
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=80e6
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=40e6
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=20e6
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=10e6
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=1e6
	$(MAKE) test P=modelsim OAS=app FLUSH=true  BURST=true  NUM_OUTPUT_SAMPLES=320 CLK_RATE=1e5
endif
	true

test: lime_dac_ts

EXECUTION_STRING=./$(PROG) $(OAS) $(CLK_RATE) $(P) $(FLUSH) $(NUM_OUTPUT_SAMPLES)

# TODO: Move to Makefile native so can fail properly

lime_dac_ts:
	# make test platform OAS
	#Ex: "make test P=isim OAS=app"
	$(eval P=isim)
	$(eval OAS=app)
	$(eval FLUSH=false)
	$(eval BURST=true)
	$(eval ASSEMBLY_NAME=Lime DAC TS $(OAS))
	echo "**** UUT: $(ASSEMBLY_NAME) - Using file_read/file_write HDL OAS to pass data ****"
	# Remove old test i/o data files
	rm -rf idata/* odata/* simulations
	# Create test input data
	python ./scripts/generate.py $(FLUSH) $(BURST) idata/input_file.bin
	# Set search path and execute application assembly
	echo $(MSG_EXE) $(P) ; \
	$(EXPORT_PATH) \
	$(EXECUTION_STRING) ;
	python ./scripts/verify.py odata/output_file.bin idata/input_data_only.bin
	grep tx_time_int simulations/lime_dac_ts_assy.$(P).*/sim.out

clean::
	rm -rf idata/* odata/* simulations
	$(MAKE) -C assemblies clean

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq-Z1, Zedboard/Zipper)'
These steps may be repeated to test each available HDL assembly
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Development Computer - generate input data
----------------------------------------------------------------------------
python scripts/generate_hw.py idata/input_data_hw.bin
----------------------------------------------------------------------------
Zynq-Based Platform - setup environment
----------------------------------------------------------------------------
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

cd /mnt/ocpi_assets/hdl/devices/lime_dac_ts.test

export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib:$$(pwd):$$(pwd)/../../../exports/lib
#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute
HDL (Matchstiq-Z1):
ocpirun -v -t 5 app_lime_dac_ts_test_matchstiq_z1
HDL (Zed/Zipper):
ocpirun -v -t 5 app_lime_dac_ts_test_zipper
----------------------------------------------------------------------------
Development Computer - verify output data
'Visually inspect the spectrum on a spectrum analyzer. 1 second off, 1 second on, 1 second off, 1 second on.'
----------------------------------------------------------------------------
endef
export showhelp
