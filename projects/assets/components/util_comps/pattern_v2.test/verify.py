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
Use this script to validate the data sent by the Pattern_v2 worker.
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
   print("Invalid arguments:  usage is: verify.py <output-file>")
   sys.exit(1)

print "*** Validate output against expected data ***"


logname = os.path.splitext(sys.argv[1])[0]
logname = os.path.splitext(logname)[0] + ".log"

logfile = open(logname, 'r')
lines = logfile.read()
logfile.close()

dataRepeat = os.environ.get("OCPI_TEST_dataRepeat")
messagesToSend = re.findall("pattern_v2.messagesToSend = \"(\d+)\".*", lines)
messagesToSend_initial = int(messagesToSend[0])
messagesToSend_final =  int(messagesToSend[1])
messagesSent = re.findall("pattern_v2.messagesSent = \"(\d+)\".*", lines)
messagesSent_final = int(messagesSent[1])
dataSent = re.findall("pattern_v2.dataSent = \"(\d+)\".*", lines)
dataSent_final = int(dataSent[1])
numMessagesMax = int(os.environ.get("OCPI_TEST_numMessagesMax"))


obj1 = re.search("pattern_v2.messages = \"([^\"]*)\".*", lines)
messages = [int(x) for x in obj1.group(1).replace("{", "").replace("}", "").split(',')]

ofilename = open(sys.argv[1], 'rb')
odata = np.fromfile(ofilename, dtype=np.uint32)
ofilename.close()

# Check that the final value of messagesToSend is 0
if (messagesToSend_final != 0):
    print '    ' + color.RED + color.BOLD + 'FAIL, the final value of messagesToSend = ', messagesToSend_final, 'while expected value is' + color.END, 0
    sys.exit(1)
else:
    print '    PASS: The final value of messagesToSend is correct'

# Check that the final value of  messageSent is the correct value.
# The correct value is messagesToSend_initial + 1 (plus 1 because there's a zlm of opcode 0 that is sent as well)
if (messagesSent_final != messagesToSend_initial + 1):
    print '    ' + color.RED + color.BOLD + 'FAIL, the final value of messagesSent = ', messagesSent_final, 'while expected value is' + color.END,  messagesToSend_initial+1
    sys.exit(1)
else:
    print '    PASS: The final value of messageSent is correct'

# Generate the expected data that was sent to compare to the output data and also grab the number
# of data sent for each message
msg_len = messagesToSend_initial*2
bytes = 0
data = np.array([], dtype=np.uint32)
start = 0
for x in range(0, msg_len, 2):
    bytes = bytes + messages[x]
    stop = messages[x]/4
    if (dataRepeat == "true"):
        data = np.append(data, np.arange(0,stop))
    elif (dataRepeat == "false"):
        data = np.append(data, np.arange(start, start+stop))
        start += stop


# Divide by 4 because data is 4 byte data
numData = bytes/4

# Check that the final value of dataSent is correct
if (dataSent_final != numData):
    print '    ' + color.RED + color.BOLD + 'FAIL, the final value of dataSent = ', dataSent_final, 'while expected value is' + color.END,  numData
    sys.exit(1)
else:
    print '    PASS: The final value of dataSent is correct'

# Check that output data matches the input data
if np.array_equal(data, odata):
	print '    PASS: Input data and output data match'
else:
	print '    FAIL: Input data and output data do not match'
	sys.exit(1)

print '    Data matched expected results.'
print '    ' + color.GREEN + color.BOLD + 'PASSED' + color.END
