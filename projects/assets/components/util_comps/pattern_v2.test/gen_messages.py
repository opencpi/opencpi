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
Generates data for the messages property
"""

import sys
import os.path
import struct

if len(sys.argv) != 2:
    print("Invalid arguments:  usage is: generate.py <output-file>")
    sys.exit(1)

numMessagesMax = int(os.environ.get("OCPI_TEST_numMessagesMax"))
messages = []
bytes = 4
opcode = 1
for x in range(0, numMessagesMax*2):
    if x % 2 == 0:
        messages.append(bytes)
        bytes = bytes + 4
    else:
        messages.append(opcode)
        if opcode < 255:
            opcode = opcode + 1
        else:
            opcode = 1

with open(sys.argv[1], 'w') as f:
    for x in range(0, numMessagesMax*2):
        # Prepend '{' to first message field
        if x % 2 == 0:
            f.write('{')
        f.write(str(messages[x]))
        # Append '}' to second message field
        if x % 2:
            f.write('}')
        f.write('\n')
