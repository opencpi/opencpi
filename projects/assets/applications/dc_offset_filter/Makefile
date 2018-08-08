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

# This file is used to build the Application Control Interface api testbench and
# is used to run the respective application. This file handles hardware workers
# and has been tested on the Matchstiq-Z1 and Zedboard platforms.

$(if $(realpath $(OCPI_CDK_DIR)),,$(error The OCPI_CDK_DIR environment variable is not set correctly.))

export OCPI_LIBRARY_PATH=lib

APP=dc_offset_filter

include $(OCPI_CDK_DIR)/include/application.mk

.SILENT: show
.PHONY: clean show

clean::
	rm -rf odata/*

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Instructions for setup/execution on remote platforms '(i.e. Matchstiq-Z1, Zedboard)'
These steps may be repeated to test each available HDL assembly
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Setup:
----------------------------------------------------------------------------
Development Computer - generate test ACI executable
'OCPI_TARGET_PLATFORM=xilinx13_3 make'
----------------------------------------------------------------------------
Zynq-Based Platform - setup environment
. /mnt/card/opencpi/mynetsetup.sh {DEVELOPMENT HOST IPADDRESS}

cd /mnt/ocpi_assets/applications/dc_offset_filter

export OCPI_LIBRARY_PATH=$$OCPI_CDK_DIR/../projects/core/exports/lib/components:$$(pwd)/../../hdl/assemblies/$${PWD##*/}:$$(pwd)/../../exports/lib
#NOTE: If copy/paste of the above exports, the redundant '$' must be removed.
----------------------------------------------------------------------------
----------------------------------------------------------------------------
Execution:
----------------------------------------------------------------------------
Zynq-Based Platform - execute (pick one)
HDL (Matchstiq-Z1):
./target-linux-x13_3-arm/dc_offset_filter 1000 .512 400 10 .75 51 1
HDL (ZedBoard):
./target-linux-x13_3-arm/dc_offset_filter 1000 .512 -1 6 .75 51 1
----------------------------------------------------------------------------
Development Computer - verify output data
python ./scripts/plotAndFft.py odata/output_file.bin complex 65536 1000000 &
----------------------------------------------------------------------------
endef
export showhelp


