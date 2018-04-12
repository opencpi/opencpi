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
MFSK Mapper: Generate test data

Generate args:
1. Number of bytes to generate
2. Output file

To test the MFSK Mapper, a binary data file is generated containing ramps from 0 to M_p up to the
number of bytes to generate. The total number of bits being written must be equally divisible by 
the number of bits per symbol.

All parameter and property values are set in the environment variables: OCPI_TEST_<propname>

"""
import struct
import numpy
import sys
import os.path

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: generate.py <number-of-bytes-to-generate> <output-file>")
    sys.exit(1)

number_of_bytes_to_write = int(sys.argv[1])
number_of_bits_to_write = number_of_bytes_to_write*8
filename = sys.argv[2]

m_p = int(os.environ.get("OCPI_TEST_M_p"))
bits_per_symbol = int(numpy.log2(m_p))

if numpy.remainder(number_of_bits_to_write,bits_per_symbol) != 0:
    print("Invalid arguments: Number of bits not divisible by bits per symbol")
    sys.exit(1)

string_of_0_to_m = "{:0{}b}".format(0,bits_per_symbol)

for i in range(1,m_p):
    string_of_0_to_m = string_of_0_to_m + "{:0{}b}".format(i,bits_per_symbol)

string_to_write_to_file = string_of_0_to_m

while len(string_to_write_to_file) < number_of_bits_to_write:
     string_to_write_to_file = string_to_write_to_file + string_of_0_to_m

# first split into 8-bit chunks
bit_strings = [string_to_write_to_file[i:i + 8] for i in range(0, len(string_to_write_to_file), 8)]

# then convert to integers
byte_list = [int(b, 2) for b in bit_strings]

with open(filename, 'wb') as f:
    f.write(bytearray(byte_list))
f.close()
