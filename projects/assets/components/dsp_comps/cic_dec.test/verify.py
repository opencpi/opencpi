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
CIC Decimator: Verify output data

Samples data is verified by modeling the CIC in python and processing the input 
data and comparing to actual output data. 
"""
import sys
import os.path
import numpy as np
import math
import opencpi.unit_test_utils as utu
import opencpi.dsp_utils as dsp

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))
DATA_WIDTH=int(os.environ.get("OCPI_TEST_DIN_WIDTH"))

# Read output and input data as complex int16 samples
odata = np.fromfile(sys.argv[1], dtype=utu.dt_iq_pair, count=-1)
idata = np.fromfile(sys.argv[2], dtype=utu.dt_iq_pair, count=-1)

# Deinterleave dt_iq_pair array into I & Q arrays
in_real = idata['real_idx']
out_real = odata['real_idx']
out_imag = odata['imag_idx']
#print(out_real)

# I and Q should match
print("    Comparing I and Q data to ensure they match")
utu.compare_arrays(out_real,out_imag)

# I and Q have been verified to be equal, so verification will only be performed on I

# Values should not all be zeros
print("    Checking output values are not all zeros")
if np.count_nonzero(out_real) == 0:
    print("    FAILED: output is all zeros")
    sys.exit(1)    

# Verify Sample Data
print("    Comparing expected output and actual output")
# Compute expected output
cic_out_expected = dsp.cic_dec_model(in_real, N, M, R, DATA_WIDTH)
#print(cic_out)
utu.compare_arrays(cic_out_expected, out_real)
