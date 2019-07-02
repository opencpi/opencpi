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
Downsample : Generate test input data

Generate ramp of sufficient size to produce 2 maximum size output messages

All other operations from ComplexShortWithMetadata were also included in the
input file in a format resembling that of an ADC output
"""
import sys
import os.path
import numpy as np
import array
import struct
import opencpi.unit_test_utils as utu
import opencpi.complexshortwithmetadata_utils as iqm

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

# from arguments to generate.py (-test.xml)
R = int(os.environ.get("OCPI_TEST_R"))
max_bytes_out = int(os.environ.get("OCPI_TEST_ocpi_max_bytes_out"))

# Generate enough samples to generate number_of_samples_messages max_bytes_out sized output messages
number_of_samples_messages = 1
bytes_per_sample = 4
num_samples_to_generate = number_of_samples_messages*max_bytes_out/bytes_per_sample*R

# Create ramp from 0 to num-samples-1
ramp = np.arange(num_samples_to_generate)

# Initialize empty array, sized to store interleaved I/Q 16bit samples
out_data = np.array(np.zeros(len(ramp)), dtype=utu.dt_iq_pair)

# Put ramp in generated output
out_data['real_idx'] = np.int16(ramp)
out_data['imag_idx'] = np.int16(ramp)

# Write to file
# Save data to file
message_size = 2048 #This is the maximum allowed by current buffer negotiation system
samples_per_message = message_size/bytes_per_sample
with open(sys.argv[1], 'wb') as f:
    utu.add_msg(f, iqm.INTERVAL_OPCODE, array.array('I',(int('00000000',16), int('00002000',16)))) #8192
    utu.add_msg(f, iqm.TIME_OPCODE, array.array('I',(int('0000AAAA',16), int('0000BBBB',16))))
    iqm.add_samples(f, out_data, 1, samples_per_message)
    utu.add_msg(f, iqm.TIME_OPCODE, array.array('I',(int('0000CCCC',16), int('0000DDDD',16))))
    iqm.add_samples(f, out_data, 1, samples_per_message)
    #Small message used to test that worker will not output samples ZLMs
    iqm.add_samples(f, np.array([3,4], dtype=utu.dt_iq_pair), 1, samples_per_message)
    utu.add_msg(f, iqm.FLUSH_OPCODE, [])
    utu.add_msg(f, iqm.SYNC_OPCODE, [])
