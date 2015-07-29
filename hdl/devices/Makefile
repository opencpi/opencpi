# #####
#
#  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
#
#    Mercury Federal Systems, Incorporated
#    1901 South Bell Street
#    Suite 402
#    Arlington, Virginia 22202
#    United States of America
#    Telephone 703-413-0781
#    FAX 703-413-0784
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

# This directory contains a library of device workers.
# It is a lot like the components directory except that all of the workers
# are device specific - each worker controls some piece of hardware on a 
# chip or attached to a chip.  Currently we have no notion of anything but
# hdl device workers, although that could (should?) change.

# The names of the device workers are only qualified when we have more than
# one -- until we have a good naming convension.
Package=ocpi.devices
HdlLibraries=platform
# Remember that subdevices that support other devices must occur AFTER the devices they support
# Similarly, emulators must occur after the devices they emulate
Workers = \
	sdp_node.hdl \
	sdp_send.hdl \
	sdp2cp.hdl \
	ocdp.hdl \
	dac.hdl \
	adcx.hdl \
	adc.rcc \
	dac.rcc \
	icap.hdl \
	flash.hdl \
	gbe.hdl \
        lime_dac.hdl \
        lime_adc.hdl \
	lime_adc_em.hdl \
	lime_dac_em.hdl \
	si5351_proxy.rcc \
	si5351.hdl \
	si5351_em.hdl \
	lime_rx.hdl \
	lime_rx_em.hdl \
	lime_tx.hdl \
	lime_tx_em.hdl \
	lime_rx_proxy.rcc \
	lime_tx_proxy.rcc \
        lime_spi.hdl \
	lime_spi_em.hdl \
        time_server.hdl

# These two require support for embedded IOBUFs which is currently turned off
#	dram_v5.hdl \
#	dram_v6.hdl \
#
#        clkgen_ti_cdce72010.hdl \

include $(OCPI_CDK_DIR)/include/lib.mk
