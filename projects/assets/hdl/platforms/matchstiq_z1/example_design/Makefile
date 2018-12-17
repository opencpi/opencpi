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

export OCPI_LIBRARY_PATH=lib

APP=matchstiq_z1_example_design

include $(OCPI_CDK_DIR)/include/application.mk

.SILENT: show
.PHONY: run clean show

# build the HDL unit test assemblies
all:
	make -C assemblies

clean::
	make -C assemblies clean
	rm -f *.forJenkins && rm -rf odata/*

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
----------------------------------------------------------------------------
The instructions below are used to execute an application on a remote
platform like the Matchstiq-Z1.

There are four steps for executing an application:
1) Build ACI and/or input test data (development system)
2) Setup the OCPI_LIBRARY_PATH (remote system)
3) Execution of application (remote system)
4) Verification of output data (development system)
----------------------------------------------------------------------------
1) Build ACI and/or input test data: (development system)
NOTE: Required only if test data has never been built.
OCPI_TARGET_PLATFORM=xilinx13_3 make
----------------------------------------------------------------------------
2) Setup the OCPI_LIBRARY_PATH: (remote system)

NOTE: Reminder to setup the run-time OCPI environment on remote system.
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

NOTE: Convenience cmd to quickly switch to application directory.
cd /mnt/ocpi_assets/hdl/platforms/matchstiq_z1/example_design

REQUIRED - Reconfigure to default.
Copy/paste string to terminal of remote system:
export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/$$OCPI_TOOL_PLATFORM/artifacts

REQUIRED - Configure for operation.
Copy/paste string to terminal of remote system:
export OCPI_LIBRARY_PATH+=:$$(pwd):$$(pwd)/../../../../artifacts
NOTE: When copy/paste of the above 'export' from the Makefile source file, the redundant '$' must be manually removed.
----------------------------------------------------------------------------
3) Execution of application is specific to platform: (remote system)
./target-xilinx13_3/matchstiq_z1_example_design
----------------------------------------------------------------------------
4) Verification of output data: (development system)
python ./scripts/plotAndFft.py odata/testbench_rx.out complex 300000 1 &
----------------------------------------------------------------------------
----------------------------------------------------------------------------
endef
export showhelp
