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

OcpiApp=copy

PROG=target-$(OCPI_TOOL_DIR)/$(OcpiApp)
override OcpiRunAfter+= < $(PROG) > test.output ;
override OcpiRunAfter+= ls -l $(PROG) test.output ; \
               cmp $(PROG) test.output

# Set package to local for the copy worker
Package=local
export OCPI_LIBRARY_PATH=lib

# These make the worker library for this application
Implementations = copy.rcc
include $(OCPI_CDK_DIR)/include/application.mk
include $(OCPI_CDK_DIR)/include/lib.mk
clean::
	rm -f test.output
