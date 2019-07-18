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
DEPRECATION NOTICE: This worker is deprecated and will be removed in OpenCPI 2.0. Use the Zero Pad component for new designs.

This script verifies files for the output port for the zero_padding unit tests

There are two arguments:
1. The amount to generate in 32-bit words
2. The output file to verify (always last in unit test verification scripts)
All parameter and property values are set in the environment variables: OCPI_TEST_<propname>

Verification:
- Each "1" input bit is sign-extended as a +1 (almost) of owidth in Qm.n format.
- Each "0" input bit is sign-extended as a -1 of owidth in Qm.n format.
- numZeros 0s of size owidth are inserted between input samples.
"""
import os.path
# import struct
import sys
import opencpi.colors as color
import numpy


if len(sys.argv) != 4:
    if len(sys.argv) != 3:
        print("Invalid arguments:  usage is: verify.py <number-of-32-bit-words-expected> <output-file-to-verify>\n                          or: verify.py <number-of-32-bit-words-expected> <output-file-to-verify> <input-file-to-compare-against>")
    sys.exit(1)
width = int(os.environ.get("OCPI_TEST_ODATA_WIDTH_p"));
if width == 64:
    fileType = 'int64'
elif width == 32:
    fileType = 'int32'
elif width == 16:
    fileType = 'int16'
elif width == 8:
    fileType = 'int8'
else:
    print("Invalid output width!")
    sys.exit(1)
isize = 32
numZeros = int(os.environ.get("OCPI_TEST_num_zeros"))
numSamps = int(sys.argv[1])

#Read all of input data file as int8/int16/int32/int64 samples
fx = open(sys.argv[2], 'rb')
dout = numpy.fromfile(fx, dtype=fileType, count=-1)
#Ensure dout is not all zeros
if all(dout == 0):
    print color.RED + color.BOLD + 'FAILED, values are all zero' + color.END
    sys.exit(100)
#Ensure that dout is the expected amount of data
if len(dout) != numSamps * (numZeros+1) * isize:
    print color.RED + color.BOLD + 'FAILED, input file length is unexpected' + color.END
    print color.RED + color.BOLD + 'Length dout = ', len(dout), 'while expected length is = ' + color.END, numSamps * (numZeros+1) * isize
    sys.exit(101)

#Ones in the input file are unpacked as signed +1 values
#Zeros in the input file are unpacked as signed -1 values
#numZeros 8/16/32/64-bit zeros are placed between input samples
#Verification is below
myPattern = 0x0123456789ABCDEF
count = 63
ttype = numpy.iinfo(fileType)
array_of_zeros = numpy.zeros(numZeros)
array_of_dout = numpy.ones(numZeros)
for i in xrange(0,len(dout)-numZeros,numZeros+1):
    if (myPattern >> count) & 0x1:
        if dout[i] != ttype.max:
            print color.RED + color.BOLD + '#1.FAILED at sample:', i, 'with value:' + color.END, format(dout[i], '#X')
            print color.RED + color.BOLD + '*** Error:End validation ***\n' + color.END
            sys.exit(102)
    else:
        if dout[i] != ttype.min:
            print color.RED + color.BOLD + '#2.FAILED at sample:', i, 'with value:' + color.END, format(dout[i], '#X')
            print color.RED + color.BOLD + '*** Error:End validation ***\n' + color.END
            sys.exit(103)

    # Construct array of dout for bulk comparison
    for j in xrange(1,numZeros+1):
        array_of_dout[j-1] = dout[j]

    if not(numpy.array_equal(array_of_dout,array_of_zeros)):
        print color.RED + color.BOLD + '#3.FAILED at sample:', i+numZeros, 'with value:' + color.END, format(dout[i+numZeros], '#X')
        print color.RED + color.BOLD + '*** Error:End validation ***\n' + color.END
        sys.exit(104)

    if count == 0:
        count = 63
    else:
        count -= 1
