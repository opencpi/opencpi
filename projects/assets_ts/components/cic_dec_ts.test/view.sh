#!/bin/bash --noprofile
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


#$OCPI_PROJECT_DIR/scripts/plotAndFft.py $2 complex $((2400*$OCPI_TEST_R)) 16384&
#$OCPI_PROJECT_DIR/scripts/plotAndFft.py $2 complex $((9600*$OCPI_TEST_R)) 16384&
$OCPI_PROJECT_DIR/scripts/plotAndFft.py $2 complex $((9600*$OCPI_TEST_R)) 100000 true&

#$OCPI_PROJECT_DIR/scripts/plotAndFft.py $1 complex $((9600/4)) $((16384/$OCPI_TEST_R)) &
#$OCPI_PROJECT_DIR/scripts/plotAndFft.py $1 complex $((9600*2/4)) $((16384/$OCPI_TEST_R)) &
$OCPI_PROJECT_DIR/scripts/plotAndFft.py $1 complex $((9600*2/4)) $((100000/$OCPI_TEST_R)) true&
