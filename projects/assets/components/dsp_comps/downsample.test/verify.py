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

def compare_arrays(a1,a2):
  if not np.array_equal(a1,a2):
    bad_idxs = np.where(a1 != a2)
    bad_idx = bad_idxs[0][0]
    print("    FAILED: output file did not match expected values for",
        "sample index =", bad_idx,
        ", expected value =", a1[bad_idx],
        ", actual value =", a2[bad_idx])
    sys.exit(1)

if len(sys.argv) != 3:
    print("Invalid arguments:  usage is: verify.py <output-file> <input-file>")
    sys.exit(1)

# from OCS or OWD
R=int(os.environ.get("OCPI_TEST_R"))

# from -test.xml
num_samples = int(os.environ.get("OCPI_TEST_num_samples"))

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

ofilename = open(sys.argv[1], 'rb')
odata = np.fromfile(ofilename, dtype=dt_iq_pair, count=-1)
ofilename.close()

# Create downsampled ramp to compare to
ramp = np.int16(np.arange(num_samples)[::R])

#Verify output data
print("    Comparing I data")
compare_arrays(ramp, odata['real_idx'])
print("    Comparing Q data")
compare_arrays(ramp, odata['imag_idx'])
