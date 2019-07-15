#!/usr/bin/env python2
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

#!/usr/bin/env python
import sys
import os.path
import numpy as np

# '../../verify.py', '1', 'case00.00.unit_tester.rcc.out1.out', '../../gen/inputs/case00.00.in1', '../../gen/inputs/case00.00.in2', '../../gen/inputs/case00.00.in3'
# if len(sys.argv) != 7:
#     print("Invalid arguments:  usage is: verify.py <port number> <output-file> <input-file-1> <input-file-2> <input-file-3> <input-file-4>")
#     sys.exit(1)

# Read all output data as complex int16 samples
ofilename = open(sys.argv[2], 'rb')
odata = np.fromfile(ofilename, dtype=np.uint32, count=-1)
ofilename.close()

# Read all input data as complex int16 samples
ifilename = open(sys.argv[3], 'rb')
idata = np.fromfile(ifilename, dtype=np.uint32, count=-1)
ifilename.close()

if np.array_equal(idata, odata):
    print '    PASS: Input and output data files match'
else:
    print '    FAIL: Input and output data files not match'
    sys.exit(1)
