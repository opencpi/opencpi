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

"""
Use this script to validate the data sent by the Pattern_v2 worker.
"""

import os.path
import re
import sys
import struct
import opencpi.colors as color
import numpy as np


if len(sys.argv) != 2:
   print("Invalid arguments:  usage is: verify.py <output-file>")
   sys.exit(1)


logName = os.path.splitext(sys.argv[1])[0]
logName = os.path.splitext(logName)[0] + ".log"

logFile = open(logName, 'r')
lines = logFile.read()
logFile.close()

dataRepeat = os.environ.get("OCPI_TEST_dataRepeat")
messagesToSend = re.findall("pattern_v2.messagesToSend = \"(\d+)\".*", lines)
messagesToSendInitial = int(messagesToSend[0])
messagesToSendFinal =  int(messagesToSend[1])
messagesSent = re.findall("pattern_v2.messagesSent = \"(\d+)\".*", lines)
messagesSentFinal = int(messagesSent[1])
dataSent = re.findall("pattern_v2.dataSent = \"(\d+)\".*", lines)
dataSentFinal = int(dataSent[1])
numMessagesMax = int(os.environ.get("OCPI_TEST_numMessagesMax"))


obj1 = re.search("pattern_v2.messages = \"([^\"]*)\".*", lines)
messages = [int(x) for x in obj1.group(1).replace("{", "").replace("}", "").split(',')]

# Gets the opcode and data written to file
def parse_odata(opcode, odata):
    bytes = 0
    with open(sys.argv[1], 'rb') as f:
        for x in range(0, messagesToSendInitial):
            # The contents for a message are arranged in file in this order: bytes,
            # opcode, and then data for the message
            bytes = struct.unpack("<I",f.read(4))[0]
            opcode = np.append(opcode, struct.unpack("<I",f.read(4))[0])
            for y in range(0, bytes//4):
                odata = np.append(odata, struct.unpack("<I",f.read(4))[0])
    return opcode, odata

# Check that the final value of messagesToSend is 0
if (messagesToSendFinal != 0):
    print ('    The final value of messagesToSend = ', messagesToSendFinal, 'while expected value is 0')
    sys.exit(1)
else:
    print ('    The final value of messagesToSend is correct')

# Check that the final value of  messageSent is the correct value.
if (messagesSentFinal != messagesToSendInitial):
    print ('    The final value of messagesSent = ', messagesSentFinal, 'while expected value is',  messagesToSendInitial)
    sys.exit(1)
else:
    print ('    The final value of messageSent is correct')

# Generate the expected data that was sent to compare to the output data and also grab the number
# of data sent for each message
# Multiplying messagesToSendInitial by 2 because there are 2 message fields
messagesLength = messagesToSendInitial*2
bytes = 0
opcodesSent = np.array([], dtype=np.uint32)
data = np.array([], dtype=np.uint32)
start = 0
for x in range(0, messagesLength, 2):
    bytes = bytes + messages[x]
    opcodesSent = np.append(opcodesSent, messages[x+1])
    stop = messages[x]//4
    if (dataRepeat == "true"):
        data = np.append(data, np.arange(0,stop))
    elif (dataRepeat == "false"):
        data = np.append(data, np.arange(start, start+stop))
        start += stop

opcodeReceived = np.array([], dtype=np.uint32)
odata = np.array([], dtype=np.uint32)

opcodeReceived, odata = parse_odata(opcodeReceived, odata)

# Divide by 4 because data is 4 byte data
numData = bytes//4

# Check that the final value of dataSent is correct
if (dataSentFinal != numData):
    print ('    The final value of dataSent = ', dataSentFinal, 'while expected value is',  numData)
    sys.exit(1)
else:
    print ('    The final value of dataSent is correct')

# Check that opcodes sent match opcodes received
if np.array_equal(opcodesSent, opcodeReceived):
	print ('    opcodes sent match opcodes received')
else:
	print ('    opcodes sent do not all match opcodes received')
	sys.exit(1)


# Check that output data matches the input data
if np.array_equal(data, odata):
	print ('    Data sent and output data match')
else:
	print ('    Data sent and output data do not match')
	sys.exit(1)
