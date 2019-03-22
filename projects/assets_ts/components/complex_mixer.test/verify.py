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
Complex Mixer: Verify Validate output data

Verify args:
1. sample frequency of output data file
2. output data file used for validation
3. input data file used for comparison

To test the Complex Mixer, a binary data file is generated containing complex
signed 16-bit samples with a tone at a configurable center frequency and sample
frequency. 
The Application XML sets the phase increment of the Mixer/NCO to output a 1 Hz tone,
sampled at 16 Hz. The output waveform's frequency is expected to +1 Hz greater than
the input waveform frequency. The validation of the test involves performing an FFT
of the output file to ensure the max tone is located at DC or 0 Hz.
"""
import numpy as np
import sys
import os.path

def getmsg(m):
    length = m[0]
    opcode = m[1]
    data   = None
    if length > 0:
        data = m[2:length/4+2]
    return (opcode, length, data)

"""
Ex: python verify.py {sample-freq} {ofilename} {ifilename}
Validate:
TEST #1: Verify I & Q values are not all zeros
TEST #2: Output file matches expected size
TEST #3: Two possibilities: 1) NORMAL MODE - Target tone tuned to DC, 2) BYPASS MODE - idata = odata
TEST #4: Output non-sample messages match input non-sample messages
"""

if len(sys.argv) != 4:
    print('Invalid arguments:  usage is: verify.py <sample-freq> <output-file> <input-file>')
    sys.exit(1)

# I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
# Thus Q is indexed at byte 0 and I is indexed at byte 2
dt_iq_pair = np.dtype((np.uint32, {'imag_idx':(np.int16,0),'real_idx':(np.int16,2)}))

sample_rate = float(sys.argv[1])
enable = os.environ.get('OCPI_TEST_enable')
phs_inc = float(os.environ.get('OCPI_TEST_phs_inc'))

#Read input and output data files as uint32
ifile = open(sys.argv[3], 'rb')
din = np.fromfile(ifile, dtype=np.uint32, count=-1)
ifile.close()
ofile = open(sys.argv[2], 'rb')
dout = np.fromfile(ofile, dtype=np.uint32, count=-1)
ofile.close()

# Test that output data is not all zeros
if all(dout == 0):
    print ('FAILED - values are all zero')
    sys.exit(1)
else:
    print ('      PASS - File is not all zeros')

# Test that output data is the expected amount
if len(dout) != len(din):
    print ('FAILED, output file length is unexpected')
    print ('Length ofilename = ', len(dout), 'while expected length is = ', len(din))
    sys.exit(1)
else:
    print ('      PASS - Input and output file lengths match')

#Parse messages within input data
index = 0
msg_count = 0
msg = []
samples = []
din_non_sample_msg = []
while index < len(din):
    msg.append(getmsg(din[index:]))
    if msg[msg_count][2] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][2]) + 2
    msg_count += 1
for i in range(0,len(msg)):
    if(msg[i][0]==0):
        samples.extend(msg[i][2])
    else:
        din_non_sample_msg.append(msg[i])
din_samples_array = np.array(samples, dtype=dt_iq_pair)

#Parse messages within output data
index = 0
msg_count = 0
msg = []
samples = []
dout_non_sample_msg = []
while index < len(dout):
    msg.append(getmsg(dout[index:]))
    if msg[msg_count][2] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][2]) + 2
    msg_count += 1
for i in range(0,len(msg)):
    if(msg[i][0]==0):
        samples.extend(msg[i][2])
    else:
        dout_non_sample_msg.append(msg[i])
dout_samples_array = np.array(samples, dtype=dt_iq_pair)
dout_samples_length = len(dout_samples_array)
if(enable == 'true'): # => NORMAL MODE
    complex_data = np.zeros(dout_samples_length, dtype=np.complex)
    complex_data.real = dout_samples_array['real_idx']
    complex_data.imag = dout_samples_array['imag_idx']
    FFT = 1.0/dout_samples_length * abs(np.fft.fft(complex_data,dout_samples_length))
    Max_FFT_freq=np.argmax(FFT)*sample_rate/dout_samples_length
    if Max_FFT_freq != 0.0:
        print ('Fail: Max of FFT occurs at index: ',Max_FFT_freq, 'Hz (Should be 0)')
        sys.exit(1)
    else:
        print ('      PASS - Max of FFT occurs at index: ',Max_FFT_freq, 'Hz')
else: # => BYPASS MODE
    if (din_samples_array != dout_samples_array).all():
        print ('Fail - Bypass Mode: Input and output file do not match')
        sys.exit(1)
    else:
        print ('      PASS - Bypass Mode: Input and output file match')

# Test #4 - Compare non-samples messages between input and output data
if np.array_equiv(din_non_sample_msg, dout_non_sample_msg):
    #print din_non_sample_msg
    #print dout_non_sample_msg
    print ("    FAIL - Input and output non-samples messages do not match")
    sys.exit(1)
else:
    print ("    PASS - Input and output non-samples messages match")
