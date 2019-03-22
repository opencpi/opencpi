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


import os
import sys
import struct
import opencpi.colors as color


if len(sys.argv) != 2:
    print('Invalid arguments:  usage is: verify.py <output-file>')
    sys.exit(1)

txen = os.environ.get('OCPI_TEST_txen')
expected_opcode_str = "txOn" if (txen == "true") else "txOff";

f = open(sys.argv[1], 'rb')

tx_event_prot_operation_txOff = 0
tx_event_prot_operation_txOn = 1

exit_status = 0

def get_opcode_str(opcode):
  if(opcode == tx_event_prot_operation_txOff):
    return 'txOff'
  elif(opcode == tx_event_prot_operation_txOn):
    return 'txOn'
  else:
    return str(opcode)

try:
  # reading file_write.rcc output file whose name is passed in via sys.argv[1],
  # messagesInFile=true is assumed

  file_len_bytes = os.path.getsize(sys.argv[1])
  if(file_len_bytes == 0):
    raise ValueError('out port did not send any data (output file was empty)')

  # The first 32-bit word of the header is interpreted as the message length in
  # bytes
  msg_len_bytes = struct.unpack('i',f.read(4))[0] # 32/8 = 4 bytes
  if(msg_len_bytes != 0):
    e = 'out port sent message w/ non-zero message length of '
    e += str(msg_len_bytes)
    raise ValueError(e)

  # The next 8-bit byte is the opcode of the message, followed by 3 padding
  # bytes
  opcode = struct.unpack('i',f.read(4))[0] # 32/8 = 4 bytes
  if(get_opcode_str(opcode) != expected_opcode_str):
    raise ValueError('out port sent unexpected opcode: ' + get_opcode_str(opcode))

except ValueError as err:
  print color.RED + color.BOLD + 'FAILED, ' + err.message + color.END
  exit_status = 1

finally:
  f.close()
  sys.exit(exit_status)
