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

import sys
import os.path
import struct

"""
Generate script
"""

if len(sys.argv) != 2:
    print ("Don't run this script manually, it is called by 'ocpidev test' or 'make test'")
    sys.exit(1)


numDataWords = int(os.environ.get("OCPI_TEST_numDataWords"))
numRecords = int(os.environ.get("OCPI_TEST_numRecords"))
testScenario = int(os.environ.get("OCPI_TEST_testScenario"))


# Test sending no data
def testScenario1():
    filename = sys.argv[1]
    f = open(filename, 'wb')

    f.close()

# Test filling up only metadata
def testScenario2():
    filename = sys.argv[1]
    f = open(filename, 'wb')
    # Send ZLMs not opcode 0 to fill up metadata
    for x in range(0, numRecords):
        f.write(struct.pack("<I", 0))
        f.write(struct.pack("<I", 32))
    f.close()

# Test filling up only data
def testScenario3():
    filename = sys.argv[1]
    f = open(filename, 'wb')

    f.write(struct.pack("<I", numDataWords*4))
    f.write(struct.pack("<I", 255))

    for x in range(0, numDataWords):
        f.write(struct.pack("<I", x))

    f.write(struct.pack("<I", 4))
    f.write(struct.pack("<I", 255))
    f.write(struct.pack("<I", numDataWords))
    f.close()

# Test sending a multiple zlms, a single word message, filling data and filling up metadata
def testScenario4():
    filename = sys.argv[1]
    f = open(filename, 'wb')

    # Send multiple ZLMs
    f.write(struct.pack("<I", 0))
    f.write(struct.pack("<I", 1))

    f.write(struct.pack("<I", 0))
    f.write(struct.pack("<I", 2))

    # Send a single word message
    f.write(struct.pack("<I", 4))
    f.write(struct.pack("<I", 3))
    f.write(struct.pack("<I", 0))


    # Send another message to fill up data buffer
    f.write(struct.pack("<I", (numDataWords-1)*4))
    f.write(struct.pack("<I", 0))

    for x in range(1, numDataWords):
        f.write(struct.pack("<I", x))

    # Send another ZLM
    f.write(struct.pack("<I", 0))
    f.write(struct.pack("<I", 4))


    # Fill up metadata buffer
    for x in range(numDataWords, numDataWords+numRecords-5):
        f.write(struct.pack("<I", 4))
        f.write(struct.pack("<I", 0))
        f.write(struct.pack("<I", x))

    f.close()

# Test no output port connected and sending a stopZLMOpcode opcode
def testScenario5():
    filename = sys.argv[1]
    f = open(filename, 'wb')

    # Send ZLM with stopZLMOpcode opcode
    f.write(struct.pack("<I", 0))
    f.write(struct.pack("<I", 255))

    f.close()

def main():
   if (testScenario == 1):
       testScenario1()
   elif (testScenario == 2):
       testScenario2()
   elif (testScenario == 3):
       testScenario3()
   elif (testScenario == 4):
       testScenario4()
   elif (testScenario == 5):
       testScenario5()
   else:
       print("Invalid testScenario: valid testScenario are 1, 2, 3 and 4")
       sys.exit(1)

main()
