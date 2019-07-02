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
CIC Decimator: Generate test input data

Two Test Cases implemented: 
1. Impulse - Max value followed by all zeros
2. Step - All max values

Generate data of sufficient size to produce 2 maximum size output samples
messages
"""
import sys
import os.path
import numpy as np
import array

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

#from -test.xml
TEST_CASE = int(os.environ.get("OCPI_TEST_TEST_CASE"))

# from OCS or OWD
R = int(os.environ.get("OCPI_TEST_R"))
MAX_BYTES_OUT = int(os.environ.get("OCPI_TEST_ocpi_max_bytes_out"))

#Generate enough samples to generate number_of_samples_messages max_bytes_out sized output messages
number_of_samples_messages = 2
bytes_per_sample = 4
samples_to_generate = number_of_samples_messages*MAX_BYTES_OUT/bytes_per_sample*R
# Select test data to generate: Impulse or Step 
if TEST_CASE == 0: # Generate Impulse
    out_data = np.zeros(samples_to_generate, dtype=np.int32)
    out_data[0] = 0x7FFF7FFF
elif TEST_CASE == 1: #Generate Step
    out_data = 0x7FFF7FFF*np.ones(samples_to_generate, dtype=np.int32)

# Write to file
message_size = 2048 #This is the maximum allowed by current buffer negotiation system
samples_per_message = message_size / bytes_per_sample
f = open(sys.argv[1],'wb')
f.write(out_data)
f.close()
