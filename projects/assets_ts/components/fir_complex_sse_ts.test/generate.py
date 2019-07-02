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
FIR Complex: Generate test input data

One test case implemented: 
1. Impulse train - sequence impulse trains of size num_taps

Generate data of sufficient size to produce 2 maximum size output samples
messages

All other operations from ComplexShortWithMetadata were also included in the
input file in a format resembling that of an ADC output

"""
import sys
import os.path
import array
import numpy as np
import opencpi.unit_test_utils as utu
import opencpi.complexshortwithmetadata_utils as iqm

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

# from OCS or OWD
num_taps = int(os.environ.get("OCPI_TEST_NUM_TAPS"))
COEFF_WIDTH = int(os.environ.get("OCPI_TEST_COEFF_WIDTH"))
MAX_BYTES_OUT = int(os.environ.get("OCPI_TEST_ocpi_max_bytes_out"))

#Generate enough samples to generate number_of_samples_messages max_bytes_out sized output messages
number_of_samples_messages = 1
bytes_per_sample = 4
min_samples_to_generate = number_of_samples_messages*MAX_BYTES_OUT/bytes_per_sample

#impulse array is an array of size num_taps and element zero is max coefficient value
if COEFF_WIDTH == 16: 
    impulse_array=np.concatenate(([np.iinfo(np.int16).max],np.zeros(num_taps-1,dtype=np.int16)))
elif COEFF_WIDTH == 8:
    impulse_array=np.concatenate(([np.iinfo(np.int8).max],np.zeros(num_taps-1,dtype=np.int16)))
else:
    print('  FAILED: Unsupported coefficient width: ' + str(COEFF_WIDTH))
    sys.exit(1)        

num_impulse_arrays_required = min_samples_to_generate // num_taps + 1

out_data = np.zeros(num_impulse_arrays_required*num_taps, dtype=utu.dt_iq_pair)
out_data['real_idx'] = np.tile(impulse_array, num_impulse_arrays_required)
out_data['imag_idx'] = out_data['real_idx']

# Write to file
message_size = 2048 #This is the maximum allowed by current buffer negotiation system
samples_per_message = message_size / bytes_per_sample
with open(sys.argv[1], 'wb') as f:
    utu.add_msg(f, iqm.INTERVAL_OPCODE, array.array('I',(int('00000000',16), int('00001FFF',16)))) #8191
    utu.add_msg(f, iqm.TIME_OPCODE, array.array('I',(int('0000AAAA',16), int('0000BBBB',16))))
    iqm.add_samples(f, out_data, 1, samples_per_message)
    utu.add_msg(f, iqm.TIME_OPCODE, array.array('I',(int('0000CCCC',16), int('0000DDDD',16))))
    iqm.add_samples(f, out_data, 1, samples_per_message)
    utu.add_msg(f, iqm.FLUSH_OPCODE, [])
    utu.add_msg(f, iqm.SYNC_OPCODE, [])
