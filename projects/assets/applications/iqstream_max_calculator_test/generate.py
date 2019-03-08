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

import sys
import struct

"""
Use this file to generate your input data.
Args: <list-of-user-defined-args>
"""

if len(sys.argv) != 6:
  print("Invalid arguments:  usage is: generate.py <output-file> <I> <Q> <EOFonly> <numZLMs>")
  sys.exit(1)

def main():
  filename = sys.argv[1]
  I = int(sys.argv[2])
  Q = int(sys.argv[3])
  EOFonly = int(sys.argv[4])
  numZLMs = int(sys.argv[5])
  f = open(filename, 'wb')

  iqstream_protocol_operation_iq = 0

  if numZLMs > 0:
    for ii in range(numZLMs):
      opcode = 0

      # The first 32-bit word of the header is interpreted as the message length in bytes
      message_length = 0 # ZLM
      f.write(struct.pack("I", message_length))

      # The next 8-bit byte is the opcode of the message, followed by 3 padding bytes
      f.write(struct.pack("B", opcode))
      f.write(struct.pack("B", 0))
      f.write(struct.pack("B", 0))
      f.write(struct.pack("B", 0))
  else:
    if EOFonly == 0:
      # The first 32-bit word of the header is interpreted as the message length in bytes
      message_length = 4
      f.write(struct.pack("I", message_length))

      # The next 8-bit byte is the opcode of the message, followed by 3 padding bytes
      f.write(struct.pack("B", iqstream_protocol_operation_iq))
      f.write(struct.pack("B", 0))
      f.write(struct.pack("B", 0))
      f.write(struct.pack("B", 0))

      # data (I)
      f.write(struct.pack("h", I))

      # data (Q)
      f.write(struct.pack("h", Q))

#  EOF_opcode = 0 # assumption of file_read/file_write
#
#  # The first 32-bit word of the header is interpreted as the message length in bytes
#  message_length = 0 # ZLM
#  f.write(struct.pack("I", message_length))
#
#  # The next 8-bit byte is the opcode of the message, followed by 3 padding bytes
#  f.write(struct.pack("B", EOF_opcode))
#  f.write(struct.pack("B", 0))
#  f.write(struct.pack("B", 0))
#  f.write(struct.pack("B", 0))

  f.close()

main()
