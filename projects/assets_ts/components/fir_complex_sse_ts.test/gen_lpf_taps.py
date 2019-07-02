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

"""
Generate an ASCII taps file for a symmetric low-pass filter

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

    fc = 1.0/float(os.environ.get("OCPI_TEST_samplesPerBaud"))
    if fc <= 0.0 or fc > 1.0:
        print("Exit: cutoff frequency out of range (0,1)")
        sys.exit(1)
    max_tap = pow(2,int(os.environ.get("OCPI_TEST_COEFF_WIDTH"))-1)-1
    #Num taps is different than the assets version, which requires a separate script
    length = int(os.environ.get("OCPI_TEST_NUM_TAPS"))

    result = np.array(np.zeros(length), dtype=float)
    ii = 0
    scale = 0.0

    #check for even number of taps
    if (length % 2 == 0):
        my_range = -((length)/2.0 - 0.5 )
    else: #odd
        my_range = -((length - 1) / 2.0)

    for i in np.arange(my_range, -my_range+1):
        #sinc function
        result[ii] = (np.sin(np.pi*i*fc))/(np.pi*i*fc)
        if np.isnan(result[ii]):
           result[ii] = 1.0
        #Hamming window
        result[ii] *= 0.54 - (0.46*np.cos((2*np.pi*ii)/(length-1)))
        if abs(result[ii]) > scale:
            scale = abs(result[ii])
        ii+=1

    #set taps scale to integers
    taps = (np.int16)(result * max_tap / scale)
    #print (float)(sum(taps))/(float)(max_tap)
    #print taps[0:int(np.ceil(length/2.0))]

    fo = open(sys.argv[1], 'w')
    #print "\tName of the output file:", fo.name
    for i in taps[0:int(np.ceil(length/2.0))]:
        stringy = ''.join(str(i)+'\n')
        fo.write(stringy)
    fo.close()


def main():
    set_taps_lpf()

if __name__ == '__main__':
    main()
