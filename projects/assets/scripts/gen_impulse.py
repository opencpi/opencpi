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

""" Generate real/complex impulse data files (ASCII and binary)

Used for testing real/complex FIR filters

"""
import struct
import shutil
import sys
import os.path

def impulse_real_ascii(filename): #REAL - ASCII
    print "\n*** Python: Generate an ASCII impulse file, real ***"
    fo = open(filename, 'w')
    scnt=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
    fo.write(''.join(str(max_tap))+'\n')
    for i in range(1,scnt):
        fo.write(''.join(str('0'))+'\n')
    fo.close()

def impulse_real_bin(filename): #REAL - BINARY
    print "\n*** Python: Generate a binary impulse file, real ***"
    #binary, 16b values packed into 32bit word, little-endian
    scnt=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
    fo = open(filename, 'wb')
    for j in range(scnt/2):
        fo.write(struct.pack('h', max_tap))
        for i in range(1,scnt):
            fo.write(struct.pack('h', 0))
    fo.close()

def impulse_cmplx_ascii(filename): #COMPLEX - ASCII
    print "\n*** Python: Generate an ASCII impulse file, complex ***"
    fo = open(filename, 'w')
    scnt=2*2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
    fo.write(''.join(str(max_tap))+'\n')
    fo.write(''.join(str(max_tap))+'\n')
    for i in range(2,scnt):
        fo.write(''.join(str('0'))+'\n')
    fo.close()

def impulse_cmplx_bin(filename): #COMPLEX - BINARY
    print "\n*** Python: Generate a binary impulse file, complex ***"
    #binary, 16b values packed into 32bit word, little-endian
    scnt=2*2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
    max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
    fo = open(filename, 'wb')
    for j in range(scnt/2):
        fo.write(struct.pack('h', max_tap))
        fo.write(struct.pack('h', max_tap))
        for i in range(2,scnt):
            fo.write(struct.pack('h', 0))
    fo.close()

def main():
    impulse_real_ascii(filename)
    impulse_real_bin(filename)
    impulse_cmplx_ascii(filename)
    impulse_cmplx_bin(filename)

if __name__ == '__main__':
    main()
