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
FIR Complex: Verify output data

Samples data is verified by comparing the output data to the tap values, which
is the expected output in response to an impulse.

All other operations are verified by comparing input and output to ensure they
match
"""
import numpy as np
import sys
import os.path
import opencpi.unit_test_utils as utu
import opencpi.complexshortwithmetadata_utils as iqm

if len(sys.argv) < 2:
    print("Usage expected: output filename, input filename\n")
    sys.exit(1)

#Generate text file of symmetric tap values for comparison
num_taps=int(os.environ.get("OCPI_TEST_NUM_TAPS"))
taps=list(map(int, os.environ.get("OCPI_TEST_taps").split(","))) #tap values are comma-separated

ofilename= sys.argv[1]
ifilename= sys.argv[2]

#Check to confirm number of taps is as expected
if num_taps != 2*len(taps):
    print('  FAILED: Actual number of taps does not match specified NUM_TAPS')
    sys.exit(1)

#Check to make sure not all taps are zero
if( sum(map(abs,taps)) == 0):
    print('  FAILED: taps are all zero')
    sys.exit(1)

# Parse non-samples messages from input file
din_non_sample_msg = iqm.parse_non_samples_msgs_from_msgs_in_file(ifilename)

# Parse non-samples messages from output file
dout_non_sample_msg = iqm.parse_non_samples_msgs_from_msgs_in_file(ofilename)

# Parse samples data from input file. Returns dt_iq_pair array
idata = iqm.parse_samples_data_from_msgs_in_file(ifilename)

# Parse samples data from output file. Returns dt_iq_pair array
odata = iqm.parse_samples_data_from_msgs_in_file(ofilename)

# Deinterleave dt_iq_pair array into I & Q arrays
in_real  = idata['real_idx']
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
    utu.compare_msgs(dout_non_sample_msg[i],din_non_sample_msg[i])

# Verify Sample Data
print("    Comparing length of output data")
if len(in_real) != len(out_real):
    print("    FAIL - Unexpected Output Length")
    print("    Expected: ", len(in_real), " Actual: ", len(out_real))
    sys.exit(1)

print("    Comparing expected output and actual output")
# Compare repeated symmetric taps array to output
taps.extend(reversed(taps)); #extend the taps to be symmetric
expected_output = np.tile(taps,len(out_real)/len(taps))
tolerance=1
if not (np.isclose(out_real,expected_output,atol=tolerance).all()):
    print(out_real)
    print(expected_output)
    print ('  FAILED: Sample data does not match expected result')
    sys.exit(1)
