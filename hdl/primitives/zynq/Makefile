# #####
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
############################################################################

# This primitive library contains VHDL primitive modules for zynq platforms.
HdlLibraries=fixed_float ocpi platform

XilinxZynqCore=processing_system7_v4_03_a
ifndef OCPI_XILINX_EDK_DIR
OCPI_XILINX_EDK_DIR=$(OCPI_XILINX_TOOLS_DIR)/EDK
endif
XilinxZynq=$(OCPI_XILINX_EDK_DIR)/hw/XilinxProcessorIPLib/pcores/$(XilinxZynqCore)/hdl/verilog
# Since the wrapper file is verilog. we'll provide the verilog PS7
#XilinxPS7=$(OCPI_XILINX_TOOLS_DIR)/ISE/verilog/src/unisims/PS7.v
XilinxZynqPSFiles=atc aw_atc b_atc processing_system7 trace_buffer w_atc

SourceFiles=\
  $(XilinxPS7) \
  $(XilinxZynqPSFiles:%=$(XilinxZynq)/%.v) \
  zynq_pkg.vhd unoc2axi_wd.vhd unoc2axi_rd.vhd unoc2axi.vhd axinull.vhd \
  zynq_ps.vhd axi2cp.vhd

include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk
