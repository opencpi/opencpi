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
Package=ocpi.devices
# Remember that subdevices that support other devices must occur AFTER the devices they support
# Similarly, emulators must occur after the devices they emulate
# TODO / FIXME - add ad9361_config_proxy.rcc to Workers
Workers += \
        $(and $(call OcpiHavePrereq,ad9361), \
         ad9361_adc.hdl \
          ad9361_adc_sub.hdl \
          ad9361_dac.hdl \
          ad9361_dac_sub.hdl \
          ad9361_config.hdl $(Comment this must be after adc/dac since it supports them) \
          ad9361_config_proxy.rcc \
          ad9361_spi.hdl $(Comment this must come after ad9361_config.hdl since it supports it) \
          ad9361_data_sub.hdl $(Comment this must come last since it supports multiple ad9361 workers)) \
        ad7291.hdl \
        stm_mc24c02.hdl \
	sdp_node.hdl \
	sdp_pipeline.hdl \
	sdp_send.hdl \
	sdp_receive.hdl \
	sdp2cp.hdl \
	sdp_term.hdl \
        unoc2cp.hdl \
        unoc_term.hdl \
	ocdp.hdl \
	flash.hdl \
	lime_dac.hdl \
	lime_adc.hdl \
	si5351_proxy.rcc \
	si5351.hdl \
	si5351_em.hdl \
	lime_rx.hdl \
	lime_tx.hdl \
	lime_rx_em.hdl \
	lime_tx_em.hdl \
	lime_rx_proxy.rcc \
	lime_tx_proxy.rcc \
	lime_spi.hdl \
	lime_spi_em.hdl \
	cstest.hdl \
	time_server.hdl

# These components are old / untested and not shipped (AV-790,AV-877)
ifeq (,$(OCPI_MINIMAL_COMPONENTS_BUILD))
Workers += \
	lime_adc_em.hdl \
	lime_dac_em.hdl \
	dac.hdl \
	adcx.hdl \
	adc.rcc \
	dac.rcc \
	icap.hdl \
	gbe.hdl
endif # OCPI_MINIMAL_COMPONENTS_BUILD

# These two require support for embedded IOBUFs which is currently turned off
#	dram_v5.hdl \
#	dram_v6.hdl \
#
#        clkgen_ti_cdce72010.hdl \

include $(OCPI_CDK_DIR)/include/library.mk
