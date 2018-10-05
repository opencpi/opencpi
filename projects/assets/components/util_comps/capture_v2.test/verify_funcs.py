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
This script defines the functions used to validate the data captured by the Capture worker and used
by the verify.py script.
"""

import sys
import os.path
import struct
import numpy as np


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

# For testScenario 1
def verify_metadataCount1(metadataCount):
    # Because no messages were sent, metadataCount should be 1; 1 because a ZLM of opcode 0 is sent.
    if metadataCount != 1:
        print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 1
        sys.exit(1)
    else:
        print "    PASS: metadataCount is correct"

# For testScenario 2 and 4
def verify_metadataCount2(metadataCount, numRecords, stopOnFull):
        if stopOnFull == "false":
            # metadataCount should be numRecords + 1, since made metadata full and got a ZLM of opcode 0
            if metadataCount != numRecords+1:
                print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, numRecords+1
                sys.exit(1)
            else:
                print "    PASS: metadataCount is correct"
        elif stopOnFull == "true":
            if metadataCount != numRecords:
                print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, numRecords
                sys.exit(1)
            else:
                print "    PASS: metadataCount is correct"

# For testScenario 3
def verify_metadataCount3(metadataCount, numRecords, stopOnFull):
    if (stopOnFull == "false"):
        # metadataCount should be 3 because 2 messages are sent in this scenario and then there's a ZLM of opcode 0
        if metadataCount != 3:
            print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 3
            sys.exit(1)
        else:
            print "    PASS: metadataCount is correct"
    elif (stopOnFull == "true"):
        if numRecords >= 3:
            # metadataCount should be 3 because 2 messages are sent in this scenario and then there's a ZLM of opcode 0
            if metadataCount != 3:
                print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 3
                sys.exit(1)
            else:
                print "    PASS: metadataCount is correct"
        elif numRecords == 2:
            # if numRecords is 2 then only 2 messages are captured when stopOnFull is true. So metadataCount is 2.
            if metadataCount != 2:
                print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 2
                sys.exit(1)
            else:
                print "    PASS: metadataCount is correct"
        else:
            # if numRecords is 1 then only 1 messages is captured when stopOnFull is true. So metadataCount is 1.
            if metadataCount != 1:
                print "    " + color.RED + color.BOLD + "FAIL, metadataCount is", metadataCount,  "while expected value is" + color.END, 1
                sys.exit(1)
            else:
                print "    PASS: metadataCount is correct"


# For testScenario 1 and 2
def verify_dataCount1(dataCount):
    # dataCount should be 0 since no data sent in these scenarios
    if dataCount != 0:
        print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, 0
        sys.exit(1)
    else:
        print "    PASS: dataCount is correct"

# For testScenario 3
def verify_dataCount2(dataCount, numDataWords, stopOnFull):
    if stopOnFull == "false":
        # This scenario sends numDataWords+1 amount of data, so dataCount should be numDataWords+1 when stopOnFull is false
        if dataCount != numDataWords+1:
            print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, numDataWords+1
            sys.exit(1)
        else:
            print "    PASS: dataCount is correct"
    elif stopOnFull == "true":
        # For this scenario dataCount should be numDataWords when stopOnFull is true
        if dataCount != numDataWords:
            print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, numDataWords
            sys.exit(1)
        else:
            print "    PASS: dataCount is correct"

# For testScenario 4
def verify_dataCount3(dataCount, numDataWords, metadataCount, numRecords,stopOnFull):
    if stopOnFull == "false":
        # In this scenario, numDataWords amount of data is sent and then numRecords-5 amount of data is sent.
        # So when stopOnFull is true, dataCount should be numDataWords+numRecords-5
        if dataCount != numDataWords+numRecords-5:
            print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, numDataWords+numRecords-5
            sys.exit(1)
        else:
            print "    PASS: dataCount is correct"
    elif stopOnFull == "true":
        # For this scenario dataCount should be numDataWords when stopOnFull is true
        if dataCount != numDataWords:
            print "    " + color.RED + color.BOLD + "FAIL, dataCount is", dataCount,  "while expected value is" + color.END, numDataWords
            sys.exit(1)
        else:
            print "    PASS: dataCount is correct"

# For all testScenario
def verify_status(metaFull, dataFull, dataCount, numDataWords,metadataCount, numRecords):
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
    # Only data is full
    elif (dataCount >= numDataWords and metadataCount < numRecords):
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


# For testScenario 1
def verify_metadata1(metadata, numRecords):
    opcode = (metadata[0] & 0xFF000000) >> 24
    messageSize = (metadata[0] & 0x00FFFFFF)
    eom_fraction = metadata[1]
    som_fraction = metadata[2]
    # opcode and message size should both 0 since only a ZLM was sent
    if opcode != 0:
        print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
        print "    " + "For metadata record 1, ocpode is", opcode, "while expected value is" + color.END, "0"
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
    else:
        print   "    PASS: metadata is correct"

# For testScenario 2
def verify_metadata2(metadata, metadataCount, stopOnFull, numRecords):
    eom_frac_prev = 0
    som_frac_prev = 0
    if stopOnFull == "true":
        # Multiplying metadataCount by 4 because there are 4 metadata words
        stop = 4*metadataCount
    elif stopOnFull == "false":
        stop = 4*(metadataCount-1)
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
            if stopOnFull == "true":
                # For this scenario when stopOnFull is true, message size should be 0, because 0 bytes were sent, and opcode should be 32
                if opcode != 32:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "32"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
            elif stopOnFull == "false":
                # For this scenario when stopOnFull is false, there would have been a wrap around when the ZLM of opcode 0 was received
                # so message size should be 0, because 0 bytes were sent, and opcode should be 0
                if opcode != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
                # When stopOnFull is false,  there would have been a wrap around when the ZLM of opcode 0 was received.
                # So the first metadata records's timestamps should be greater than the last metadata record's timestamp
                if numRecords >= 2:
                    if eom_fraction <= metadata[(numRecords*4)-3]:
                        print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, eom fraction time stamp is not incrementing"  + color.END
                        sys.exit(1)
                    elif som_fraction <= metadata[(numRecords*4)-2]:
                        print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, som fraction time stamp is not incrementing" + color.END
                        sys.exit(1)
        else:
            # All the messages sent were ZLMs of opcode 32, so message size should be 0 and opcode should be 32
            if opcode != 32:
                print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "32"
                sys.exit(1)
            elif messageSize != 0:
                print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                sys.exit(1)
            if ((stopOnFull == "true") or (stopOnFull == "false" and x > 4)):
                # Check that eom fraction and som fraction timestamps are incrementing
                if eom_fraction <= eom_frac_prev:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", eom fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
                elif som_fraction <= som_frac_prev:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", som fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
        eom_frac_prev = eom_fraction
        som_frac_prev = som_fraction
    print   "    PASS: metadata is correct"

# For testScenario 3
def verify_metadata3(metadata, numRecords, numDataWords, stopOnFull):
    eom_frac_prev = 0
    som_frac_prev = 0
    if numRecords >= 3:
        stop = 4*3
    elif numRecords == 2:
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
        if ((stopOnFull == "true") or (stopOnFull == "false" and numRecords >= 3)):
            if x == 0:
                # For this scenario when stopOnFull is true, the first metadata record's
                # message size should be numDataWords*4 bytes, because numDataWords*4 bytes were sent, and opcode should be 255
                if opcode != 255:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "255"
                    sys.exit(1)
                elif messageSize != numDataWords*4:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, numDataWords*4
                    sys.exit(1)
            else:
                # The second metadata record's message size should be 4 bytes, because 4 bytes were sent, and opcode should be 255
                if x == 4:
                    if opcode != 255:
                        print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "255"
                        sys.exit(1)
                    elif messageSize != 4:
                        print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, 4
                        sys.exit(1)
                # The third metadata record's message size should be 0 bytes, because a ZLM was received, and opcode should be 0
                elif x == 8:
                    if opcode != 0:
                        print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "0"
                        sys.exit(1)
                    elif messageSize != 0:
                        print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                        sys.exit(1)
                # Check that eom fraction and som fraction timestamps are incrementing
                if eom_fraction <= eom_frac_prev:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", eom fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
                elif som_fraction <= som_frac_prev:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", som fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
        elif stopOnFull == "false":
            if numRecords == 1:
                # For this scenario when stopOnFull is false, there would have been a wrap around when the ZLM of opcode 0 was received
                # so message size should be 0, because 0 bytes were sent, and opcode should be 0
                if opcode != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
            elif numRecords == 2:
                if x == 0:
                    # Because a ZLM of opcode 0 gets sent and there is wrap around when stopOnFull is false, metadata[0] should be 0 bytes
                    # and metdata[1] should be 0 for opcode 0
                    if opcode != 0:
                        print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
                        sys.exit(1)
                    elif messageSize != 0:
                        print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                        sys.exit(1)
                    # When stopOnFull is false,  there would have been a wrap around when the ZLM of opcode 0 was received.
                    # So the first metadata records's timestamps should be greater than the second metadata record's timestamp
                    elif eom_fraction <= metadata[5]:
                        print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, eom fraction time stamp is not incrementing"  + color.END
                        sys.exit(1)
                    elif som_fraction <= metadata[6]:
                        print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, som fraction time stamp is not incrementing" + color.END
                        sys.exit(1)
                else:
                    # The second metadata record's message size should be 4 bytes, because 4 bytes were sent, and opcode should be 255
                    # Also check that eom fraction and som fraction timestamps are incrementing
                    if opcode != 255:
                        print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) + ", ocpode is", opcode, "while expected value is" + color.END, "255"
                        sys.exit(1)
                    elif messageSize != 4:
                        print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                        print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, 4
                        sys.exit(1)
        eom_frac_prev = eom_fraction
        som_frac_prev = som_fraction
    print   "    PASS: metadata is correct"

# For testScenario 4
def verify_metadata4(metadata, metadataCount, numRecords, stopOnFull, numDataWords):
    eom_frac_prev = 0
    som_frac_prev = 0
    stop = 4*(metadataCount-1)
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
            if stopOnFull == "true":
                # For this scenario when stopOnFull is true, the first metadata record's
                # message size should be 0 bytes and opcode should be 1
                if opcode != 1:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "1"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
            elif stopOnFull == "false":
                # For this scenario when stopOnFull is false, there would have been a wrap around when the ZLM of opcode 0 was received
                # so message size should be 0, because 0 bytes were sent, and opcode should be 0
                if opcode != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
                # When stopOnFull is false,  there would have been a wrap around when the ZLM of opcode 0 was received.
                # So the first metadata records's timestamps should be greater than the last metadata record's timestamp
                elif eom_fraction <= metadata[(numRecords*4)-3]:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, eom fraction time stamp is not incrementing"  + color.END
                    sys.exit(1)
                elif som_fraction <= metadata[(numRecords*4)-2]:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record 1, som fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
        else:
            if x == 4:
                # The second metadata record should contain opcode of 2 and messageSize of 0 bytes
                if opcode != 2:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "2"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
            elif x == 8:
                # The third metadata record should contain opcode of 3 and messageSize of 4 bytes
                if opcode != 3:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "3"
                    sys.exit(1)
                elif messageSize != 4:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "4"
                    sys.exit(1)
            elif x == 12:
                # The second metadata record should contain opcode of 0 and messageSize of (numDataWords-1)*4 bytes
                if opcode != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
                    sys.exit(1)
                elif messageSize != (numDataWords-1)*4:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, (numDataWords-1)*4
                    sys.exit(1)
            elif x == 16:
                # The third metadata record should contain opcode of 4 and messageSize of 0 bytes
                if opcode != 4:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "4"
                    sys.exit(1)
                elif messageSize != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "0"
                    sys.exit(1)
            elif x >= 20:
                # The other metadata records should contain opcode of 0 and messageSize of 4 bytes
                if opcode != 0:
                    print "    " + color.RED + color.BOLD + "FAIL, opcode is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", ocpode is", opcode, "while expected value is" + color.END, "0"
                    sys.exit(1)
                elif messageSize != 4:
                    print "    " + color.RED + color.BOLD + "FAIL, message size is not correct"
                    print "    " + "For metadata record " + str((x/4)+1) +  ", message size is", messageSize, "while expected value is" + color.END, "4"
                    sys.exit(1)
            if ((stopOnFull == "true") or (stopOnFull == "false" and x > 4)):
                # Check that eom fraction and som fraction timestamps are incrementing
                if eom_fraction <= eom_frac_prev:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", eom fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
                elif som_fraction <= som_frac_prev:
                    print "    " + color.RED + color.BOLD + "FAIL, for metadata record " + str((x/4)+1) + ", som fraction time stamp is not incrementing" + color.END
                    sys.exit(1)
        eom_frac_prev = eom_fraction
        som_frac_prev = som_fraction
    print   "    PASS: metadata is correct"


# For testScenario 3
def verify_data1(odata, dataCount, numDataWords, stopOnFull):
    # u4 is uint32 and setting it to little endian with "<"
    dt = np.dtype('<u4')
    idata = np.empty(numDataWords, dtype=dt)
    if stopOnFull == "true":
        for x in range(0, dataCount):
            idata[x] = x
    elif stopOnFull == "false":
        idata[0] = numDataWords
        for x in range(1, dataCount-1):
            idata[x] = x
    if np.array_equal(idata, odata):
        print "    PASS: Input and output data match"
    else:
        print "    " + color.RED + color.BOLD + "FAIL: Input and output data don't match" + color.END
        sys.exit(1)

# For testScenario 4
def verify_data2(odata, stopOnFull, numDataWords, numRecords):
    # u4 is uint32 and setting it to little endian with "<"
    dt = np.dtype('<u4')
    idata = np.empty(numDataWords, dtype=dt)
    if stopOnFull == "false":
        i = 0
        for x in range(numDataWords, numDataWords+numRecords-5):
            idata[i] = x
            i = i + 1
        for x in range(numDataWords+numRecords-5-numDataWords, numDataWords):
            idata[i] = x
            i = i + 1
    elif stopOnFull == "true":
        for x in range(0, numDataWords):
            idata[x] = x

    if np.array_equal(idata, odata):
        print "    PASS: Input and output data match"
    else:
        print "    " + color.RED + color.BOLD + "FAIL: Input and output data don't match" + color.END
        sys.exit(1)
