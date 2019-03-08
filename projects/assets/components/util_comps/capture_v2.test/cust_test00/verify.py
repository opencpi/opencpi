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

obj1 = re.search(r'capture_v2 metadataCount (\w+)', lines)
obj2 = re.search(r'capture_v2 dataCount (\w+)', lines)
obj3 = re.search(r'capture_v2 numRecords (\w+)', lines)
obj4 = re.search(r'capture_v2 numDataWords (\w+)', lines)
obj5 = re.search(r'capture_v2 metaFull (\w+)', lines)
obj6 = re.search(r'capture_v2 dataFull (\w+)', lines)
obj7 = re.search(r'capture_v2 stopZLMOpcode (\w+)', lines)
obj8 = re.search(r'\bcapture_v2 metadata\b', lines)
obj9 = re.search(r'\bcapture_v2 data\b', lines)
obj10 = re.search("capture_v2 ocpi_debug", lines)
metadata =  [int(x) for x in lines[obj8.end()+1:obj9.start()-1].replace("{", "").replace("}", "").split(',')]
# u4 is uint32 and setting it to little endian with "<"
dt = np.dtype('<u4')
odata = np.array(lines[obj9.end()+1:obj10.start()-1].split(','), dtype=dt)

metadataCount = int(obj1.group(1))
dataCount = int(obj2.group(1))
numRecords = int(obj3.group(1))
numDataWords = int(obj4.group(1))
metaFull = obj5.group(1)
dataFull = obj6.group(1)
stopZLMOpcode = int(obj7.group(1))


print "    metadataCount: " + str(metadataCount)
print "    dataCount: " + str(dataCount)
print "    numRecords: " + str(numRecords)
print "    numDataWords: " + str(numDataWords)
print "    metaFull: " + metaFull
print "    dataFull: " + dataFull
print "    stopZLMOpcode: " + str(stopZLMOpcode)


# Check if the metadataCount matches the expected metadataCount
if metadataCount != 1:
    print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 1
    sys.exit(1)
else:
    print "    PASS: metadataCount is correct"

# Check if the dataCount matches the expected dataCount
if dataCount != 0:
    print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, 0
    sys.exit(1)
else:
    print "    PASS: dataCount is correct"

# Check if the status (metaFull and dataFull) matches the expeceted status

# Neither data or metadata are full
if (dataCount < numDataWords and metadataCount < numRecords):
    if (metaFull != "false" or dataFull != "false"):
        print "    " + color.RED + color.BOLD + "FAIL, metaFull and/or dataFull are not correct"
        print "    " + "metaFull and dataFull are " + metaFull + " and " + dataFull + " while expected values are" + color.END, "false and false"
        sys.exit(1)
    else:
        print "    PASS: status is correct"
# Only metadata is full
elif (metadataCount >= numRecords and dataCount < numDataWords):
    if (metaFull != "true" or dataFull != "false"):
        print "    " + color.RED + color.BOLD + "FAIL, metaFull and/or dataFull are not correct"
        print "    " + "metaFull and dataFull are " + metaFull + " and " + dataFull + " while expected values are" + color.END, "true and false"
        sys.exit(1)
    else:
        print "    PASS: status is correct"

opcode = (metadata[0] & 0xFF000000) >> 24
messageSize = (metadata[0] & 0x00FFFFFF)
eom_fraction = metadata[1]
som_fraction = metadata[2]

# opcode should be stopZLMOpcode and message size should be 0
if opcode != stopZLMOpcode:
    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
    print "    " + "For metadata record 1, ocpode is", opcode, "while expected value is" + color.END, stopZLMOpcode
    sys.exit(1)
elif messageSize != 0:
    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
    print "    " + "For metadata record 1, message size is", messageSize, "while expected value is" + color.END, "0"
    sys.exit(1)
# Check that timestamps are non-zero
elif eom_fraction == 0:
        print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, eom fraction time stamp is 0"  + color.END
        sys.exit(1)
elif som_fraction == 0:
    print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, som fraction time stamp is 0" + color.END
    sys.exit(1)



print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END
