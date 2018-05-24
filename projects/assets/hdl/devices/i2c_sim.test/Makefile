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
# respective component. This file handles hdl simulation and has been tested on
# isim on the x86_64 platform.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

# add the RCC component and HDL assemblies to OCPI_LIBRARY_PATH
EXPORT_PATH = export OCPI_LIBRARY_PATH=:$(OCPI_LIBRARY_PATH):$(PWD);

MSG_EXE =  ==== Target Platform :

#Package=ocpi.assets.devices.i2c_sim.simdevices
APP=testbench

include setup.mk

#if never, this statement exists so that the directory evaluates as a test directory for ocpidev
ifneq (,)
  include $(OCPI_CDK_DIR)/include/test.mk
endif

# build the testbench executable
run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

# build the HDL unit test assemblies
all:
	$(MAKE) -C simdevices
	$(MAKE) -C assemblies

# skip if executing a 'clean' or 'show'
ifeq ($(filter clean%,$(MAKECMDGOALS))$(filter show,$(MAKECMDGOALS)),)
# This can be overriden on the command line, e.g. "make tests MODELSIM_CONTAINERS=0"
ifeq (,$(CONTAINERS))
# This should run ONCE
CONTAINERS:=$(shell ocpirun -C)
endif
ISIM_CONTAINERS?=$(shell echo "$(CONTAINERS)" | grep -c isim)
endif

export CONTAINERS ISIM_CONTAINERS

.SILENT: tests sim_testbench show
.PHONY: tests sim_testbench run clean show

# "make tests" runs all tests below based upon if isim and/or modelsim are found to be installed
tests:
ifneq ($(ISIM_CONTAINERS),0)
	$(MAKE) test P=isim OAS=sim_testbench_app_file
endif
	true

test: sim_testbench

sim_testbench:
	$(eval P=isim)
	$(eval OAS=sim_testbench_app_file)
	$(eval ASSEMBLY_NAME=I2C SIM TESTBENCH $(OAS))
	@echo "**** UUT: $(ASSEMBLY_NAME) - Using file_read/write HDL OAS to pass data ****"
	# Remove old test i/o data files
	rm -f *.results && rm -rf simulations/*
	# Setup environment path per platform and execute application assembly
	# Set search path and execute application assembly
	echo $(MSG_EXE) $(P) ; \
	$(EXPORT_PATH) \
	./$(PROG) $(OAS) $P ;

clean::
	rm -f *.results && rm -rf simulations/*
	$(MAKE) -C simdevices clean
	$(MAKE) -C assemblies clean

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
make test P=isim OAS=sim_testbench_app_file
----------------------------------------------------------------------------
endef
export showhelp
