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

"""Generate the taps for a symmetric rrc filter

"""

import sys
import os.path
import numpy as np

def main():
    print(("\n","*"*80))
    print("*** Python: Generate Root-Raised Cosine taps ***")
    if len(sys.argv) < 2:
        print("Exit: must enter the Length of the filter in samples")
        return
    elif len(sys.argv) < 3:
        print("Exit: must enter the roll-off factor Alpha in the range (0,1)")
        return
    elif len(sys.argv) < 4:
        print("Exit: must enter the symbol period Ts in seconds (1/baud_rate)")
        return
    elif len(sys.argv) < 5:
        print("Exit: must enter the sample rate Fs in Hertz")
        return
    elif len(sys.argv) < 6:
        print("Exit: must enter a value for the Maximum Tap")
        return
    elif len(sys.argv) < 7:
        print("Exit: must enter an output file")
        return

    length = int(sys.argv[1])
    alpha = float(sys.argv[2])
    if alpha <= 0.0 or alpha > 1.0:
        print("Exit: alpha out of range (0,1)")
        return
    Ts = float(sys.argv[3])
    Fs = float(sys.argv[4]);
    max_tap = int(sys.argv[5]);

    import filters
    time_idx, h_rrc = filters.rrcosfilter(length, alpha, Ts, Fs)
    #print time_idx
    #print h_rrc

    #set taps scale to integers
    scale = max(abs(h_rrc))
    print(scale)
    taps = (np.int16)(h_rrc * max_tap / scale)
    print(taps)
    print((sum(taps)))
    print(((sum(taps))/max_tap))#variables were previously cast to float 
    #print taps[0:int(np.ceil(length/2.0))]

    fo = open(sys.argv[6], 'w')
    #print "\tName of the output file:", fo.name
    for i in taps[0:int(np.ceil(length/2.0))]:
        stringy = ''.join(str(i)+'\n')
        fo.write(stringy)
    fo.close()

if __name__ == '__main__':
    main()
