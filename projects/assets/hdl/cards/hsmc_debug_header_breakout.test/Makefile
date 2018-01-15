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

include setup.mk

.SILENT: show
.PHONY: clean show

# build the HDL unit test assemblies
all:
	make -C assemblies

clean::
	make -C assemblies clean

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Hardware Setup:
Populate either HSMC Port A or HSMC Port B with an HSMC debug header 
breakout card. Leave the other HSMC Port unconnected.
----------------------------------------------------------------------------
Execution:
# if HSMC Port A is populated:
./scripts/test_hsmc_debug_header_breakout_is_plugged_into_port_a.sh
./scripts/test_hsmc_debug_header_breakout_is_plugged_into_port_a.sh -p # to prompt you before each LED
# if HSMC Port B is populated:
./scripts/test_hsmc_debug_header_breakout_is_plugged_into_port_b.sh
./scripts/test_hsmc_debug_header_breakout_is_plugged_into_port_b.sh -p # to prompt you before each LED
----------------------------------------------------------------------------
Verification:
visually inspect LEDs while executing (also inspect stdout while executing)
----------------------------------------------------------------------------
endef
export showhelp

