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

import os
import sys
import numpy as np

"""
Use this script to validate your output data against your input data.
Args: <list-of-user-defined-args> <output-file> <input-files>
"""
def main():

    max_count_value = int(os.environ.get('OCPI_TEST_max_count_value'))

    ofile = open(sys.argv[1], 'rb')
    contents = np.fromfile(ofile, dtype=np.uint8, count=-1)
    if max_count_value > 0:
        if(len(contents) == 0):
            msg = 'FAIL: Output file was empty when 1 or more messages was '
            msg += 'expected to be written to file'
            print(msg)
            sys.exit(1)

    print('PASS')

if __name__ == "__main__":
    main()
