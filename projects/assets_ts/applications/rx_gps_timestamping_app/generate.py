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
RX app: Generate test input data (derived from cic_dec.test/generate.py)

Generate args:
1. amplitude of output file (max 16bit signed = 32767)
2. sample frequency in Hz
3. CIC decimation rate
4. tune frequency of complex mixer
5. name of output file

The data file is generated containing with opcodes of the ComplexShortWithMetadata 
protocol in the following sequence:
1. Interval
2. Sync (this opcode is expected after an Interval opcode)
3. Time
4. Samples

Tones at 50.0, 100.0 and Fs/R Hz, sampled at 1024000 Hz.
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


if len(sys.argv) != 6:
    print("Invalid arguments:  usage is: generate.py <amplitude> <sample-frequency> <R> <tune-frequency> <output-file>")
    sys.exit(1)

#ComplexShortWithMetadata opcodes
SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4

# from arguments to generate.py
AMPLITUDE = int(sys.argv[1])
Fs = float(int(sys.argv[2]))          # sample frequency
R = int(sys.argv[3])
Ftune = int(sys.argv[4])
Ts = 1.0 / Fs;                        # sampling interval
t = np.arange(0,1,Ts,dtype=np.float)
T1 = 50.0 + Ftune    # Within the filter bandwidth
T2 = 100.0 + Ftune   # Within the filter bandwidth
T3 = Fs / R + Ftune  # In the first null of the CIC filter
Ft = T1
real = np.cos(2*np.pi*T1*t) + 0.75*np.cos(2*np.pi*T2*t) + 0.25*np.cos(2*np.pi*T3*t)
imag = np.sin(2*np.pi*T1*t) + 0.75*np.sin(2*np.pi*T2*t) + 0.25*np.sin(2*np.pi*T3*t)
# Set gain at something less than 32767 (full scale) - i.e. back off to avoid overflow
gain = AMPLITUDE*0.70 / max(abs(real))
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
interval=(1/Fs)*pow(2,64)
interval_msb=long(interval)>>32
interval_lsb=long(interval)&0xFFFFFFFF
with open(sys.argv[5], 'wb') as f:
    addmsg(f, INTERVAL_OPCODE, array.array('I',(interval_msb,interval_lsb)))
    addmsg(f, SYNC_OPCODE, [])
    addmsg(f, TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    addsamples(f, out_data, int(num_cycles), samples_per_message)
    
# Summary
print( '    Sample Frequency:', Fs)
print( '    Target Frequency:', Ft)
print( '    Amplitude:', AMPLITUDE)
print( '    # of Bytes:', len(out_data)*num_cycles*4)
print( '    # of I/Q (16b) samples:', len(out_data)*num_cycles)
