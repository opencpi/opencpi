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
DC Offset Filter: Generate test data

Generate args:
1. Number of complex signed 16-bit samples to generate
2. Output file

To test the DC Offset Filter, a binary data file is generated containing complex
signed 16-bit samples with tones at 5Hz, 13Hz, and 27Hz, and with a DC component.
"""
import struct
import numpy as np
import sys
import os.path

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: generate.py <num-samples> <output-file>")
    sys.exit(1)

print ("    GENERATE (Complex 16b binary data file):")
    
OFILENAME = sys.argv[2]
NUM_SAMPLES = int(sys.argv[1])

#Create an input file with three complex tones and a DC component
#Tones are at 5 Hz, 13 Hz, and 27 Hz; Fs=100 Hz
ToneDC = 0
Tone05 = 5
Tone13 = 13
Tone27 = 27
Fs = 100
Ts = 1.0/float(Fs)
t = np.arange(0,NUM_SAMPLES*Ts,Ts,dtype=np.float)
#comment the next two lines, and uncomment the following two random number
#lines, to see the frequency response over the entire range
real = np.cos(Tone05*2*np.pi*t) + np.cos(Tone13*2*np.pi*t) + np.cos(Tone27*2*np.pi*t) + 1
imag = np.sin(Tone05*2*np.pi*t) + np.sin(Tone13*2*np.pi*t) + np.sin(Tone27*2*np.pi*t) + 1
#real = np.random.randn(NUM_SAMPLES,1) + 1
#imag = np.random.randn(NUM_SAMPLES,1) + 1
out_data = np.array(np.zeros(NUM_SAMPLES), dtype=np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)})))
#pick a gain at 70% max value - i.e. back off more than a 1/4 bit to avoid
#internal worker overflow. Note that the amplitude is not centered due to
#the "+1" dc offset argument above in the real/imag declarations. This
#results in complex amplitudes that swing between +23k and -12k within an
#int16. we must use same gain on both rails to avoid I/Q spectral image
gain = 32768*0.70 / max(abs(real))
out_data['real_idx'] = np.int16(real * gain)
out_data['imag_idx'] = np.int16(imag * gain)

#Save data file
f = open(OFILENAME, 'wb')
for i in range(NUM_SAMPLES):
    f.write(out_data[i])
f.close()

# Summary
print ("    Output filename   : ", OFILENAME)
print ("    Number of samples : ", NUM_SAMPLES)
print ("    Number of bytes   : ", NUM_SAMPLES*4)
