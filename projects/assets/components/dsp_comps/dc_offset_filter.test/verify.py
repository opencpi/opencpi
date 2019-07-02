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
DC Offset Filter: Verify output data

Verify args:
1. Number of complex signed 16-bit samples to validate
2. Output file used for validation
3. Input file used for comparison

To test the DC Offset Filter, a binary data file is generated containing complex
signed 16-bit samples with tones at 5Hz, 13Hz, and 27Hz, and with a DC component.

To validate the test, the output file is is examined using FFT analysis to 
determine if the DC component is removed and the tones are still present and of
sufficient power.
"""
import struct
import numpy as np
import sys
import os.path

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <num-samples> <output-file> <input-file>")
    sys.exit(1)
    
NUM_SAMPLES = int(sys.argv[1])
OFILENAME = sys.argv[2]
IFILENAME = sys.argv[3]
bypass = os.environ.get("OCPI_TEST_bypass")

#Read input and output data files as complex int16
ofile = open(OFILENAME, 'rb')
dout = np.fromfile(ofile, dtype=np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)})), count=-1)
ofile.close()
ifile = open(IFILENAME, 'rb')
din = np.fromfile(ifile, dtype=np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)})), count=-1)
ifile.close()

# Test #1 - Check that output data is not all zeros
if all(dout == 0):
    print ("\tValues are all zero")
    sys.exit(1)

# Test #2 - Check that output data is the expected amount
if len(dout) != NUM_SAMPLES:
    print ("\tOutput file length is unexpected")
    print ("\tLength = ", len(dout), "while expected length is = ", NUM_SAMPLES)
    sys.exit(1)

# Test #3 - Check that output data values
if(bypass=="false"): # => NORMAL MODE
    #share values used during generation of the input file
    #convert to complex data type to perform fft and power measurements
    ToneDC = 0
    Tone05 = 5
    Tone13 = 13
    Tone27 = 27
    Fs = 100
    complex_idata = np.array(np.zeros(NUM_SAMPLES), dtype=np.complex)
    complex_odata = np.array(np.zeros(NUM_SAMPLES), dtype=np.complex)
    for i in range(0,NUM_SAMPLES):
        complex_idata[i] = complex(din['real_idx'][i], din['imag_idx'][i])
        complex_odata[i] = complex(dout['real_idx'][i], dout['imag_idx'][i])
    IFFT = 1.0/NUM_SAMPLES * abs(np.fft.fft(complex_idata,NUM_SAMPLES))
    OFFT = 1.0/NUM_SAMPLES * abs(np.fft.fft(complex_odata,NUM_SAMPLES))
    eps = pow(10, -10) #Error factor to avoid divide by zero in log10
    IPowerDC = 20*np.log10(IFFT[ToneDC]+eps)
    IPowerT1 = 20*np.log10(IFFT[int(round(float(Tone05)/(float(Fs)/2.0)*float(len(IFFT)/2.0)))]+eps)
    IPowerT2 = 20*np.log10(IFFT[int(round(float(Tone13)/(float(Fs)/2.0)*float(len(IFFT)/2.0)))]+eps)
    IPowerT3 = 20*np.log10(IFFT[int(round(float(Tone27)/(float(Fs)/2.0)*float(len(IFFT)/2.0)))]+eps)
    print ("\tInput DC power level     =", IPowerDC, " dBm")
    print ("\tInput Tone 1 power level =", IPowerT1, " dBm")
    print ("\tInput Tone 2 power level =", IPowerT2, " dBm")
    print ("\tInput Tone 3 power level =", IPowerT3, " dBm")
    OPowerDC = 20*np.log10(OFFT[ToneDC]+eps)
    OPowerT1 = 20*np.log10(OFFT[int(round(float(Tone05)/(float(Fs)/2.0)*float(len(OFFT)/2.0)))]+eps)
    OPowerT2 = 20*np.log10(OFFT[int(round(float(Tone13)/(float(Fs)/2.0)*float(len(OFFT)/2.0)))]+eps)
    OPowerT3 = 20*np.log10(OFFT[int(round(float(Tone27)/(float(Fs)/2.0)*float(len(OFFT)/2.0)))]+eps)
    print ("\tOutput DC power level     = ", OPowerDC, " dBm")
    print ("\tOutput Tone 1 power level = ", OPowerT1, " dBm")
    print ("\tOutput Tone 2 power level = ", OPowerT2, " dBm")
    print ("\tOutput Tone 3 power level = ", OPowerT3, " dBm")

    #Perform calculations comparing output power to input power for dc and tones
    if OPowerDC > IPowerDC - 83.9:
        print ("\tOutput DC power level = ", OPowerDC, " dBm")
        sys.exit(1)
    if abs(OPowerT1 - IPowerT1) > 5.3:
        print ("\tOutput Tone 1 level = ", OPowerT1, " dBm")
        sys.exit(1)
    if abs(OPowerT2 - IPowerT2) > 0.4:
        print ("\tOutput Tone 2 level = ", OPowerT2, " dBm")
        sys.exit(1)
    if abs(OPowerT3 - IPowerT3) > 2.3:
        print ("\tOutput Tone 3 level = ", OPowerT3, " dBm")
        sys.exit(1)
    print ("\tResults (Normal Mode): Tones measured at expected power levels")

else: #=> BYPASS MODE
    #Test that odata is the expected amount
    if (din != dout).all():
        print ("\tInput and output file do not match")
        sys.exit(1)
    print ("\tResults (Bypass Mode): Input and output file match")
