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

Used for testing real/complex FIR filters using ComplexShortWithMetadata
(Only complex binary is currently supported)

To test the FIR filter, a binary data file is generated containing all of the
opcodes of the ComplexShortWithMetadata protocol in the following sequence:
1. Interval
2. Sync (this opcode is expected after an Interval opcode)
3. Time
4. Samples (impulse with length numtaps*2)
4. Samples (impulse with length numtaps*2)
5. Flush
6. Samples (impulse with length numtaps*2)
7. Sync
8. Samples (impulse with length numtaps*2)

"""
import struct
import array
import numpy as np
import shutil
import sys
import os.path

SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4

def addmsg(f, opcode, data):
    f.write(struct.pack("II",4*len(data),opcode))
    if len(data):
        f.write(data)

# def impulse_real_ascii(filename): #REAL - ASCII
#     print "\n*** Python: Generate an ASCII impulse file, real ***"
#     fo = open(filename, 'w')
#     scnt=int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
#     max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
#     fo.write(''.join(str(max_tap))+'\n')
#     for i in range(1,scnt):
#         fo.write(''.join(str('0'))+'\n')
#     fo.close()

# def impulse_real_bin(filename): #REAL - BINARY
#     print "\n*** Python: Generate a binary impulse file, real ***"
#     #binary, 16b values packed into 32bit word, little-endian
#     scnt=int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
#     max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
#     time_int = int('00000001',16)
#     time_frac = int('5A5A5A51',16)
#     data = np.zeros(scnt, dtype=np.int16)
#     data[0] = max_tap
#     fo = open(filename, 'wb')
#     addmsg(fo, INTERVAL_OPCODE, array.array('I',(int('01010101',16), int('10101010',16))))
#     fo.write(struct.pack("II",0,FLUSH_OPCODE))
#     fo.write(struct.pack("II",0,SYNC_OPCODE))
#     for j in range(scnt/2):
#         addmsg(fo, TIME_OPCODE, array.array('I',(time_int, time_frac)))
#         time_int = time_int + 1
#         time_frac = time_frac + 1
#         addmsg(fo, SAMPLES_OPCODE, data)
#     fo.write(struct.pack("II",0,DONE_OPCODE))
#     fo.close()

# def impulse_cmplx_ascii(filename): #COMPLEX - ASCII
#     print "\n*** Python: Generate an ASCII impulse file, complex ***"
#     fo = open(filename, 'w')
#     scnt=2*int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
#     max_tap=pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH_p"))-1)-1
#     fo.write(''.join(str(max_tap))+'\n')
#     fo.write(''.join(str(max_tap))+'\n')
#     for i in range(2,scnt):
#         fo.write(''.join(str('0'))+'\n')
#     fo.close()

def impulse_cmplx_bin(filename): #COMPLEX - BINARY
    print("\n*** Python: Generate a binary impulse file, complex ***")
    #binary, 16b values packed into 32bit word, little-endian
    scnt=int(os.environ.get("OCPI_TEST_NUM_TAPS"))
    coeff_width = int(os.environ.get("OCPI_TEST_COEFF_WIDTH"))
    data = np.zeros(scnt, dtype=np.int32)
    if coeff_width == 16: 
        data[0] = 0x7fff7fff
    elif coeff_width == 8:
        data[0] = 0x007f007f
    else:
        print('  FAILED: Unsupported coefficient width: ' + str(coeff_width))
        sys.exit(1)        
    fo = open(filename, 'wb')
    addmsg(fo, INTERVAL_OPCODE, array.array('I',(int('00000000',16), int('0000008C',16)))) #30.72 MHz
    fo.write(struct.pack("II",0,SYNC_OPCODE))
    addmsg(fo, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    addmsg(fo, SAMPLES_OPCODE, data)
    addmsg(fo, SAMPLES_OPCODE, data)
    fo.write(struct.pack("II",0,FLUSH_OPCODE))
    addmsg(fo, SAMPLES_OPCODE, data)
    fo.write(struct.pack("II",0,SYNC_OPCODE))
    addmsg(fo, SAMPLES_OPCODE, data)
    fo.close()

def main():
    #impulse_real_ascii(filename)
    #impulse_real_bin(filename)
    #impulse_cmplx_ascii(filename)
    impulse_cmplx_bin(filename)

if __name__ == '__main__':
    main()
