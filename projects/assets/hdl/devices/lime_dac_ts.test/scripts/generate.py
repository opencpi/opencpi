#!/usr/bin/env python
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

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: generate.py <flush> <burst> <input-file>")
    sys.exit(1)

FLUSH     = sys.argv[1]
BURST     = sys.argv[2]
IFILENAME = sys.argv[3]
SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4
DONE_OPCODE     = 5

ofile_data_only=open("idata/input_data_only.bin", "wb")

#I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
#Thus Q is indexed at byte 0 and I is indexed at byte 2
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,2), 'imag_idx':(np.int16,0)}))

with open(IFILENAME, "wb") as ifile:
    if (BURST == "true"):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00084000',16))))
    i_data = array.array('H',(int(i*16) for i in xrange(0+1,128+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,128+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    if (FLUSH == "false"):
        ofile_data_only.write(data)

    if (BURST == "true"):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00086FFF',16))))
    i_data = array.array('H',(int(i*16) for i in xrange(0+1,32+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,32+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    if (FLUSH == "false"):
        ofile_data_only.write(data)

    if (FLUSH == "true"):
        ifile.write(struct.pack("II",0,FLUSH_OPCODE))

    if (BURST == "true"):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00088FFF',16))))
    i_data = array.array('H',(int(i*16) for i in xrange(0+1,128+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,128+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    if (BURST == "true"):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    i_data = array.array('H',(int(i*16) for i in xrange(0+1,64+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,64+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    if (BURST == "true"):
        addmsg(ifile, INTERVAL_OPCODE, array.array('I',(int('01010101',16), int('01010101',16))))

    if (BURST == "true"):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    i_data = array.array('H',(int(i*16) for i in xrange(0+1,32+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,32+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    if (BURST == "true"):
        addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000000',16), int('0008E000',16))))
    i_data = array.array('H',(int(i*16) for i in xrange(0+1,64+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,64+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    i_data = array.array('H',(int(i*16) for i in xrange(0+1,32+1)))
    q_data = array.array('H',(int(i*16) for i in xrange(0+1+3840,32+1+3840)))
    data = np.empty(len(i_data), dtype=dt_iq_pair)
    data['real_idx'] = q_data #this is backwards but works
    data['imag_idx'] = i_data
    addmsg(ifile, SAMPLES_OPCODE, data)
    ofile_data_only.write(data)

    ifile.write(struct.pack("II",0,DONE_OPCODE))

ofile_data_only.close()
