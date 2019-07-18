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
oneshot = os.environ.get("OCPI_TEST_oneshot")

if(oneshot=="false"): # => NORMAL MODE
    #Test that odata file length is the expected amount
    if len(odata) != len(idata):
        print "    FAILED: Output file length is unexpected"
        print 'Length =', len(odata), 'while expected length is =', len(idata)
        sys.exit(1)
    else:
        print "    PASS: Output file length = input file length"
    #Test that odata file is same as idata file
    if (idata != odata).all():
        print "    FAILED: Input and output file do not match"
        sys.exit(1)
    else:
        print "    PASS: Input and output file match"
else: # => ONESHOT MODE
    #Test that odata file length is the expected amount
    if len(odata) != min(len(idata),8192):
        print "    FAILED: Output file length is unexpected"
        print 'Length =', len(odata), 'while expected length is =', len(idata)
        sys.exit(1)
    else:
        print "    PASS: Output file length = min(input file length,8192)"
    #Test that odata file is same as idata file (up to 8192 samples)
    fail=0
    for idx in range(0,min(len(idata),8192)):
        if odata[idx] != idata[idx]:
            print "    FAILED: Input and output file do not match"
            fail=1
            break
    if fail == 0:
        print "    PASS: Input and output file match (up to 8192 samples)"

