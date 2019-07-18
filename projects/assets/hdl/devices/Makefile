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

# This is the hdl/devices library

# All workers created here in *.<model> will be built automatically
# All tests created here in *.test directories will be built/run automatically
# To limit the workers that actually get built, set the Workers= variable
# To limit the tests that actually get built/run, set the Tests= variable

# Any variable definitions that should apply for each individual worker/test
# in this library belong in Library.mk

Tests = \
	lime_dac_ts.test 

# Build Ordering Rules:
# 1) Subdevices that support other devices must occur AFTER the devices they support.
# 2) Similarly, Emulators must occur after the devices they emulate.
Workers = \
	ad7291.hdl \
	ad9361_adc.hdl \
	ad9361_adc_sub.hdl \
	ad9361_config.hdl \
	ad9361_config_proxy.rcc \
	ad9361_dac.hdl \
	ad9361_dac_sub.hdl \
	ad9361_data_sub.hdl \
	ad9361_spi.hdl \
	cstest.hdl \
	flash.hdl \
	gps_uart.hdl \
	lime_adc.hdl \
	lime_dac_ts_em.hdl \
	lime_dac.hdl \
	lime_dac_ts.hdl \
	lime_rx.hdl \
	lime_rx_proxy.rcc \
	lime_spi.hdl \
	lime_tx.hdl \
	lime_tx_proxy.rcc \
	pca9534.hdl \
	pca9535.hdl \
	si5338.hdl \
	si5338_proxy.rcc \
	si5351.hdl \
	si5351_proxy.rcc \
	stm_mc24c02.hdl \
	tmp100.hdl \
	tmp100_proxy.rcc 

include $(OCPI_CDK_DIR)/include/library.mk
