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

Interval data is verified by dividing the input data by R and comparing to 
actual output data

All other operations are verified by comparing input and output to ensure they
match
"""
import sys
import os.path
import numpy as np
import math
import opencpi.unit_test_utils as utu
import opencpi.complexshortwithmetadata_utils as iqm
import opencpi.dsp_utils as dsp

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

# from OCS or OWD
N=int(os.environ.get("OCPI_TEST_N"))
M=int(os.environ.get("OCPI_TEST_M"))
R=int(os.environ.get("OCPI_TEST_R"))
DATA_WIDTH=int(os.environ.get("OCPI_TEST_DATA_WIDTH"))

ofilename= sys.argv[1]
ifilename= sys.argv[2]

# Parse non-samples messages from input file
din_non_sample_msg = iqm.parse_non_samples_msgs_from_msgs_in_file(ifilename)

# Parse non-samples messages from output file
dout_non_sample_msg = iqm.parse_non_samples_msgs_from_msgs_in_file(ofilename)

# Parse samples data from output file. Returns dt_iq_pair array
odata = iqm.parse_samples_data_from_msgs_in_file(ofilename)

# Parse samples data from output file. Returns dt_iq_pair array
idata = iqm.parse_samples_data_from_msgs_in_file(ifilename)

# Deinterleave dt_iq_pair array into I & Q arrays
# Append zeros corresponding to flush operation for input data
in_real = np.append(idata['real_idx'],np.zeros(N*M*R, dtype=np.int16))
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

# Verify non-sample data
print("    Comparing length of non-sample data in input and output")
if len(dout_non_sample_msg) != len(din_non_sample_msg):
    print("    FAIL - Input and output files have different number of non-samples messages")
    print("    Input: ", len(din_non_sample_msg), " Output: ", len(dout_non_sample_msg))
    print(din_non_sample_msg)
    print(dout_non_sample_msg)
    sys.exit(1)

for i in range(len(dout_non_sample_msg)):
    if dout_non_sample_msg[i][utu.MESSAGE_OPCODE]==iqm.INTERVAL_OPCODE:
        if not iqm.check_interval_division(din_non_sample_msg[i],dout_non_sample_msg[i],R):
            sys.exit(1)
    else:
        utu.compare_msgs(dout_non_sample_msg[i],din_non_sample_msg[i])

# Verify Sample Data
print("    Comparing expected output and actual output")
# Implement CIC
cic_out = dsp.cic_dec_model(in_real, N, M, R, DATA_WIDTH)
#print(cic_out)
# Remove startup samples from output
cic_out = cic_out[N*M::]
#print(cic_out)
utu.compare_arrays(cic_out, out_real)
