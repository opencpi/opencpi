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
Use this script to validate the data captured by the Capture_v2 worker.
"""

import sys
import os.path
import struct
import numpy as np
import re

class color:
    PURPLE = '\033[95m'
    CYAN = '\033[96m'
    DARKCYAN = '\033[36m'
    BLUE = '\033[94m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    BOLD = '\033[1m'
    UNDERLINE = '\033[4m'
    END = '\033[0m'

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: verify.py <props-file>")
    sys.exit(1)


print "*** Validate output against expected data ***"

filename = sys.argv[1]

# Do some parsing to grab the final values of the properties
ofile = open(filename, 'r')
lines = ofile.read()
ofile.close()

obj1 = re.search(r'capture_v2 stopOnFull (\w+)', lines)
obj2 = re.search(r'capture_v2 metadataCount (\w+)', lines)
obj3 = re.search(r'capture_v2 dataCount (\w+)', lines)
obj4 = re.search(r'capture_v2 numRecords (\w+)', lines)
obj5 = re.search(r'capture_v2 numDataWords (\w+)', lines)
obj6 = re.search(r'capture_v2 metaFull (\w+)', lines)
obj7 = re.search(r'capture_v2 dataFull (\w+)', lines)
obj8 = re.search(r'\bcapture_v2 metadata\b', lines)
obj9 = re.search(r'\bcapture_v2 data\b', lines)
obj10 = re.search("capture_v2 ocpi_debug", lines)
metadata =  [int(x) for x in lines[obj8.end()+1:obj9.start()-1].replace("{", "").replace("}", "").split(',')]
# u4 is uint32 and setting it to little endian with "<"
dt = np.dtype('<u4')
odata = np.array(lines[obj9.end()+1:obj10.start()-1].split(','), dtype=dt)

stopOnFull = obj1.group(1)
metadataCount = int(obj2.group(1))
dataCount = int(obj3.group(1))
numRecords = int(obj4.group(1))
numDataWords = int(obj5.group(1))
metaFull = obj6.group(1)
dataFull = obj7.group(1)

print "    stopOnFull: " + stopOnFull
print "    metadataCount: " + str(metadataCount)
print "    dataCount: " + str(dataCount)
print "    numRecords: " + str(numRecords)
print "    numDataWords: " + str(numDataWords)
print "    metaFull: " + metaFull
print "    dataFull: " + dataFull



# Check if the metadataCount matches the expected metadataCount
if (numRecords >= 2):
    if metadataCount != 2:
        print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 2
        sys.exit(1)
    else:
        print "    PASS: metadataCount is correct"
else:
    if metadataCount != 1:
        print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 1
        sys.exit(1)
    else:
        print "    PASS: metadataCount is correct"

# Check if the dataCount matches the expected dataCount
if dataCount != numDataWords:
    print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, numDataWords
    sys.exit(1)
else:
    print "    PASS: dataCount is correct"

# Check if the status (metaFull and dataFull) matches the expeceted status

# Only data is full
if (dataCount >= numDataWords and metadataCount < numRecords):
    if (metaFull != "false" or dataFull != "true"):
        print "    " + color.RED + color.BOLD + "FAIL, metaFull and/or dataFull are not correct"
        print "    " + "metaFull and dataFull are " + metaFull + " and " + dataFull + " while expected values are" + color.END, "false and true"
        sys.exit(1)
    else:
        print "    PASS: status is correct"
# Both data and metadata are full
elif (dataCount >= numDataWords and metadataCount >= numRecords):
    if (metaFull != "true" or dataFull != "true"):
        print "    " + color.RED + color.BOLD + "FAIL, metaFull and/or dataFull are not correct"
        print "    " + "metaFull and dataFull are " + metaFull + " and " + dataFull + " while expected values are " + color.END, "true and true"
        sys.exit(1)
    else:
        print "    PASS: status is correct"


eom_frac_prev = 0
som_frac_prev = 0
if numRecords >= 2:
    stop = 4*2
else:
    stop = 4
for x in range(0, stop, 4):
    opcode = (metadata[x] & 0xFF000000) >> 24
    messageSize = (metadata[x] & 0x00FFFFFF)
    eom_fraction = metadata[x+1]
    som_fraction = metadata[x+2]
    # Check that timestamps are non-zero
    if eom_fraction == 0:
            print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", eom fraction time stamp is 0"  + color.END
            sys.exit(1)
    elif som_fraction == 0:
        print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", som fraction time stamp is 0" + color.END
        sys.exit(1)
    if x == 0:
        # The first metadata record should contain opcode of 0 and messageSize of numDataWords*4 bytes
        if opcode != 0:
            print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
            print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "0"
            sys.exit(1)
        elif messageSize != numDataWords*4:
            print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
            print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, numDataWords*4
            sys.exit(1)
    else:
        # The second metadata record should contain opcode of 0 and messageSize of 0 bytes
        if opcode != 0:
            print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
            print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
            sys.exit(1)
        elif messageSize != 0:
            print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
            print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
            sys.exit(1)
        # Check that eom fraction and som fraction timestamps are incrementing
        elif eom_fraction <= eom_frac_prev:
            print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", eom fraction time stamp is not incrementing" + color.END
            sys.exit(1)
        elif som_fraction <= som_frac_prev:
            print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", som fraction time stamp is not incrementing" + color.END
            sys.exit(1)
    eom_frac_prev = eom_fraction
    som_frac_prev = som_fraction



# Check if data is correct
idata = np.empty(numDataWords, dtype=np.uint32)
for x in range(0, dataCount):
    idata[x] = x
if (odata == idata).all():
    print "    PASS: Input and output data match"
else:
    print "    " + color.RED + color.BOLD + "FAIL: Input and output data don't match" + color.END
    sys.exit(1)


print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END
