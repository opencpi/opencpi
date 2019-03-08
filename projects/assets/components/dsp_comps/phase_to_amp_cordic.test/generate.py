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
Phase to Amplitude CORDIC: Generate a binary data file with a constant value

Generate args:
1. name of output file

To test the Phase to Amplitude CORDIC, two operating modes are tested:
A) When enable=0, the worker configured for 'BYPASS' Mode and simply
passes the 16b real input to the outputs lower 16b [15:0] data lines.
B) When enable=1, a constant (DC) input results in a constant output frequency.
"""
import sys
import os.path
import struct

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)
print "    GENERATE (16b real binary data file):"

CONSTANT_VALUE = int(os.environ.get("OCPI_TEST_CONSTANT_VALUE"))
NUM_SAMPLES = int(os.environ.get("OCPI_TEST_NUM_SAMPLES"))

f = open(sys.argv[1], 'wb')
for x in range(0,NUM_SAMPLES):
    f.write(struct.pack('h', CONSTANT_VALUE))
f.close()

print '    Value of constant: ', CONSTANT_VALUE
print '    Number of bytes: ', NUM_SAMPLES*2
print '    Number of 16b words: ', NUM_SAMPLES
