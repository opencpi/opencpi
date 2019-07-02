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

# This Makefile is for the primitive library: util_prims

# Set this variable to any other primitive libraries that this library depends on.
# If they are remote from this project, use slashes in the name (relative or absolute)
# If they are in this project, they must be compiled first, and this requires that the
# PrimitiveLibraries variable be set in the hdl/primitives/Makefile such that the
# libraries are in dependency order.
Libraries=fixed_float ocpi util misc_prims

# Set this variable to the list of source files in dependency order
# If it is not set, all .vhd and .v files will be compiled in wildcard/random order,
# except that any *_pkg.vhd files will be compiled first
SourceFiles=$(patsubst %,%.vhd,\
	util_prims_pkg \
	agc/src/agc \
	pd/src/peakDetect \
	zp/src/zero_padding_gen \
	mult/src/complex_mult \
	divide_by_constant/src/divide_by_constant \
	set_clr/src/set_clr)

# Remember two rules for OpenCPI primitive libraries, in order to be usable with all tools:
# 1. Any entity (VHDL) or module (verilog) must have a VHDL component declaration in util_prims_pkg.vhd
# 2. Entities or modules to be used from outside the library must have the file name
#    be the same as the entity/module name, and one entity/module per file.

include $(OCPI_CDK_DIR)/include/hdl/hdl-library.mk
