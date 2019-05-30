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

import sys
import os.path
import numpy as np

sys.path.append("../../../include")
import assets_ts_utils as ts

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

# from OCS or OWD
R=int(os.environ.get("OCPI_TEST_R"))

# from -test.xml
num_samples = int(os.environ.get("OCPI_TEST_num_samples"))

# from arguments to generate.py (-test.xml)
ofilename = sys.argv[1]
ifilename = sys.argv[2]

#Parse non-samples messages from input file
din_non_sample_msg = ts.parse_non_samples_msgs_from_msgs_in_file(ifilename)

#Parse non-samples messages from output file
dout_non_sample_msg = ts.parse_non_samples_msgs_from_msgs_in_file(ofilename)

#Parse samples data from output file
odata = ts.parse_samples_data_from_msgs_in_file(ofilename)

# Create downsampled ramp to compare to
#ramp = np.int16(np.arange(num_samples)[::R])
ramp = np.array(np.arange(num_samples)[::R],dtype=np.int16)

# Verify non-sample data
print("    Comparing non-sample data")
if len(dout_non_sample_msg) != len(din_non_sample_msg):
    print("    FAIL - Input and output files have different number of non-samples messages")
    print("    Input: ", len(din_non_sample_msg), " Output: ", len(dout_non_sample_msg))
    sys.exit(1)

for i in range(len(dout_non_sample_msg)):
  if(dout_non_sample_msg[i][0]==ts.INTERVAL_OPCODE):
    ts.check_interval_division(din_non_sample_msg[i],dout_non_sample_msg[i],R)
  else:
    ts.compare_msgs(dout_non_sample_msg[i],din_non_sample_msg[i])

#Verify sample data
print("    Comparing Expected I data to Actual I Data")
ts.compare_arrays(ramp, odata['real_idx'])
print("    Comparing Expected Q data to Actual Q Data")
ts.compare_arrays(ramp, odata['imag_idx'])
