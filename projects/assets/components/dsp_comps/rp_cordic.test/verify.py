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

"""Validate odata for RP CORDIC (binary data file).

Validate args:
- target output file
- input file generated used to create output file

Environment Variable args:
- amount provided as input (number of complex signed 16-bit samples)

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

def dispGetPeakErr(val,n):
    maximum = 0
    for k in range(0,n):
        if abs(val[k]) > maximum:
            maximum = abs(val[k])
    return(maximum)

def AvgPeakError(x,y,n):
    sumDiffsX = 0
    sumDiffsY = 0
    for k in range(0,n):
        sumDiffsX += abs(x[k])
        sumDiffsY += abs(y[k])
    x_err = float(sumDiffsX) / n
    y_err = float(sumDiffsY) / n
    #print (sumDiffsX, sumDiffsY, x_err, y_err, n)

    if (x_err and y_err) <= 1:
        return 1
    else:
        return -1

NUM_WORKER_DELAYS = 8 + int(os.environ.get("OCPI_TEST_STAGES"))
num_samples = (int(os.environ.get("OCPI_TEST_NUM_SAMPLES"))) - NUM_WORKER_DELAYS
# The measured peak magnitude from the UUT
magnitude   = int(os.environ.get("OCPI_TEST_magnitude"))

# Read all of input data file as complex int16
ifx = open(sys.argv[2], 'rb')
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))
din = np.fromfile(ifx, dtype=dt_iq_pair, count=-1)
ifx.close()
# Read all of output data file as real int16
ofx = open(sys.argv[1], 'rb')
dout = np.fromfile(ofx, dtype=np.int16, count=-1)
ofx.close()

# Test #1 - Check that output data is not all zeros
if all(dout == 0):
    print ('Values are all zero')
    sys.exit(1)

# Test #2 - Check that output data is the expected amount
if len(dout) != num_samples:
    print ('Output file length is unexpected')
    print ('Length dout = ', len(dout), 'while expected length is = ', num_samples)
    sys.exit(1)

# Test #3 - Check that output data values
# Construct complex sample arrays from the input file
real      = din['real_idx']
imag      = din['imag_idx']
# The real output samples from the UUT are the phase
phase     = dout
# Construct empty arrays to hold the expected results for comparison to measured output
magnitude_exp = np.array(np.zeros(num_samples), dtype=np.int16)
phase_exp     = np.array(np.zeros(num_samples), dtype=np.float)

# Calculate the expected results from the input file
scaling_factor = pow(2,int(os.environ.get("OCPI_TEST_DATA_WIDTH"))-1) / np.pi
for i in range(0,num_samples):
    magnitude_exp[i] = math.sqrt( real[i]**2 + imag[i]**2 )
    phase_exp[i]     = scaling_factor * math.atan2( imag[i], real[i] )

# Calculate the expected difference in phase. Since we will have n-1 results
# for n samples simply copy the n-1 result into the nth result
for i in range(0,num_samples-1):
    phase_exp[i] = phase_exp[i+1] - phase_exp[i]
    #if np.int16(abs(phase_exp[i])) > 177:
    #    print (i, np.int16(phase_exp[i]))
phase_exp[num_samples-1] = phase_exp[num_samples-2]

# Buffers for holding the difference
diffsMag = np.array(np.zeros(num_samples), dtype=np.int16)
diffsPha = np.array(np.zeros(num_samples), dtype=np.int16)

# Calculate the difference between UUT output and expected values
for i in range(0,num_samples):
    diffsMag[i] = magnitude - magnitude_exp[i]
    diffsPha[i] = phase[i]  - phase_exp[i]
    #print (diffsMag[i], diffsPha[i])
    #if abs(diffsPha[i]) >= 2:
    #    print (i, diffsMag[i], diffsPha[i], phase_exp[i])

# Calculate the measured errors
check_AvgPeakError = 0
check_AvgPeakError = AvgPeakError(diffsMag,diffsPha,num_samples)
mag_peak_err = dispGetPeakErr(diffsMag,num_samples)
pha_peak_err = dispGetPeakErr(diffsPha,num_samples)

# Verify - Output is within the acceptable tolerances.
# Note that "mag_peak_err > 2" (not > 1) to compensate for the larger delta between
# Python (Non-CORDIC algorithm, but rather equivalent math functions) vs HDL CORDIC algorithm
if (check_AvgPeakError > 1) or (mag_peak_err > 2) or (pha_peak_err > 1):
    print ('\tAvgPeakError = ', check_AvgPeakError, 'mag_peak_err = ', mag_peak_err, 'pha_peak_err = ', pha_peak_err, '')
    sys.exit(1)
print ("\tResults (Normal Mode): Output within acceptable tolerances")
