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
Generate Scrpit
"""

import sys
import os.path
import struct
import numpy as np



with open(sys.argv[1], 'wb') as f:
    testCase = int(os.environ.get("OCPI_TEST_testCase"))
    counterClockCycles = pow(2, int(os.environ.get("OCPI_TEST_COUNTER_WIDTH")))+2
    # The code below should be uncommented and be used instead when AV-5635 is resolved
    # counterWidth = int(np.ceil(np.log2(float(os.environ.get("OCPI_TEST_CLK_RATE_HZ"))*float(os.environ.get("OCPI_TEST_DEBOUNCE_TIME_PSEC"))/1000000000000.0)))
    # counterClockCycles = pow(2, counterWidth)+2
    if (testCase == 1 or testCase == 3):
        # Toggle pins on and off.
        # Case 1 does not use the debounce circuit, edge detector circuit, or toggle circuit.
        # Case 3 tests the edge detector circuit
        f.write(struct.pack("<I", 1))
        f.write(struct.pack("<I", 0))
    elif testCase == 2:
        # Toggle pins on and off. This case tests only the debounce circuit.
        # Need to keep the input the same for counterClockCycles number of clock cycles
        for x in range(0, counterClockCycles):
            f.write(struct.pack("<I", 1))

        for x in range(0, counterClockCycles):
            f.write(struct.pack("<I", 0))
    elif testCase == 4:
        # Toggle pins on, off, on, and then off. This case tests only the toggle circuit.
        f.write(struct.pack("<I", 1))
        f.write(struct.pack("<I", 0))
        f.write(struct.pack("<I", 1))
        f.write(struct.pack("<I", 0))
    elif testCase == 5:
        # Toggle pins on, off, and then on. Tests the debounce circuit, edge detector circuit, and toggle circuit.
        # Need to keep the input the same for counterClockCycles number of clock cycles
        for x in range(0, counterClockCycles):
            f.write(struct.pack("<I", 0x0000FFFF))

        for x in range(0, counterClockCycles):
            f.write(struct.pack("<I", 0))

        for x in range(0, counterClockCycles):
            f.write(struct.pack("<I", 0x0000FFFF))
    else:
        print("Invalid testCase: Valid testCase are 1, 2, 3, 4 and 5")
        sys.exit(1)
