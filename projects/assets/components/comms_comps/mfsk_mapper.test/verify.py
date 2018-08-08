#!/usr/bin/env python3
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
MFSK Mapper: Verify output data

Verify args:
1. Output file used for validation
2. Input file used for comparison

To test the MFSK Mapper, a binary data file is generated containing ramps of 0 to M_p
with a configurable length.

To validate the test, the size and the values of the output file are verified.
"""
import struct
import numpy
import sys
import os.path

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

#Open input file and grab samples as int32
IFILENAME = open(sys.argv[2], 'rb')
idata = numpy.fromfile(IFILENAME, dtype=numpy.uint8, count=-1)
IFILENAME.close()

#Open output file and grab samples as int32 
OFILENAME = open(sys.argv[1], 'rb')
odata = numpy.fromfile(OFILENAME, dtype=numpy.uint16, count=-1)
OFILENAME.close()

m_p = int(os.environ.get("OCPI_TEST_M_p"))
bits_per_symbol = int(numpy.log2(m_p))
number_of_symbols_in_input_file = len(idata)*8/bits_per_symbol

#Ensure dout is not all zeros
if all(odata == 0):
    print('FAILED, values are all zero')
    sys.exit(1)
#Ensure that dout is the expected amount of data
if len(odata) != number_of_symbols_in_input_file:
    print('FAILED, input file length is unexpected')
    print('Length odata = ', len(odata), 'while expected length is = ', number_of_symbols_in_input_file)
    sys.exit(1)

for i in range(0,len(odata)):
    if odata[i] != numpy.mod(i,m_p):
        print('FAILED at sample:', i, 'with value:', format(odata[i], '#X'))
        sys.exit(1)
