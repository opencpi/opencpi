#!/bin/bash
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

set -e
echo Checking if forced to stop...
grep -L "finished after waiting" ocpirun.log | grep ocpirun.log
# Trim mid-way dumps to match the main ones
dd if=odata/myoutput_164_mid.out of=odata/myoutput_164_mid.out.cut bs=7992 count=1
dd if=odata/myoutput_242_mid.out of=odata/myoutput_242_mid.out.cut bs=8000 count=1
md5sum -c golden.md5
