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
"""
import sys
import os.path
import numpy as np
import math

import assets_ts_utils as ts
import cic_utils

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

# from arguments to verify.py (-test.xml)
ofilename= sys.argv[1]
ifilename= sys.argv[2]
TEST_CASE=int(os.environ.get("OCPI_TEST_TEST_CASE"))

#Parse non-samples messages from input file
din_non_sample_msg = ts.parse_non_samples_msgs_from_msgs_in_file(ifilename)

#Parse non-samples messages from output file
dout_non_sample_msg = ts.parse_non_samples_msgs_from_msgs_in_file(ofilename)

#Parse samples data from output file. Returns dt_iq_pair array
odata = ts.parse_samples_data_from_msgs_in_file(ofilename)

#Parse samples data from output file. Returns dt_iq_pair array
idata = ts.parse_samples_data_from_msgs_in_file(ifilename)

# Deinterleave dt_iq_pair array into I & Q arrays
# Append zeros corresponding to flush operation for input data
in_real = np.append(idata['real_idx'],np.zeros(cic_utils.N*cic_utils.M*cic_utils.R, dtype=np.int16))
in_imag = np.append(idata['imag_idx'],np.zeros(cic_utils.N*cic_utils.M*cic_utils.R, dtype=np.int16))
out_real = odata['real_idx']
out_imag = odata['imag_idx']
#print(out_real)

# I and Q should match
print("    Comparing I and Q data to ensure they match")
ts.compare_arrays(out_real,out_imag)

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
  if dout_non_sample_msg[i][0]==ts.INTERVAL_OPCODE:
    ts.check_interval_division(din_non_sample_msg[i],dout_non_sample_msg[i],cic_utils.R)
  else:
    ts.compare_msgs(dout_non_sample_msg[i],din_non_sample_msg[i])

# Verify Sample Data
print("    Comparing expected output and actual output")
# Implement CIC
# Variable names track cic_dec_gen.vhd
integ     = np.zeros(cic_utils.N+1,dtype=np.int64)
out_integ = np.zeros(len(in_real),dtype=np.int64) #Store sequence of integrator output

# Integrator
for a in range(len(in_real)):
    integ[0] = in_real[a]
    for b in range(1,cic_utils.N+1):
        integ[b] = integ[b] + integ[b-1]
        if integ[b] > pow(2,cic_utils.ACC_WIDTH): #Overflow
            integ[b] = integ[b]-pow(2,cic_utils.ACC_WIDTH)
        #Underflow??
    out_integ[a] = integ[cic_utils.N]
#print(out_integ)

# Decimator
# Starting point of decimation is rate minus prop delay of integrator stage
out_dec = out_integ[cic_utils.R-cic_utils.N::cic_utils.R]
#print(out_dec)

# Comb
comb       = np.zeros(cic_utils.N+1,dtype=np.int64)
comb_dly   = np.zeros((cic_utils.N+1,cic_utils.M+1),dtype=np.int64)
comb_r     = np.zeros(cic_utils.N+1,dtype=np.int64)                #copy of comb
comb_dly_r = np.zeros((cic_utils.N+1,cic_utils.M+1),dtype=np.int64)#copy of comb_r
out_comb   = np.zeros(len(out_dec),dtype=np.int64)

for a in range(len(out_dec)):
    comb_r      = comb.copy()
    comb_dly_r  = comb_dly.copy()
    comb[0] = out_dec[a]
    for b in range(1,cic_utils.N+1):
        for c in range(1,cic_utils.M+1):
            comb_dly[b][c] = comb_dly_r[b][c-1]
        comb_dly[b][0] = comb_r[b-1]
        comb[b] = comb_r[b-1] - comb_dly[b][cic_utils.M]
    out_comb[a] = comb[cic_utils.N]
    #print(comb)
#print(out_comb)

# Compare HDL implementation to Python Implementation
cic_out = np.int16(out_comb >> (cic_utils.ACC_WIDTH - cic_utils.DATA_WIDTH))
#print(cic_out)
# Remove startup samples from output
cic_out = cic_out[cic_utils.N*cic_utils.M::]
#print(cic_out)
ts.compare_arrays(cic_out, out_real)
