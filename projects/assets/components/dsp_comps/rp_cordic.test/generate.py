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

"""Generate input idata for RP CORDIC (binary data file).

Generate args:
- target file

Environment Variable args: 
- amount of input data to generate (number of complex signed 16-bit samples)

To test the RP CORDIC, a binary data file is generated containing complex
signed 16-bit samples at maximum amplitude with a tone at 27Hz sampled at 10kHz.
This input file is run through the UUT to produce an output binary file consist-
ing of real signed 16-bit samples. For verification, the complex input file is
used to produce the expected arrays magnitude_exp and phase_exp. These arrays
are compared sample-by-sample to the UUT output array phase and the single value
magnitude. With an input consisting of a single tone at a fixed magnitude, the
expected output is a fixed maximal magnitude value and a constant phase value.
Error checks are then performed on the results, where the measured values can be
no more than +/- 1.0 from the expected values.

"""
import struct
import numpy as np
import math
import sys
import os.path

filename = sys.argv[1]
num_samples = (int(os.environ.get("OCPI_TEST_NUM_SAMPLES")))

# Create an input file with one complex tone
# Tone is at 27 Hz; Fs=10000 Hz
Tone27 = 27
Fs = 10000
Ts = 1.0/float(Fs)
t = np.arange(0,num_samples*Ts,Ts,dtype=np.float)
real = np.cos(Tone27*2*np.pi*t)
imag = np.sin(Tone27*2*np.pi*t)
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))
out_data = np.array(np.zeros(num_samples), dtype=dt_iq_pair)
j = 0
# pick a gain at 32767 (full scale) - this verifies full scale does not cause overflows
# using a different gain for each rail causes an I/Q spectral image (don't do this!)
gain = 32767 / max(abs(real))
out_data['real_idx'] = np.int16(real * gain)
out_data['imag_idx'] = np.int16(imag * gain)

# convert to complex data type to perform fft and power measurements
complex_data = np.array(np.zeros(num_samples), dtype=np.complex)
for i in range(0,len(out_data)):
    complex_data[i] = complex(out_data['real_idx'][i], out_data['imag_idx'][i])
FFT = 1.0/num_samples * abs(np.fft.fft(complex_data))
eps = pow(10, -10) #Error factor to avoid divide by zero in log10
# one tone in range DC to +Fs/2
PowerT1  = 20*np.log10(FFT[int(round(float(Tone27)/(float(Fs)/2.0)*float(len(FFT)/2.0)))]+eps)
print ('Input Tone 1 power level = ', PowerT1, ' dBm')

# Save data file
f = open(filename, 'wb')
for i in range(num_samples):
    f.write(out_data[i])
f.close()
print ('    Number of samples: ', num_samples)
