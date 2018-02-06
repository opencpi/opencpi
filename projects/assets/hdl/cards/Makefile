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

XXTests = \
	fmcomms_2_3_rx.test \
	fmcomms_2_3_tx.test \
	hsmc_debug_header_breakout.test \
	hsmc_loopback.test \
	zipper_i2c.test

# Build Ordering Rules:
# 1) Subdevices that support other devices must occur AFTER the devices they support.
# 2) Similarly, Emulators must occur after the devices they emulate.
XXWorkers = \
	fmcomms_2_3_i2c.hdl \
	fmcomms_2_3_rx.rcc \
	fmcomms_2_3_tx.rcc \
	hsmc_debug_header_breakout.hdl \
	hsmc_loopback.hdl \
	zipper_i2c.hdl \
	zipper_rx.rcc \
	zipper_tx.rcc


include $(OCPI_CDK_DIR)/include/library.mk
