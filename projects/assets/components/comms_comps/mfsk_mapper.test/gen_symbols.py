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
Generate a ASCII symbols file for MFSK mapper

Generate args:
1. Output filename
"""

import os
import sys
import numpy as np

def set_taps_lpf():
    if len(sys.argv) < 2:
        print("Exit: must enter an output file")
        sys.exit(1)

    symbols = np.arange(0,int(os.environ.get("OCPI_TEST_M_p")))
    
    fo = open(sys.argv[1], 'w')
    for i in symbols:
        stringy = ''.join(str(i)+'\n')
        fo.write(stringy)
    fo.close()


def main():
    set_taps_lpf()

if __name__ == '__main__':
    main()
