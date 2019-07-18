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
Generate input data and call gen_golden_data to generate golden data file.

Generate args:
1. Output file

"""

import sys
import os.path
import struct
import numpy as np
import generate_golden



if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

filename = sys.argv[1]

f = open(filename, 'wb')

numOutputSamples = int(os.environ.get("OCPI_TEST_numOutputSamples"))
testCase = int(os.environ.get("OCPI_TEST_testCase"))
caseName =  os.path.splitext(sys.argv[1])[0].split('/')[-1]
# Initial value of GPIO pins before start of message
gpioInitial = 0

# To generate the golden data, the result of AND'ing the data and mask is passed to the gen_golden_data function
# Property and data port test cases have the same golden data but only data port test case needs the input data.
if (testCase == 1 or testCase == 2):
    # Input data
    mask_data = np.array([], dtype=np.uint32)
    # The result of AND'ing data and mask. This is passed to to the gen_golden_data function
    # so that the expected result can be calculated
    gpioData = np.array([], dtype=np.uint32)

    # Set data but don't set mask
    # data = 1:7 and mask = 0
    for x in range(1, 8):
        mask_data = np.append(mask_data, x)
        gpioData = np.append(gpioData, (x & 0))

    # Set data and set appropriate mask
    # data = 1:7 and mask = 1:7
    for x in range(1, 8):
        mask_data = np.append(mask_data, ((x<<16) + x))
        gpioData = np.append(gpioData, (x & x))

    # Clear data by setting data to 0x0000 and mask to 0x0007
    mask_data = np.append(mask_data, 0x00070000)
    gpioData = np.append(gpioData, 0)

    # Set data but only set some of the appropriate masks
    # data = 7 and mask = 1:6
    for x in range(1, 7):
        mask_data = np.append(mask_data, ((x<<16) + 0x00000007))
        gpioData = np.append(gpioData, (0x00000007 & x))

    generate_golden.gen_golden_data(gpioData, gpioInitial, numOutputSamples, testCase, caseName)

    if (testCase == 2):
        for x in range(0, numOutputSamples):
            f.write(struct.pack("<I", mask_data[x]))

# Devsignal test case
elif (testCase == 3):
    mask_data = np.array([], dtype=np.uint32)
    gpioData = np.array([], dtype=np.uint32)
    # For the devsignal test case, the devsignal data is toggled on and off
    for x in range(0, numOutputSamples):
        mask_data = np.append(mask_data, x)
        gpioData = np.append(gpioData, x)

    generate_golden.gen_golden_data(gpioData, gpioInitial, numOutputSamples, testCase, caseName)

f.close()
