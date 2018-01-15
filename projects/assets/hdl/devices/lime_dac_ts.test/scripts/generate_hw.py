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

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <input-file>")
    sys.exit(1)

#FLUSH     = sys.argv[1]
#BURST     = sys.argv[2]
IFILENAME = sys.argv[1]
SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4
DONE_OPCODE     = 5

#I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
#Thus Q is indexed at byte 0 and I is indexed at byte 2
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,2), 'imag_idx':(np.int16,0)}))
# Create cosine & sine waveforms
Fs = 32                   # sample frequency
Ts = 1.0/float(Fs);       # sampling interval
t = np.arange(0,128-4*Ts,Ts)# time vector
Ft = 1                    # target frequency
AMPLITUDE = 16836
NUM_CYCLES = 125

with open(IFILENAME, "wb") as ifile:
    addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000002',16), int('00000000',16))))

    c = np.cos(2*np.pi*Ft*t)
    s = np.sin(2*np.pi*Ft*t)

    data = np.empty(len(c), dtype=dt_iq_pair)

    data['real_idx'] = np.int16(AMPLITUDE*c)
    data['imag_idx'] = np.int16(AMPLITUDE*s)

    for i in range(0,int(NUM_CYCLES)):
        addmsg(ifile, SAMPLES_OPCODE, data)

    addmsg(ifile, TIME_OPCODE, array.array('I',(int('00000004',16), int('00000000',16))))

    for i in range(0,int(NUM_CYCLES)):
        addmsg(ifile, SAMPLES_OPCODE, data)
