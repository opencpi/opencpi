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

OcpiApp=biasFile

OcpiRunArgs?= $(ARGS)

# Prepare the input data before the app is run
override OcpiRunBefore+= \
  g++ -Wall -o target-$(OCPI_TOOL_DIR)/a.out mkTestFile.cxx; \
  target-$(OCPI_TOOL_DIR)/a.out $(FILE_SIZE) > test.input 2> good.output; $(VG)

# Verify after output after the app is run
override OcpiRunAfter+= && cmp test.output good.output && echo PASSED

include $(OCPI_CDK_DIR)/include/application.mk

clean::
	$(AT)rm -f -r test.output test.input good.output sss simulations/
