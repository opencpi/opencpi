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
# the matchstiq_z1 platform.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

APP=lime_rx_proxy_test_app

include setup.mk

.SILENT: show
.PHONY: run clean show

# build the testbench executable
run: all
	OCPI_LIBRARY_PATH=lib $(VG) $(PROG) $(OUT)

# build the HDL unit test assemblies
all:
	make -C assemblies

clean::
	rm -rf odata/*
	make -C assemblies clean

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq-Z1, Zedboard/Zipper)'
These steps may be repeated to test each available HDL assembly
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Zynq-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {IPADDRESS}
cd /mnt/ocpiassets/hdl/devices/lime_rx_proxy.test
HDL: 
export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/lib/components:/mnt/ocpi_baseproject/exports/lib/devices/:$$(pwd)/../../../exports/lib/devices/:$$(pwd)/../../../exports/lib/platforms/:$$(pwd)
#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute
HDL: 
./target-linux-x13_3-arm/lime_rx_proxy_test_app
----------------------------------------------------------------------------
Development Computer - verify output data
(Matchstiq-Z1): 
python ./scripts/plotAndFft.py odata/lime_rx_proxy_test_app.out complex 300000 0.1 &
(Zedboard): 
python ./scripts/plotAndFft.py odata/lime_rx_proxy_test_app.out complex 1500000 0.5 &
----------------------------------------------------------------------------
endef
export showhelp
