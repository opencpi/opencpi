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
import array
import struct

sys.path.append("../include")
import assets_ts_utils as ts

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

# from arguments to generate.py (-test.xml)
num_samples = int(os.environ.get("OCPI_TEST_num_samples"))
ofilename   = sys.argv[1]

# Create ramp from 0 to num-samples-1
ramp=np.arange(num_samples)

# Initialize empty array, sized to store interleaved I/Q 16bit samples
out_data = np.array(np.zeros(len(ramp)), dtype=ts.dt_iq_pair)

# Put ramp in generated output
out_data['real_idx'] = np.int16(ramp)
out_data['imag_idx'] = np.int16(ramp)

# Write to file
# Save data to file
message_size=2048
samples_per_message=message_size/4
with open(ofilename, 'wb') as f:
    ts.addmsg(f, ts.INTERVAL_OPCODE, array.array('I',(int('00000000',16), int('00001FFF',16)))) #8191
    ts.addmsg(f, ts.TIME_OPCODE, array.array('I',(int('00000000',16), int('00000000',16))))
    ts.addsamples(f, out_data, 1, samples_per_message)
    ts.addmsg(f, ts.FLUSH_OPCODE, [])
    ts.addmsg(f, ts.SYNC_OPCODE, [])
