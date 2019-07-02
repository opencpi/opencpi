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
IQ Imbalance Fixer: Generate test data

Generate args:
1. Number of complex signed 16-bit samples to generate
2. Output file

To test the IQ Imbalance Fixer, a binary data file is generated containing complex
signed 16-bit samples with tones at 5Hz, 13Hz, and 27Hz. A phase offset of 10
degrees is applied to the Q channel, and then different gain amounts between the
I and Q rails, which result in a spectral image in the range of -Fs/2 to DC.
"""
import numpy as np
import sys

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: generate.py <num-samples> <output-file>")
    sys.exit(1)

OFILENAME = sys.argv[2]
NUM_SAMPLES = int(sys.argv[1])

dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

#Create an input file with three complex tones
#Tones are at 5 Hz, 13 Hz, and 27 Hz; Fs=100 Hz
Tone05 = 5
Tone13 = 13
Tone27 = 27
Fs = 100
Ts = 1.0/float(Fs)
t = np.arange(0,NUM_SAMPLES*Ts,Ts,dtype=np.float)
#Define a phase offset of 10 degrees and apply to the Q rail
phase_offset = 10*np.pi/180
real = np.cos(Tone05*2*np.pi*t) + np.cos(Tone13*2*np.pi*t) + np.cos(Tone27*2*np.pi*t)
imag = np.sin(Tone05*2*np.pi*t+phase_offset) + np.sin(Tone13*2*np.pi*t+phase_offset) + np.sin(Tone27*2*np.pi*t+phase_offset)
out_data = np.array(np.zeros(NUM_SAMPLES), dtype=dt_iq_pair)
#pick a gain at something less than 32767 (full scale) - i.e. back off to avoid overflow
#using a different gain for each rail causes an I/Q spectral image in addition to the phase offset
gain_i = 31000 / max(abs(real))
gain_q = 31000 / max(abs(imag))
out_data['real_idx'] = np.int16(real * gain_i)
out_data['imag_idx'] = np.int16(imag * gain_q)

#Save data file
f = open(OFILENAME, 'wb')
for i in range(NUM_SAMPLES):
    f.write(out_data[i])
f.close()

#Summary
print ("    Output filename: ", OFILENAME)
print ("    Number of samples: ", NUM_SAMPLES)
print ("    Number of bytes: ", NUM_SAMPLES*4)
