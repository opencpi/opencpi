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



Fs_in=3072;
Fs_out=$((3072*$OCPI_TEST_R));

if (($OCPI_TEST_TARGET_FREQ == 1)) ; then
    Fs_in=$((1024000/$OCPI_TEST_R));
    Fs_out=1024000;
fi

isize=$(du -sb $2 | awk '{ print $1 }')
if (($OCPI_TEST_TARGET_FREQ == 1 && $isize >= 1000000)) ; then
    isize=1000000;
fi
osize=$(du -sb $1 | awk '{ print $1 }')
if (($OCPI_TEST_TARGET_FREQ == 1 && $osize >= 4096000)) ; then
    osize=4096000;
fi

$OCPI_PROJECT_DIR/scripts/plotAndFft.py $2 complex $(($isize/4)) $Fs_in  &
$OCPI_PROJECT_DIR/scripts/plotAndFft.py $1 complex $(($osize/4)) $Fs_out &

