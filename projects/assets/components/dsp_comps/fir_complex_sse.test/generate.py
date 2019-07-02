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

Generate args:
 * calls a python script to generate ASCII and binary impulse files

To test the FIR Complex filter, a binary data file is generated containing an impulse.
The output of the filter is thus an impulse response, showing the symmetric tap values.

"""

import sys
import os.path


import gen_impulse
gen_impulse.impulse_cmplx_ascii(os.path.splitext(sys.argv[1])[0]+'.dat')
gen_impulse.impulse_cmplx_bin(sys.argv[1])
