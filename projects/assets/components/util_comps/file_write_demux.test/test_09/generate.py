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


# This script will write out values for file_read in message mode
# NOTE: It assumes all array values are bytes - if you have larger
# values needed, you are responsible for massaging the data into
# an array of bytes!

from os import mkdir
from errno import EEXIST
import struct

def addmsg(f, opcode, data):
  f.write(struct.pack("II",len(data),opcode)) # Two unsigned 32-bit
  if len(data):
    f.write(bytearray(data))

try:
  mkdir("idata")
except OSError as e:
  if e.errno != EEXIST:
    raise e

with open("idata/infile.bin", "wb") as ofile:
  for loop in xrange(0,2000):
    for opcode in xrange(0,256):
      addmsg(ofile, opcode, [opcode])
  # ZLM on 255 to stop:
  addmsg(ofile, 255, [])
