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
PR CORDIC: Generate test input data

Generate args:
- input data file

To test the PR CORDIC, a binary data file is generated containing a constant-phase
ramp input in the upper 16 bits, and a constant magnitude value in the lower 16 bits.
The output of the circuit is a complex sinusoid (tone given a constant-phase input)
with maximum amplitude as defined by the magnitude input.

"""
import numpy as np
import sys
import os.path
try:
  import matplotlib.pyplot as plt
  PlotLib = True
except (ImportError, RuntimeError):
  PlotLib = False

print "\n","*"*80
print "*** Python: PR CORDIC ***"

if len(sys.argv) < 2:
    print("Usage expected: plotmode,filename\n")
    sys.exit(1)

# Get input arguments
FCW = int(os.environ.get("OCPI_TEST_FREQUENCY_CONTROL_WORD"))
MAG = int(os.environ.get("OCPI_TEST_MAG"))
DW = int(os.environ.get("OCPI_TEST_DATA_WIDTH"))
num_of_cycles = int(os.environ.get("OCPI_TEST_NUM_OF_CYCLES"))
plotmode = int(sys.argv[1])
filename = sys.argv[2]

# Init internal variables
ramp_max=(2**(DW-1))-1
byte=8
adjust=4
seq_length=np.int(np.ceil((ramp_max/FCW)*(num_of_cycles*adjust)))
data=np.array(np.zeros(seq_length), dtype=np.int16)
ramp_sum=0

# Assign constant magnitude to even indices of data buffer
for i in range(1,seq_length,2):
    data[i]=MAG

# Assign PAC (phase accumulator) to odd indices of data buffer
# Note: The PAC is centered about zero and has a range of
# -(2^15) to (2^15)-1. Starting point is zero.
for i in range(0,seq_length,2):
    if (ramp_sum > ramp_max):
        ramp_sum = ramp_sum - 65536

    data[i] = ramp_sum
    ramp_sum = ramp_sum + FCW

# Write contents to file in 16-bit signed chunks
ifd=open(filename,"wb")
data=np.int16(data)
ifd.write(data)
ifd.close()

# Plots, if desired
if PlotLib and plotmode == 1:
    plt.figure(1)
    plt.plot(data[0::2])
    plt.title("Input Data - Lower 16 bits")
    plt.grid()

    plt.figure(2)
    plt.plot(data[1::2])
    plt.title("Input Data - Upper 16 bits")
    plt.grid()

    plt.show()
