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

"""
Complex Mixer: Generate test input data & Validate output data

Generate args:
1. sample frequency of generated sinusoid in Hz
2. center frequency of generated sinusoid in Hz
3. amplitude of generated sinusoid
4. number of complex samples to generated
5. output file

To test the Complex Mixer, a binary data file is generated containing complex
signed 16-bit samples with a tone at a configurable center frequency and sample
frequency.

The data file is generated containing all of the opcodes of the ComplexShortWithMetadata 
protocol in the following sequence:
1. Interval
2. Sync (this opcode is expected after an Interval opcode)
3. Time
4. Samples (tone with configurable length num-samples)
5. Flush
6. Samples (tone with configurable length num-samples)
7. Sync
8. Samples (tone with configurable length num-samples)

The Application XML sets the phase increment of the Mixer/NCO to output a 1 Hz tone,
sampled at 16 Hz. The output waveform's frequency is expected to +1 Hz greater than
the input waveform frequency. The validation of the test involves performing an FFT
of the output file to ensure the max tone is located at DC or 0 Hz.
"""
import struct
import array
import numpy as np
import sys
import os.path

def addmsg(f, opcode, data):
    f.write(struct.pack("II",4*len(data),opcode)) # Two unsigned 32-bit
    if len(data):
        f.write(data)

if len(sys.argv) != 6:
    print("Invalid arguments:  usage is: generate.py <sample-freq> <target-freq> <amplitude> <num-samples> <output-file>")
    sys.exit(1)
print ("    GENERATE (I/Q 16b binary data file):")

SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4

ofilename = sys.argv[5]
# Create cosine & sine waveforms
Fs = float(sys.argv[1])     # sample frequency
Ts = 1.0/float(Fs);     # sampling interval
Ft = float(sys.argv[2])     # target frequency
AMPLITUDE = int(sys.argv[3])
NUM_SAMPLES = int(sys.argv[4])
t = np.arange(0,NUM_SAMPLES*Ts,Ts,dtype=np.float)   # time vector
# Generate I/Q samples @ target freq
real = np.cos(2*np.pi*Ft*t)
imag = np.sin(2*np.pi*Ft*t)
# Initialize empty array, sized to store 16b I/Q samples
z = np.array(np.zeros(NUM_SAMPLES), dtype=np.dtype((np.uint32, {'imag_idx':(np.int16,0), 'real_idx':(np.int16,2)})))
# Set the gain
gain_i = AMPLITUDE / max(abs(real))
gain_q = AMPLITUDE / max(abs(imag))
z['real_idx'] = np.int16(real * gain_i)
z['imag_idx'] = np.int16(imag * gain_q)

#Save data file
fo = open(ofilename, 'wb')
addmsg(fo, INTERVAL_OPCODE, array.array('I',(int('00000000',16), int('0000008C',16)))) #30.72 MHz
addmsg(fo, SYNC_OPCODE, [])
addmsg(fo, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
addmsg(fo, SAMPLES_OPCODE, z)
addmsg(fo, FLUSH_OPCODE, [])
addmsg(fo, SAMPLES_OPCODE, z)
addmsg(fo, SYNC_OPCODE, [])
addmsg(fo, SAMPLES_OPCODE, z)
fo.close()
