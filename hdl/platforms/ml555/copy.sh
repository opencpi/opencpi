#!/bin/sh
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

# Populate from drop (in addition to creating platform.mk by hand)
DROP=$OCPI_HDL_IMPORTS_DIR
# Source files that are specific to this platform
cp $DROP/rtl/mkFTop_ml555.v .
cp $DROP/libsrc/hdl/ocpi/fpgaTop_ml555.v .
cp $DROP/ucf/ml555.ucf .
cp $DROP/ucf/ml555.xcf .

