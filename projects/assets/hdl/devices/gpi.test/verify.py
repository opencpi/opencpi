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

import struct
import numpy as np
import sys
import os.path


dt = np.dtype('<u4')

# Open input file and grab samples as uint32
with open(sys.argv[1], 'rb') as f:
    outData = np.fromfile(f, dtype=dt)

testCase = int(os.environ.get("OCPI_TEST_testCase"))
numClockCycles = int(os.environ.get("OCPI_TEST_numClockCycles"))
# How long the input data to gpi_em is held at the same value before changing values
# The code below should be uncommented and be used instead when AV-5635 is resolved
counterClockCycles = pow(2, int(os.environ.get("OCPI_TEST_COUNTER_WIDTH")))+2
# counterWidth = int(np.ceil(np.log2(float(os.environ.get("OCPI_TEST_CLK_RATE_HZ"))*float(os.environ.get("OCPI_TEST_DEBOUNCE_TIME_PSEC"))/1000000000000.0)))
# # How long the input data to gpi_em is held at the same value before changing values
# counterClockCycles = pow(2, counterWidth)+2
expectedData = np.array([], dtype=np.uint32)
extractedData = outData[np.where(outData!=0)]

def genExpectedData(expectedData):
    if testCase == 1:
        expectedData = np.array([0x00010001, 0x00010000], dtype=np.uint32)
    elif testCase == 2:
        expectedData = np.append(expectedData, 0x00010001)
        # There are counterClockCycles-1 number of the value 0x00000001 that is sent to the output port
        for x in range(0, counterClockCycles-1):
            expectedData = np.append(expectedData, 0x00000001)
        expectedData = np.append(expectedData, 0x00010000)
    elif testCase == 3:
        expectedData = np.array([0x00010001, 0x00010000], dtype=np.uint32)
    elif testCase == 4:
        expectedData = np.array([0x00010001, 0x00000001, 0x00010000], dtype=np.uint32)
    elif testCase == 5:
        expectedData = np.append(expectedData, 0xFFFFFFFF)
        # There are 2*counterClockCycles-1 number of the value 0x0000FFFF that is sent to the output port
        for x in range(0, 2*counterClockCycles-1):
            expectedData = np.append(expectedData, 0x0000FFFF)
        expectedData = np.append(expectedData, 0xFFFF0000)

    return expectedData

expectedData = genExpectedData(expectedData)
if len(extractedData) != len(expectedData):
    print ("    Length of extracted output data and expected data do not match")
    print ("    Length = ", len(extractedData), "while expected length is = ", len(expectedData))
    sys.exit(1)
else:
    print ("    Length of extracted output data and expected data match")

if np.array_equal(extractedData, expectedData):
    print ("    Extracted output data and expected data match")
else:
    print ("    Extracted output data and expected data do not match")
    sys.exit(1)
