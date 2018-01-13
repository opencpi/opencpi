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

# This directory contains a library of device workers.
# It is a lot like the components directory except that all of the workers
# are device specific - each worker controls some piece of hardware on a 
# chip or attached to a chip.  Currently we have no notion of anything but
# hdl device workers, although that could (should?) change.

# The names of the device workers are only qualified when we have more than
# one -- until we have a good naming convension.
#
# Remember that subdevices that support other devices must occur AFTER the devices they support
# Similarly, emulators must occur after the devices they emulate
Workers += \
	sdp_node.hdl \
	sdp_pipeline.hdl \
	sdp_send.hdl \
	sdp_receive.hdl \
	sdp2cp.hdl \
	sdp_term.hdl \
	sma.hdl \
	unoc2cp.hdl \
	unoc_term.hdl \
	ocdp.hdl \
	time_server.hdl

include $(OCPI_CDK_DIR)/include/library.mk
