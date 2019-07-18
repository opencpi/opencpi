#!/usr/bin/env python2
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

import struct
import numpy as np
import sys
import os.path

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <golden-file> <output-file> <input-file>")
    sys.exit(1)

# u4 is uint32 and setting it to little endian with "<"
dt = np.dtype('<u4')

# Open input file and grab samples as uint32
with open(sys.argv[1], 'rb') as f:
    goldendata = np.fromfile(f, dtype=dt)


# Open output file and grab samples as uint32
with open(sys.argv[2], 'rb') as f:
    odata = np.fromfile(f, dtype=dt)

# Ensure that output data is the expected amount of data
if len(odata) != len(goldendata):
    print "    FAILED: Output file length is unexpected"
    print "    Length = ", len(odata), "while expected length is = ", len(goldendata)
    sys.exit(1)
else:
    print "    PASS: Golden and output file lengths match'"

if np.array_equal(goldendata, odata):
    print "    PASS: Golden and output file match"
else:
    print "    FAILED: Golden and output file do not match"
    sys.exit(1)
