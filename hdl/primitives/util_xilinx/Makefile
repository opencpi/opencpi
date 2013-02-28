#
#  This file is part of OpenCPI (www.opencpi.org).
#     ____                   __________   ____
#    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
#   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
#  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
#  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
#      /_/                                             /____/
#
#  OpenCPI is free software: you can redistribute it and/or modify
#  it under the terms of the GNU Lesser General Public License as published
#  by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  OpenCPI is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU Lesser General Public License for more details.
#
#  You should have received a copy of the GNU Lesser General Public License
#  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
#
########################################################################### #

# This library is for v6-specific primitives, that may shadow things in other libraries
# Pick only what we need for now since the libsrc/hdl/ocpi is a kitchen sink
SimLibraries=$(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/unisims
SimFiles=$(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/glbl.v
OnlyTargets=xilinx
Imports=$(foreach i,bsv/BRAM1 bsv/fBRAM1BE ocpi/ddrOutput2 ocpi/ddrInput2 ocpi/IDELAYCTRL_GRP,\
	  $(OCPI_HDL_IMPORTS_DIR)/libsrc/hdl/$i.v)
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk # we are a library of primitives



