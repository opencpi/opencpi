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
CIC Interpolator: Generate test input data

Generate args:
1. amplitude of output file (max 16bit signed = 32767)
2. name of output file

Two test cases are used for all of the worker's various configurations:
1. Unity Gain Reponse to DC input (constant value)
2. A tone at 50.0 Hz sampled @ 1024000/R.
 
A factor which was considered when the amount of input test data was generated
was the total amount of output data was large enough to perform quality FFTs.
"""
import sys
import os.path
import numpy as np

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: generate.py <amplitude> <output-file>")
    sys.exit(1)
print "    GENERATE (I/Q 16b binary data file):"

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))

# from arguments to generate.py (-test.xml)
AMPLITUDE = int(sys.argv[1])

# from -test.xml (properties that are declared to be test='true')
Ft = int(os.environ.get("OCPI_TEST_TARGET_FREQ"))

print '    UUT:(N=%d, M=%d, R=%d) Test Data:(%d)' % (N,M,R,Ft)

# Select test data to generate: DC or a Tone
if (Ft == 0): # Generate DC data
    Fs = float(256)            # sample frequency
    Ts = 1.0 / Fs;             # sampling interval
    t = np.arange(0,1,Ts,dtype=np.float)
    T1 = 0.0
    print "      Test data: DC with Amplitude=%d" % AMPLITUDE
    real = np.ones((int(Fs),), dtype=np.int16)
    imag = np.ones((int(Fs),), dtype=np.int16)
    gain = AMPLITUDE
    num_cycles = 1
else: # Generate a complex waveform with a tone
    Fs = float(1024000 / R)    # sample frequency
    Ts = 1.0 / Fs;             # sampling interval
    t = np.arange(0,1,Ts,dtype=np.float)
    T1 = 50.0                  # frequency of target signal
    print "      Test data: Tone=%.1f Hz" % T1
    real = np.cos(2*np.pi*T1*t)
    imag = np.sin(2*np.pi*T1*t)
    # Set gain at something less than 32767 (full scale)
    gain = AMPLITUDE*0.70 / max(abs(real))
    # Generate enough input data to interpolate and produce enough output data to perform quality FFTs
    # (may need more data for larger interpolation factors)
    if R >= 2048:
        num_cycles = 50
    else:
        num_cycles = 1

# Define complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))
# Initialize empty array, sized to store interleaved I/Q 16bit samples
out_data = np.array(np.zeros(len(real)), dtype=dt_iq_pair)

# Adjust waveform with desired gain
out_data['real_idx'] = np.int16(real * gain)
out_data['imag_idx'] = np.int16(imag * gain)

# Save data to file
f = open(sys.argv[2], 'wb')
for i in range(0,int(num_cycles)):
    for x in xrange(len(out_data)):
        f.write(out_data[x])
f.close()

# Summary
print '    Sample Frequency:', Fs
print '    Target Frequency:', T1
print '    Amplitude:', gain
print '    # of Bytes:', len(out_data)*num_cycles*4
print '    # of I/Q (16b) samples:', len(out_data)*num_cycles
