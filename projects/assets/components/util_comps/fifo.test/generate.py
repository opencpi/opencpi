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
FIFO: Generate test data

Generate args:
1. Amplitude and length of generated ramp
2. Number of ramps to generate
3. Output file

To test the FIFO, a binary data file is generated containing real 
32-bit samples with a configurable ampltiude and length.
"""
import struct
import array
import numpy as np
import sys
import os.path

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: generate.py <block-size> <num-cycles> <output-file>")
    sys.exit(1)
print "    GENERATE (Real 32b binary data file):"

OFILENAME = sys.argv[3]

# Create ramp
BLOCK_SIZE = int(sys.argv[1])
NUM_CYCLES = int(sys.argv[2])
 
dt_iq_pair=np.dtype((np.uint32, {'imag_idx':(np.int16,2), 'real_idx':(np.int16,0)}))

f = open(OFILENAME, 'wb')
for idx in range(0,int(NUM_CYCLES)):
    I_data = array.array('H',(int(x) for x in xrange(0,BLOCK_SIZE)))
    Q_data = array.array('H',(int(x) for x in xrange(0,BLOCK_SIZE)))
    data = np.empty(len(I_data),dtype=dt_iq_pair)
    data['imag_idx'] = Q_data
    data['real_idx'] = I_data
    f.write(data)
f.close()

# Summary
print "    Output filename: ", OFILENAME
print "    Test data:", NUM_CYCLES, "ramp(s) of size ", BLOCK_SIZE 
print "    Number of bytes: ", BLOCK_SIZE*NUM_CYCLES*4
