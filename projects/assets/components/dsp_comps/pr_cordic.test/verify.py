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
PR CORDIC: Validate output data for Polar-to-Rectangular CORDIC

Validation args:
- output data filename
- input data file

To test the PR CORDIC, a binary data file is generated containing a constant-phase
ramp input in the lower 16 bits, and a constant magnitude value in the upper 16 bits.
The output of the circuit is a complex sinusoid (tone given a constant-phase input)
with maximum amplitude as defined by the magnitude input.

"""
import os
import sys
import numpy as np

def dispGetPeakErr(val,n):

    maximum=0

    for k in range(0,n):
        if abs(val[k])>maximum:
            maximum=abs(val[k])

    return(maximum)

# Get difference between actual and expected
def getDiff(diff,a_val,e_val,n):

    for k in range(0,n):
         diff[k]=a_val[k]-e_val[k]

    return(diff)

def AvgPeakError(x,y,n):

    sumDiffsX=0
    sumDiffsY=0
    for k in range(0,n):
        sumDiffsX+=abs(x[k])
        sumDiffsY+=abs(y[k])
    x_err=sumDiffsX/n
    y_err=sumDiffsY/n

    if (x_err and y_err) <= 1:
        return 1
    else:
        return -1

def getDATA(FILE):

    ifd=open(FILE,"r")
    idata=np.fromfile(ifd,dtype=np.int16)
    ifd.close()

    upper16=idata[1::2]
    lower16=idata[0::2]

    return(upper16,lower16)


if len(sys.argv) != 3:
    print("Invalid arguments: usage is verify.py <output_file> <input_file>")
    sys.exit(1)

OFILE = sys.argv[1]
IFILE = sys.argv[2]
DATA_WIDTH = int(os.environ.get("OCPI_TEST_DATA_WIDTH"))

NUM_PRIM_DELAYS = int(os.environ.get("OCPI_TEST_STAGES")) + 2
NUM_WORKER_DELAYS = NUM_PRIM_DELAYS
NUM_OUTPUT_SAMPLES = int((os.stat(IFILE).st_size)/4) - NUM_WORKER_DELAYS

iUpper16, iLower16 = getDATA(IFILE)
oUpper16, oLower16 = getDATA(OFILE)

x_expected = [None]*NUM_OUTPUT_SAMPLES
y_expected = [None]*NUM_OUTPUT_SAMPLES

for k in range(0,NUM_OUTPUT_SAMPLES):
    x_expected[k] = round(iUpper16[k] * np.cos ((np.pi*iLower16[k])/(2**(DATA_WIDTH-1))))
    y_expected[k] = round(iUpper16[k] * np.sin ((np.pi*iLower16[k])/(2**(DATA_WIDTH-1))))

# Buffers for holding the difference
diffsY=[None]*NUM_OUTPUT_SAMPLES
diffsX=[None]*NUM_OUTPUT_SAMPLES

diffsX= getDiff(diffsX,oLower16,x_expected,NUM_OUTPUT_SAMPLES)
diffsY= getDiff(diffsY,oUpper16,y_expected,NUM_OUTPUT_SAMPLES)

check_AvgPeakError=0
check_AvgPeakError=AvgPeakError(diffsX,diffsY,NUM_OUTPUT_SAMPLES)

x_peak_err=dispGetPeakErr(diffsX,NUM_OUTPUT_SAMPLES)
y_peak_err=dispGetPeakErr(diffsY,NUM_OUTPUT_SAMPLES)

# Verify - Output is within the acceptable tolerances.
if ((check_AvgPeakError != 1) or (check_AvgPeakError >= 1)) and (x_peak_err > 1) and (y_peak_err > 1):
    print ('\tAvgPeakError = ', check_AvgPeakError, 'x_peak_err = ', x_peak_err, 'y_peak_err = ', y_peak_err, '')
    sys.exit(1)
