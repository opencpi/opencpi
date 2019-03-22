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
This script verifies files for the output port for the zero_pad unit tests

There are two arguments:
1. The amount to generate in 32-bit words
2. The output file to verify (always last in unit test verification scripts)
All parameter and property values are set in the environment variables: OCPI_TEST_<propname>

Verification:
- numZeros 0s of size dwidth are inserted between input samples.
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
width = int(os.environ.get("OCPI_TEST_DWIDTH_p"));
numZeros = int(os.environ.get("OCPI_TEST_num_zeros"))
numSamps = int(sys.argv[1])
if width == 64:
    fileType = 'uint64'
    numSamps = numSamps / 2
elif width == 32:
    fileType = 'uint32'
elif width == 16:
    fileType = 'uint16'
    numSamps = numSamps * 2
elif width == 8:
    fileType = 'uint8'
    numSamps = numSamps * 4
else:
    print("Invalid output width!")
    sys.exit(1)

#Read all of input data file as int8/int16/int32/int64 samples
fx = open(sys.argv[2], 'rb')
dout = numpy.fromfile(fx, dtype=fileType, count=-1)
#Ensure dout is not all zeros
if all(dout == 0):
    print color.RED + color.BOLD + 'FAILED, values are all zero' + color.END
    sys.exit(100)
#Ensure that dout is the expected amount of data
if len(dout) != numSamps * (numZeros+1):
    print color.RED + color.BOLD + 'FAILED, input file length is unexpected' + color.END
    print color.RED + color.BOLD + 'Length dout = ', len(dout), 'while expected length is = ' + color.END, numSamps * (numZeros+1)
    sys.exit(101)

#numZeros 8/16/32/64-bit zeros are placed between input samples
#Verification is below
myPattern = 0x0123456789ABCDEF
ttype = numpy.iinfo(fileType)
firstIdx=64/width-1
idx=firstIdx
for i in xrange(0,len(dout)):
    #print color.RED + color.BOLD + 'dout[i] =' + hex(dout[i])
    #print color.RED + color.BOLD + 'i mod numZeros+1 =', i % (numZeros+1)
    if not(i % (numZeros+1)):
        #print color.RED + color.BOLD + 'myPattern >> idx*width&ttype.max =', hex(myPattern >> idx*width&ttype.max)
        if dout[i] != myPattern >> idx*width&ttype.max:
            print color.RED + color.BOLD + '#1.FAILED at sample:', i, 'with value:' + color.END, format(dout[i], '#X')
            print color.RED + color.BOLD + '*** Error:End validation ***\n' + color.END
            sys.exit(102)
        if idx == 0:
            idx=firstIdx
        else:
            idx-=1
    else:
         if dout[i] != 0:
            print color.RED + color.BOLD + '#2.FAILED at sample:', i, 'with value:' + color.END, format(dout[i], '#X')
            print color.RED + color.BOLD + '*** Error:End validation ***\n' + color.END
            sys.exit(102)
