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
Downsample: Verify output data

Output is verified by downsampling input data in python and comparing to actual 
output data
"""
import sys
import os.path
import numpy as np
import opencpi.unit_test_utils as utu

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

# from OCS or OWD
R=int(os.environ.get("OCPI_TEST_R"))

# parse output file as complex samples
odata = np.fromfile(sys.argv[1], dtype=utu.dt_iq_pair, count=-1)

# parse input file as complex samples
idata = np.fromfile(sys.argv[2], dtype=utu.dt_iq_pair, count=-1)

#Compare expected output to actual output
print("    Comparing Expected I data to Actual I Data")
utu.compare_arrays(idata['real_idx'][::R], odata['real_idx'])
print("    Comparing Expected Q data to Actual Q Data")
utu.compare_arrays(idata['imag_idx'][::R], odata['imag_idx'])
