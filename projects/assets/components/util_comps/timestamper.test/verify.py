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
Timestamper: Verify output data

Verify args:
1. Message size of input data to Timestamper / 4
2. Output file used for validation
3. Input file used for comparison

To test the Timestamper, a binary data file is generated containing real 
32-bit samples with a configurable ampltiude and length.

To validate the test, the size of the output file is verified, and the
timestamps are checked to ensure they are incrementing.
"""
import struct
import numpy as np
import sys
import os.path

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: verify.py <message-size-words> <output-file> <input-file>")
    sys.exit(1)

#Open input file and grab samples as int32
IFILENAME = open(sys.argv[3], 'rb')
idata = np.fromfile(IFILENAME, dtype=np.uint32, count=-1)
IFILENAME.close()

#Open output file and grab samples as int32 
OFILENAME = open(sys.argv[2], 'rb')
odata = np.fromfile(OFILENAME, dtype=np.uint32, count=-1)
OFILENAME.close()

MESSAGE_SIZE_WORDS = int(sys.argv[1])
enable = os.environ.get("OCPI_TEST_enable")

# Test that odata is not all zeros
if all(odata == 0):
    print "    FAILED: Values are all zero"
    sys.exit(1)
else:
    print "    PASS: File is not all zeros"

if(enable=="true"): # => NORMAL MODE
    #Test if timestamps are incrementing
    ofilename_without_timestamps = sys.argv[2] + ".no_timestamps"
    output_file_without_timestamps =  open(ofilename_without_timestamps, 'wb')
    timestamp_list = list();
    timestamp_error = 0
    a = 0
    while a < len(odata):
        if(a % (MESSAGE_SIZE_WORDS+2) == 0):
            if(len(timestamp_list) and (timestamp_list[-1] > odata[a]+1.0*(odata[a+1])/0xFFFFFFFF)):
                timestamp_error = 1
            timestamp_list.append(odata[a]+1.0*(odata[a+1])/0xFFFFFFFF)
            a += 2
        else:
            output_file_without_timestamps.write(struct.pack('i', odata[a]))
            a += 1
    output_file_without_timestamps.close()

    if(timestamp_error):
        print "    FAILED: one or more timestamps were not incrementing"
        sys.exit(1)
    else:
        print "    PASS: Timestamps are incrementing"

        #Test that odata is the expected amount
        if len(odata) != len(idata)+len(timestamp_list)*2:
            print "    FAILED: Output file length is unexpected"
            print 'Length =', len(odata), 'while expected length is =', len(idata)+len(timestamp_list)*2
            sys.exit(1)
        else:
            print "    PASS: Output file length = input file length plus timestamps"
else: # => BYPASS MODE
    #Test that odata is the expected amount
    if (idata != odata).all():
        print "    FAILED: Input and output file do not match"
        sys.exit(1)
    else:
        print "    PASS: Input and output file match"
