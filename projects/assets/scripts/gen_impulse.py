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

""" Generate real/complex impulse data files (ASCII and binary)

Used for testing real/complex FIR filters

"""

import struct
import numpy as np
import shutil
import sys
import os.path

def impulse_real_ascii(filename): #REAL - ASCII
    fo = open(filename, 'w')
    scnt=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    # Taps are signed 16 bits so max value is (2^15)-1
    max_tap=pow(2,15)-1
    fo.write(''.join(str(max_tap))+'\n')
    for i in range(1,scnt):
        fo.write(''.join(str('0'))+'\n')
    fo.close()

def impulse_real_bin(filename): #REAL - BINARY
    #binary, 16b values packed into 32bit word, little-endian
    scnt=int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    data = np.zeros(scnt, dtype=np.int32)
    # (2^15)-1 is 7fff in hex which is the max tap value
    data[0] = 0x00007fff
    fo = open(filename, 'wb')
    for i in range(scnt):
        fo.write(data)
    fo.close()

def impulse_cmplx_ascii(filename): #COMPLEX - ASCII
    fo = open(filename, 'w')
    scnt=2*2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    # Taps are signed 16 bits so max value is (2^15)-1
    max_tap=pow(2,15)-1
    fo.write(''.join(str(max_tap))+'\n')
    fo.write(''.join(str(max_tap))+'\n')
    for i in range(2,scnt):
        fo.write(''.join(str('0'))+'\n')
    fo.close()

def impulse_cmplx_bin(filename): #COMPLEX - BINARY
    #binary, 16b values packed into 32bit word, little-endian
    scnt=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    data = np.zeros(scnt, dtype=np.int32)
    # (2^15)-1 is 7fff in hex which is the max tap value
    # Since it is a complex number write max tap twice
    data[0] = 0x7fff7fff
    fo = open(filename, 'wb')
    for i in range(scnt):
        fo.write(data)
    fo.close()


def main():
    impulse_real_ascii(filename)
    impulse_real_bin(filename)
    impulse_cmplx_ascii(filename)
    impulse_cmplx_bin(filename)

if __name__ == '__main__':
    main()
