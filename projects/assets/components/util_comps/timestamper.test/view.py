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
Timestamper: View output data

View args:
1. Message size of input data to Timestamper / 4
2. Output file used for view

To test the Timestamper, a binary data file is generated containing real 
32-bit samples with a configurable ampltiude and length.

To validate the test, the timestamps are checked to ensure they are 
incrementing.
"""
import struct
import numpy as np
import sys
import os.path

OFILENAME = open(sys.argv[2], 'rb')
odata = np.fromfile(OFILENAME, dtype=np.uint32, count=-1)
OFILENAME.close()
MESSAGE_SIZE_WORDS = int(sys.argv[1])
enable = os.environ.get("OCPI_TEST_enable")

if(enable=="true"): # => NORMAL MODE
    #Test if timestamps are incrementing
    timestamp_list = list();
    a = 0
    while a < len(odata):
        if(a % (MESSAGE_SIZE_WORDS+2) == 0):
            if(len(timestamp_list) and (timestamp_list[-1] > odata[a]+1.0*(odata[a+1])/0xFFFFFFFF)):
                print "    Bad timestamp: " , timestamp_list[-1], " > ", odata[a]+1.0*(odata[a+1])/0xFFFFFFFF           
            timestamp_list.append(odata[a]+1.0*(odata[a+1])/0xFFFFFFFF)
            if(len(timestamp_list)>1):
                print "    Timestamp at index: ", str(a), " is:", "{:10.7f}".format(timestamp_list[-1]), " ( Seconds:", "{0:#x}".format(odata[a]), " Fraction:", "{0:#x}".format(odata[a+1]),") Delta:","{:10.7f}".format(timestamp_list[-1]-timestamp_list[-2]) 
            else:
                print "    Timestamp is:", "{:10.7f}".format(timestamp_list[-1]), " ( Seconds:", "{0:#x}".format(odata[a]), " Fraction:", "{0:#x}".format(odata[a+1]),")"
            a += 2
        else:
            a += 1
else: # => BYPASS MODE
    print "    View not supported for bypass mode"
