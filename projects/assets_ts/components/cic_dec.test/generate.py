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
CIC Decimator: Generate test input data

Generate args:
1. amplitude of output file (max 16bit signed = 32767)
2. name of output file

The data file is generated containing all of the opcodes of the ComplexShortWithMetadata 
protocol in the following sequence:
1. Interval
2. Sync (this opcode is expected after an Interval opcode)
3. Time
4. Samples
5. Flush
6. Samples
7. Sync
8. Samples

For the samples opcodes, two test cases are used for all of the worker's 
various configurations:
1. Unity Gain Reponse to DC input (constant value)
2. Tones at 50.0, 100.0 and Fs/R Hz, sampled at 1024000 Hz.
 
Only one critical factor was considered when creating the input test data 
for each test case and the configuration of each worker:
- To create enough input test data such that the worker outputted 
multiple data messages (8192 bytes) and the total amount of output data
was large enough to perform quality FFTs for each worker configuration.
"""
import struct
import array
import sys
import os.path
import numpy as np

def addmsg(f, opcode, data):
    f.write(struct.pack("II",4*len(data),opcode)) # Two unsigned 32-bit
    if len(data):
        f.write(data)

def addsamples(f, data, num_cycles,samples_per_message):
    for i in xrange(num_cycles):
        a = 0
        while a < len(data):
            if len(data) - a < samples_per_message/4:
                addmsg(f, SAMPLES_OPCODE, data[a:len(data)])
            else:
                addmsg(f, SAMPLES_OPCODE, data[a:a+samples_per_message/4])
            a+=samples_per_message/4


if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: generate.py <amplitude> <output-file>")
    sys.exit(1)

SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))

# from arguments to generate.py (-test.xml)
AMPLITUDE = int(sys.argv[1])

# from -test.xml (properties that are declared to be test='true')
Ft = int(os.environ.get("OCPI_TEST_TARGET_FREQ"))

print '    UUT:(N=%d, M=%d, R=%d) Test Data:(%d)' % (N,M,R,Ft)

# Select test data to generate: DC or Tones
if (Ft == 0): # Generate DC data
    Fs = float(9600*R)         # sample frequency
    Ts = 1.0 / Fs;             # sampling interval
    t = np.arange(0,1,Ts,dtype=np.float)
    print "      Test data: DC with value =", AMPLITUDE
    real = np.ones((int(Fs),), dtype=np.int16)
    imag = np.ones((int(Fs),), dtype=np.int16)
    gain = AMPLITUDE
    num_cycles = 1
else: # Generate a complex waveform with multiple tones
    Fs = float(1024000)        # sample frequency
    Ts = 1.0 / Fs;             # sampling interval
    t = np.arange(0,1,Ts,dtype=np.float)
    T1 = 50.0    # Within the filter bandwidth
    T2 = 100.0   # Within the filter bandwidth
    T3 = Fs / R  # In the first null of the CIC filter
    Ft = T1
    real = np.cos(2*np.pi*T1*t) + 0.75*np.cos(2*np.pi*T2*t) + 0.25*np.cos(2*np.pi*T3*t)
    imag = np.sin(2*np.pi*T1*t) + 0.75*np.sin(2*np.pi*T2*t) + 0.25*np.sin(2*np.pi*T3*t)
    # Set gain at something less than 32767 (full scale) - i.e. back off to avoid overflow
    gain = AMPLITUDE*0.70 / max(abs(real))
    # Generate enough input data to decimate and produce enough output data to perform quality FFTs
    if R > 2048:
        num_cycles = 50
    elif R == 2048:
        num_cycles = 40
    else:
        num_cycles = 1

# Create complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))
# Initialize empty array, sized to store interleaved I/Q 16bit samples
out_data = np.array(np.zeros(len(real)), dtype=dt_iq_pair)

# Adjust waveform with desired gain
out_data['real_idx'] = np.int16(real * gain)
out_data['imag_idx'] = np.int16(imag * gain)

# Save data to file
message_size=2048
samples_per_message=message_size/4
#There are 3 samples segments during the test. Divide out_data into 3 segments
samples_per_segment=len(out_data)//3
with open(sys.argv[2], 'wb') as f:
    addmsg(f, INTERVAL_OPCODE, array.array('I',(int('00000000',16), int('00001FFF',16)))) #8191
    addmsg(f, SYNC_OPCODE, [])
    addmsg(f, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    addsamples(f, out_data[0:samples_per_segment], int(num_cycles), samples_per_message)
    addmsg(f, FLUSH_OPCODE, [])
    addsamples(f, out_data[samples_per_segment:samples_per_segment*2], int(num_cycles), samples_per_message)
    addmsg(f, SYNC_OPCODE, [])
    addsamples(f, out_data[samples_per_segment*2:len(out_data)], int(num_cycles), samples_per_message)
    
# Summary
print '    Sample Frequency:', Fs
print '    Target Frequency:', Ft
print '    Amplitude:', AMPLITUDE
print '    # of Bytes:', len(out_data)*num_cycles*4
print '    # of I/Q (16b) samples:', len(out_data)*num_cycles
