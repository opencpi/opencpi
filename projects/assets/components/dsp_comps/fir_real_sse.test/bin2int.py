#!/usr/bin/env python
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

def main():
    print "\n*** Python: Convert binary file to text file ***"
    if len(sys.argv) < 2:
        print("Exit: Must enter an input file")
        return
    elif len(sys.argv) <3:
        print("Exit: Must enter an output file")
        return
    elif len(sys.argv) <4:
        print("Exit: Must enter a number of taps")
        return
    elif len(sys.argv) <5:
        print("Exit: Must enter the filter group delay")
        return

    print "Input file:", sys.argv[1], "\n", "Output file:", sys.argv[2]

    fr = open(sys.argv[1], 'rb')
    fw = open(sys.argv[2], 'w')

    # Initial testing has shown the output of the FIR may not start
    # at the first tap value, but rather start at a value of 0.
    # -
    # The following series of 'indexing' calls is an attempt to locate
    # the first tap value's index within the list, and perform an alignment
    # prior to comparing to the exemplar 'symmetric' taps file.
    a = np.fromfile(fr, dtype=np.int16,count=-1)
    l = a.tolist()
    adjusted = l[int(sys.argv[4])-1:int(sys.argv[4])-1+int(sys.argv[3]):]
    for i in range(len(adjusted)):
        fw.write(''.join(str(adjusted[i]))+'\n')
    fr.close()
    fw.close()
    print '***End Conversion***\n'

if __name__ == '__main__':
    main()
