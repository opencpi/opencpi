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
This script generates files for the input port for the zero_pad unit tests

There are two arguments:
1. The amount to generate in 32-bit words
2. The output file (always last in unit test generation scripts)
All parameter and property values are set in the environment variables: OCPI_TEST_<propname>

This script creates a set of input data for the UUT.
-- The input data consists of 8x unique bytes being 0x0123456789ABCDEF where
-- the sample size is defined by fileType. The sample size defines the byte
-- ordering (endianness) of the input samples. The Zero Padding component reads
-- in a string of bits packed into the input samples.
"""
import struct
import numpy
import sys
import os.path

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: generate.py <number-of-32-bit-words-to-generate> <output-file>")
    sys.exit(1)
num_samples = int(sys.argv[1])
filename = sys.argv[2]
width = int(os.environ.get("OCPI_TEST_DWIDTH_p"));
if width == 64:
    fileType = 'uint64'
    packType = 'L'
    loop_step = 1
    num_samples = num_samples / 2
elif width == 32:
    fileType = 'uint32'
    packType = 'I'
    loop_step = 2
elif width == 16:
    fileType = 'uint16'
    packType = 'H'
    loop_step = 4
    num_samples = num_samples * 2
elif width == 8:
    fileType = 'uint8'
    packType = 'B'
    loop_step = 8
    num_samples = num_samples * 4
else:
    print("Invalid input width!")
    sys.exit(1)

#Create an empty array for the output data
out_data = numpy.array(numpy.zeros(num_samples), dtype=fileType)
ttype = numpy.iinfo(fileType)
#Create a sample of all ones followed by a sample of all zeros
for i in xrange(0,num_samples,loop_step):
    if width == 64:
        out_data[i]   = 0x0123456789ABCDEF
    if width == 32:
        out_data[i]   = 0x01234567
        out_data[i+1] = 0x89ABCDEF
    if width == 16:
        out_data[i]   = 0x0123
        out_data[i+1] = 0x4567
        out_data[i+2] = 0x89AB
        out_data[i+3] = 0xCDEF
    if width == 8:
        out_data[i]   = 0x01
        out_data[i+1] = 0x23
        out_data[i+2] = 0x45
        out_data[i+3] = 0x67
        out_data[i+4] = 0x89
        out_data[i+5] = 0xAB
        out_data[i+6] = 0xCD
        out_data[i+7] = 0xEF
#Convert array to list
out_data_l = out_data.tolist()
#Save data file
f = open(filename, 'wb')
for i in xrange(0,num_samples):
    f.write(struct.pack(packType, out_data_l[i]))
f.close()
