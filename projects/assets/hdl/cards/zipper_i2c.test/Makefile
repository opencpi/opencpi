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
# respective component. This file handles hardware workers and has been tested on
# the zed platform.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

APP=testbench

include setup.mk

# if never, this statement exists so that the directory evaluates as a test directory for ocpidev
ifneq(,)
  include $(OCPI_CDK_DIR)/include/test.mk
endif

.SILENT: show
.PHONY: run clean show

# build the testbench executable
run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

# build the HDL unit test assemblies
all:
	make -C assemblies

clean::
	make -C assemblies clean
	rm -rf odata/*

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq-Z1, Zedboard)'
These steps may be repeated to test each available HDL assembly
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Zedboard-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

cd /mnt/ocpi_assets/hdl/cards/zipper_i2c.test

export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib:$$(pwd):$$(pwd)/../../../exports/lib
#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute
HDL: 
./target-linux-x13_3-arm/testbench
----------------------------------------------------------------------------
Development Computer - verify output data
python ./scripts/plotAndFft.py odata/testbench_rx.out complex 3000 1000000 &
----------------------------------------------------------------------------
endef
export showhelp
