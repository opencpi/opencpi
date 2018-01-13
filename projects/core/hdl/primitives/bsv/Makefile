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

# This primitive library contains verilog primitive modules that are used
# and usable by BSV code, and supplied by BSV.
# All targets should be happy
Imports=$(OCPI_HDL_IMPORTS_DIR)/libsrc/hdl/bsv/*.[vV]
# Exclude the BRAMs since they don't work on altera - they are in util_xilinx
ExcludeImports=main.v BRAM1.v BRAM1BE.v BRAM2.v
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk # we are a primitive library
