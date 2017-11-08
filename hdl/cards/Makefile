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

# This directory contains a library of cards and associated device workers
# Thus when support for a new card is done, it is done here.
# Slot type specs are in the "specs" directory.
# This directory should only depend on the "primitives" library

Workers = zipper_i2c.hdl \
          zipper_i2c_em.hdl \
          fmcomms3_i2c.hdl \

include $(OCPI_CDK_DIR)/include/library.mk
all: hdl
