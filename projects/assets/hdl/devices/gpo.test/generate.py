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
"""

import sys
import os.path
import struct
import numpy as np



if len(sys.argv) != 2:
    print ("Don't run this script manually, it is called by 'ocpidev test' or 'make test'")
    sys.exit(1)

out_file = open(sys.argv[1], 'wb')

numOutputSamples = int(os.environ.get("OCPI_TEST_numOutputSamples"))
testCase = int(os.environ.get("OCPI_TEST_testCase"))

# To generate the golden data, the result of AND'ing the data and mask is passed to the gen_golden_data function
# Property and data port test cases have the same golden data but only data port test case needs the input data.
if (testCase == 1 or testCase == 2):
    # Input data
    mask_data = np.array([], dtype=np.uint32)
    # The current state of the GPIO pins
    gpoData = np.array([], dtype=np.uint32)

    # Set all data bits but don't set mask
    mask_data = np.append(mask_data, 0x0000FFFF)

    # Setting data bits high
    mask_data = np.append(mask_data, 0x00FF00FF)
    gpoData = np.append(gpoData, 0x000000FF)

    mask_data = np.append(mask_data, 0xFF00FF00)
    gpoData = np.append(gpoData, 0x0000FFFF)

    # Clear data bits
    mask_data = np.append(mask_data, 0x00FF0000)
    gpoData = np.append(gpoData, 0x0000FF00)

    mask_data = np.append(mask_data, 0xFF000000)
    gpoData = np.append(gpoData, 0x00000000)


    with open("golden.dat", 'wb') as f:
        for x in range(0, numOutputSamples):
            f.write(struct.pack("<I", gpoData[x]))

    if (testCase == 2):
        for x in range(0, int(os.environ.get("OCPI_TEST_numInputSamples"))):
            out_file.write(struct.pack("<I", mask_data[x]))

out_file.close()
