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
Generate golden data file.
"""

import sys
import os.path
import struct
import numpy as np

# Does edge detector logic to generate the expected output data given idata
# gpioInitial is the inital value of the gpioPins before start of message
def gen_golden_data(idata, gpioInitial, numOutputSamples, testCase, caseName):
    filename = caseName + ".golden.dat"
    with open(filename, 'wb') as f:
        previous = gpioInitial
        for x in range(0, numOutputSamples):
            f.write(struct.pack("<I", (idata[x] & np.invert(previous))))
            previous = idata[x]
