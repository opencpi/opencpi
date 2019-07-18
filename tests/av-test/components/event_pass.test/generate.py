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


"""
Stream Random Metadata: Generate test data
"""
import sys
import struct

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <num-samples> <messages in file? T=1/F=0> <output-file>")
    sys.exit(1)
f = open(sys.argv[1], 'wb')
f.write(struct.pack("I", 0)) # length
f.write(struct.pack("I", 0)) # opcode
f.write(struct.pack("I", 0)) # length
f.write(struct.pack("I", 0)) # opcode
f.write(struct.pack("I", 0)) # length
f.write(struct.pack("I", 0)) # opcode
f.write(struct.pack("I", 0)) # length
f.write(struct.pack("I", 0)) # opcode
f.close()
