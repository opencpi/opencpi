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

The Application XML sets the phase increment of the Mixer/NCO to output a 1 Hz tone,
sampled at 16 Hz. The output waveform's frequency is expected to +1 Hz greater than
the input waveform frequency. The validation of the test involves performing an FFT
of the output file to ensure the max tone is located at DC or 0 Hz.
"""
import struct
import numpy as np
import sys
import os.path

if len(sys.argv) != 6:
    print("Invalid arguments:  usage is: generate.py <sample-freq> <target-freq> <amplitude> <num-samples> <output-file>")
    sys.exit(1)
print "    GENERATE (I/Q 16b binary data file):"

ofilename = sys.argv[5]
# Create cosine & sine waveforms
Fs = float(sys.argv[1])     # sample frequency
Ts = 1.0/float(Fs);     # sampling interval
Ft = float(sys.argv[2])     # target frequency
AMPLITUDE = int(sys.argv[3])
NUM_SAMPLES = int(sys.argv[4]) # number of complex samples
t = np.arange(0,NUM_SAMPLES*Ts,Ts,dtype=np.float)   # time vector
# Generate I/Q samples @ target freq
real = np.cos(2*np.pi*Ft*t)
imag = np.sin(2*np.pi*Ft*t)
# Initialize empty array, sized to store 16b I/Q samples
z = np.array(np.zeros(NUM_SAMPLES), 
             dtype=np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)})))
# Set the gain
gain_i = AMPLITUDE / max(abs(real))
gain_q = AMPLITUDE / max(abs(imag))
z['real_idx'] = np.int16(real * gain_i)
z['imag_idx'] = np.int16(imag * gain_q)

# Save data to file
f = open(ofilename, 'wb')
for i in xrange(NUM_SAMPLES):
    f.write(z[i])
f.close()

# Summary
print '      # of Bytes:', NUM_SAMPLES*4
print '      # of I/Q (16b) samples:', NUM_SAMPLES/2
print '      Sample Frequency:', Fs
print '      Target Frequency:', Ft
print '      Amplitude:', AMPLITUDE
