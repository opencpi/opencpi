
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

Implementations = \
	adc.hdl \
	adc.rcc \
	dac.hdl \
	dac.rcc \
	dram_v5.hdl \
	dram_v6.hdl \
	icap.hdl \
	flash.hdl \

#	gbe \ - we need to clean up wti, and deal with multiple control ifcs.

HdlTargets=xc6vlx240t-1-ff1156 xc5vlx50t-1-ff1136 xc5vsx95t-2-ff1136
#  xc5vlx330t-2-ff1738
include $(OCPI_CDK_DIR)/include/lib.mk
