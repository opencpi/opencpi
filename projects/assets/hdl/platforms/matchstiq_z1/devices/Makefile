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
# It is similar to the components directory except that all of the workers
# are device specific - each worker controls some piece of hardware on a
# chip or attached to a chip.

# Currently, all tests except matchstiq_z1_gp_out.test are disabled because they
# need specific setups with a signal generator, etc.
Tests = matchstiq_z1_gp_out.test

# Build Ordering Rules:
# 1) Subdevices that support other devices must occur AFTER the devices they support.
# 2) Similarly, Emulators must occur after the devices they emulate.
Workers = \
	matchstiq_z1_pca9535_proxy.rcc \
	matchstiq_z1_avr.hdl \
	matchstiq_z1_avr_proxy.rcc \
	matchstiq_z1_i2c.hdl \
	matchstiq_z1_rx.rcc \
	matchstiq_z1_tx.rcc \
	matchstiq_z1_gp_out.hdl \
	matchstiq_z1_gp_out_em.hdl

include $(OCPI_CDK_DIR)/include/library.mk
