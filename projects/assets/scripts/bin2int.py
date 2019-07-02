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

"""Used for converting output FIR data from binary to ascii, and also removing
invalid start-up condition samples.
"""

import struct

import numpy as np
import sys
import os.path
import itertools

def bin2int_real(ifile, ofile, num_taps, filter_group_delay):
    print ("Input file: " + ifile + "\n" + "Output file: " + ofile)
    fr = open(ifile, 'rb')
    fw = open(ofile, 'w')

    # Initial testing has shown the output of the FIR may not start
    # at the first tap value, but rather start at a value of 0.
    # -
    # The following series of 'indexing' calls is an attempt to locate
    # the first tap value's index within the list, and perform an alignment
    # prior to comparing to the exemplar 'symmetric' taps file.
    a = np.fromfile(fr, dtype=np.int16,count=-1)
    l = a.tolist()
    adjusted = l[filter_group_delay-1:filter_group_delay-1+num_taps:]
    for i in range(len(adjusted)):
        fw.write(''.join(str(adjusted[i]))+'\n')
    fr.close()
    fw.close()

def bin2int_complex(ifile, ofile, num_taps, filter_group_delay):
    print ("Input file: " + ifile + "\n" + "Output file: " + ofile)
    fr = open(ifile, 'rb')
    fw = open(ofile, 'w')

    # Initial testing has shown the output of the FIR may not start
    # at the first tap value, but rather start at a value of 0.
    # -
    # The following series of 'indexing' calls is an attempt to locate
    # the first tap value's index within the list, and perform an alignment
    # prior to comparing to the exemplar 'symmetric' taps file.
    a = np.fromfile(fr, dtype=np.int16,count=-1)
    l = a.tolist()
    adjusted = l[2*(filter_group_delay-1):2*(filter_group_delay-1)+num_taps:]
    for i in range(len(adjusted)):
        fw.write(''.join(str(adjusted[i]))+'\n')
    fr.close()
    fw.close()

def main():
    bin2int_real(ifile, ofile, num_taps, filter_group_delay)
    bin2int_complex(ifile, ofile, num_taps, filter_group_delay)

if __name__ == '__main__':
    main()
