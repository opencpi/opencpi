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

import opencpi.unit_test_utils as utu
import numpy as np
import sys
import os.path

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

# from OCS or OWD
WIDTH = int(os.environ.get("OCPI_TEST_width"))

with open(sys.argv[1], 'wb') as f:
    for i in range(int(os.environ.get("OCPI_TEST_ocpi_max_opcode_in"))):
        if WIDTH == 32:
            utu.add_msg(f, i, np.arange(i, dtype=np.int32))
        else:
            print("Unsupported data width")
            sys.exit(1)
