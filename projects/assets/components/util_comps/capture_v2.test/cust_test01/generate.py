#!/usr/bin/env python
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

import sys
import os.path
import struct

"""
Generate script for testing output not connected
"""

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <numDataWords>")
    sys.exit(1)

filename = 'test.in'
f = open(filename, 'wb')

numDataWords = int(sys.argv[1])

#Fill up data
f.write(struct.pack("<I", numDataWords*4))
f.write(struct.pack("<I", 0))

for x in range(0, numDataWords):
    f.write(struct.pack("<I", x))

f.close()
