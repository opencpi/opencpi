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

HdlLibraries=bsv fixed_float ocpi util
# We can't successfully elaborate this since it references the metadata stuff
HdlNoElaboration=yes
# FIXME FIXME: The low level primitives (FIFO etc.) are COPIED here due to an XST bug
# that doesn't seem to find verilog modules in libraries when the elaboration is vhdl->verilog->verilog...
# mkOCCP.v occp_rv.vhd
SourceFiles=\
platform_pkg.vhd platform_body.vhd \
time_client_rv.vhd \
mkTLPSerializer.v metadata_rv.vhd sim_dcp.vhd sim_clk.vhd \
wci_master.vhd ocscp_rv.vhd \
mkPktFork.v mkPktMerge.v mkTLPSM.v unoc_node_rv.vhd \
mkSimIO.v mkSimDCP.v \
FIFO1.v FIFO2.v MakeResetA.v SyncResetA.v SyncHandshake.v arSRLFIFOD.v
include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk
