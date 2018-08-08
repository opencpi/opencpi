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

ifneq (,)
  include $(OCPI_CDK_DIR)/include/test.mk
endif

# build the HDL unit test assemblies
all:
	make -C assemblies

clean::
	make -C assemblies clean

show:
	echo "$$showhelp"

define showhelp
----------------------------------------------------------------------------
Execution:
./scripts/test.sh # this will search for an available OpenCPI PCIE card
                  # (does not check to ensure its an alst4 card), load
                  # an FPGA bitfile (which supports any of the following:
                  # alst4's HSMC Port A card, alst4's HSMC Port B card, or 
                  # both) then runs the test on either or both of HSMC
                  # Port A/B, depending on which cards are physically
                  # plugged in (the underlying device worker automatically
                  # detects which cards are physically present)
----------------------------------------------------------------------------
Verification: (note that the stdout.expected includes results for both
               HSMC A and HSMC B loopback cards, so if one or more cards
               aren't physically plugged in, the diff will show 
               differences)

./scripts/test.sh > odata/stdout.testrun
diff odata/stdout.expected odata/stdout.testrun
#vimdiff odata/stdout.expected odata/stdout.testrun
----------------------------------------------------------------------------
endef
export showhelp

