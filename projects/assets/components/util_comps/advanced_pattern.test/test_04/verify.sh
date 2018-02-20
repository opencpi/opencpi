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
echo Checking for application timeout
grep "finished after waiting" ocpirun.log
echo Checking that output file is only zero
LINES=$(od -x odata/output_000.out | grep -v "0000000 0000 0000 0000 0000 0000 0000 0000 0000" | egrep -v '^\*$' | wc -l)
echo 1 = $LINES ?
[ $LINES -eq 1 ]
echo Modifying dump files...
cut -d, -f5- fwout.current.dump > fwout.current.dump.cut
cut -d, -f5- UUT.current.dump > UUT.current.dump.cut
echo Checking golden files
md5sum -c golden.md5
