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


"""
Stream Random Metadata: Generate test data
"""
import sys
import os
import struct
import numpy as np
import random

if len(sys.argv) != 4:
    print("Invalid arguments:  usage is: generate.py <num-samples> <messages in file? T=1/F=0> <output-file>")
    sys.exit(1)

filename = sys.argv[3]
num_samples = int(sys.argv[1])
MiF = int(sys.argv[2])

numDataWords = 1024
numRecords = 256

def noMessages():
    with open(filename, 'wb') as f:
        for i in xrange(num_samples-1):
            randval = random.getrandbits(32)
            f.write(struct.pack('I',randval))
    f.close()

def messagesInFile():
    f = open(filename, 'wb')

    # Send multiple ZLMs and SWMs
    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 1))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 2))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 10))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 11))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 4))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 5))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 6))

    # Send a single word message
    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 11))
    f.write(struct.pack("I", 0))


    # Send another message to fill up data buffer
    f.write(struct.pack("I", (numDataWords-1)*4))
    f.write(struct.pack("I", 0))

    for x in range(1, numDataWords):
        f.write(struct.pack("I", x))

    # Send more ZLMs and SWMs
    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 1))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 2))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))

    f.write(struct.pack("I", 0))
    f.write(struct.pack("I", 4))

    f.write(struct.pack("I", 4))
    f.write(struct.pack("I", 3))
    f.write(struct.pack("I", 0))


    # Fill up metadata buffer
    for x in range(numDataWords, numDataWords+numRecords-19):
        f.write(struct.pack("I", 4))
        f.write(struct.pack("I", 0))
        f.write(struct.pack("I", x))
    f.close()

def main():
    if (MiF == 0):
        noMessages()
    elif (MiF == 1):
        messagesInFile()

main()
