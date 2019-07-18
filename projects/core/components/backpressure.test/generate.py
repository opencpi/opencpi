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
Back Pressure: Generate test input data

This worker is primarily used during the development of an HDL worker,
specifically during unit test simulations, but hardware testing is possible.
It is built into a worker's unit test HDL assembly and is used to force
'backpressure' during the execution of application. Thus providing a better
representation of the backpressure that a worker might experience in a
real hardware environment.

This worker does not manipulate the data, but simply passes it through.
Validation of this worker, requires passing a known input data pattern
through the worker, under its various modes and comparing the input and
output files to verify that the data is unchanged.

Because this worker does not manipulate the data and validation of the
output is performed simply by comparing to the input, any non-zero input
data would be sufficient. Due to its simplicity, and usage in other unit
tests, a binary data file is generated containing complex signed 16-bit
samples with a tone at a configurable center frequency and sample
frequency.

Generate args:
1. sample frequency of generated sinusoid in Hz
2. target frequency of generated sinusoid in Hz
3. amplitude of generated sinusoid
4. number of sinusoid cycles generated
5. name of output file
"""
import sys
import os.path
import numpy as np

if len(sys.argv) != 6:
    print("Invalid arguments:  usage is: generate.py <sample-freq> <target-freq> <amplitude> <num-samples> <output-file>")
    sys.exit(1)
print "    GENERATE (I/Q 16b binary data file):"

ofilename = sys.argv[5]
# Create cosine & sine waveforms
Fs = float(sys.argv[1])     # sample frequency
Ts = 1.0/float(Fs);         # sampling interval
Ft = int(sys.argv[2])       # target frequency
AMPLITUDE = int(sys.argv[3])
NUM_SAMPLES = int(sys.argv[4]) # number of complex samples
t = np.arange(0,NUM_SAMPLES*Ts,Ts,dtype=np.float)   # time vector

# Generate 1 cycle of a complex tone
real = np.cos(2*np.pi*Ft*t)
imag = np.sin(2*np.pi*Ft*t)

# Initialize empty array, sized to store interleaved I/Q 16bit samples
z = np.array(np.zeros(NUM_SAMPLES),
             dtype=np.dtype((np.uint32, {'real_idx':(np.int16,2), 'imag_idx':(np.int16,0)})))

# Set the gain of the 1 cycle
gain_i = AMPLITUDE / max(abs(real))
gain_q = AMPLITUDE / max(abs(imag))
z['real_idx'] = np.int16(real * gain_i)
z['imag_idx'] = np.int16(imag * gain_q)

# Write desired number of cycles to file
with open(ofilename, 'wb') as f:
    for i in xrange(NUM_SAMPLES):
        f.write(z[i])

# Summary
print '    Sample Frequency:', Fs
print '    Target Frequency:', Ft
print '    Amplitude:', AMPLITUDE
print '    # of Bytes:', NUM_SAMPLES*4
print '    # of I/Q (16b) samples:', NUM_SAMPLES/2
