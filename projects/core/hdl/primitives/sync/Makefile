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

# These need to be in order for most tools that aren't smart about figuring it out.
SourceFiles= \
	sync_pkg.vhd sync_body.vhd \
	sync_bit.vhd sync_bit_ack.vhd \
	sync_pulse.vhd sync_pulse_handshake.vhd \
	sync_bits_feedback.vhd \
	sync_fifo_1xn.vhd sync_fifo_1x0.vhd sync_fifo_2xn.vhd sync_fifo_2x0.vhd \
	sync_sync_rst.vhd sync_async_rst.vhd \
	sync_srl_fifo.vhd sync_fifo_gray.vhd \
#	sync_edge_pulse.vhd \  (shadowing is not currently supported)

include $(OCPI_CDK_DIR)/include/hdl/hdl-lib.mk # we are a library of primitives
