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


import sys
import os.path
import struct
import array
import numpy as np

def addmsg(f, opcode, data):
    f.write(struct.pack("II",4*len(data),opcode)) # Two unsigned 32-bit
    if len(data):
        f.write(data)

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <input-file>")
    sys.exit(1)

#print(os.environ)
FLUSH     = int(os.environ.get("OCPI_TEST_flush") != "false") #sys.argv[1]
BURST     = int(os.environ.get("OCPI_TEST_burst") != "1") #sys.argv[2]
#print ('FLUSH=' + str(FLUSH) + ', BURST=' + str(BURST))
IFILENAME = sys.argv[1] # sys.argv[3]
SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4
DONE_OPCODE     = 5

ofilename_data_only=sys.argv[1]+".data"
ofile_data_only=open(ofilename_data_only, "wb")

#I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
#Thus Q is indexed at byte 0 and I is indexed at byte 2
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

with open(IFILENAME, "wb") as ifile:
    if (BURST == 1):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00084000',16))))
    i_data = array.array('H',(int(i*16) for i in range(0+1,128+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,128+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    if (FLUSH == 0):
        ofile_data_only.write(data)

    if (BURST == 1):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00086FFF',16))))
    i_data = array.array('H',(int(i*16) for i in range(0+1,32+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,32+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    if (FLUSH == 0):
        ofile_data_only.write(data)

    if (FLUSH == 1):
        ifile.write(struct.pack("II",0,FLUSH_OPCODE))

    if (BURST == 1):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00088FFF',16))))
    i_data = array.array('H',(int(i*16) for i in range(0+1,128+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,128+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    if (BURST == 1):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    i_data = array.array('H',(int(i*16) for i in range(0+1,64+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,64+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    if (BURST == 1):
        addmsg(ifile, INTERVAL_OPCODE, array.array('I',(int('01010101',16), int('01010101',16))))

    if (BURST == 1):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    i_data = array.array('H',(int(i*16) for i in range(0+1,32+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,32+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    if (BURST == 1):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('0008E000',16))))
    i_data = array.array('H',(int(i*16) for i in range(0+1,64+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,64+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    i_data = array.array('H',(int(i*16) for i in range(0+1,32+1)))
    q_data = array.array('H',(int(i*16) for i in range(0+1+3840,32+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = i_data
    data['imag_idx'] = q_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    ifile.write(struct.pack("II",0,DONE_OPCODE))

ofile_data_only.close()
