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

HdlLibraries=ocpi fixed_float platform util axi

# Zynq/PS7 uses 32 bits of address space for the PS slave AXI HP
SourceFilesAddr32=sdp_axi_pkg.vhd sdp2axi_ent.vhd
# ZynqMP/PS8 requires 36 bits to access full DDR for the PS slave AXI HP
SourceFilesAddr36=sdp_axi_addr36_pkg.vhd sdp2axi_addr36_ent.vhd

# For ZynqMP (zynq_ultra), use the files that define axi_in to have 36 address bits. For all other targets assume 32 bits
SourceFiles=sdp_pkg.vhd \
  $(if $(filter zynq_ultra,$(HdlTargets)),\
    $(SourceFilesAddr36),\
    $(SourceFilesAddr32)) \
    sdp_body.vhd unoc2sdp.vhd sdp_sim.vhd \
    sdp2axi_rd.vhd sdp2axi_wd.vhd sdp2axi.vhd sdp2axi_rd.vhd

include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk
