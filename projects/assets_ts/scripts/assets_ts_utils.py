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
import numpy as np
import struct

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

# Declare ComplexShortWithMetadata opcode encoding
SAMPLES_OPCODE  = 0
TIME_OPCODE     = 1
INTERVAL_OPCODE = 2
FLUSH_OPCODE    = 3
SYNC_OPCODE     = 4

# MessagesInFile structure
message_opcode = 0
message_length = 1
message_data   = 2

def compare_arrays(a1,a2):
  if not np.array_equal(a1,a2):
    if(a1.shape != a2.shape):
      print("    FAILED: array shapes do not match. ", a1.shape, " vs. ", a2.shape)
      sys.exit(1)
    bad_idxs = np.where(a1 != a2)
    bad_idx = bad_idxs[0][0]
    print("    FAILED: arrays do not match. First discrepancy is sample index =", bad_idx)
    print("    Expected value =", a1[bad_idx], " (type:", type(a1[bad_idx]),")")
    print("    Actual value =", a2[bad_idx], " (type:", type(a2[bad_idx]), ")")
    sys.exit(1)

# Get message data and metadata from uint32 array read out of a file 
# generated with messagesInFile=true
def getmsg(m):
    length = m[0]
    opcode = m[1]
    data   = None
    if length > 0:
        data = m[2:length/4+2]
    return (opcode, length, data)

# Write single message (data and metadata) to a file in messagesInFile=true format
def addmsg(f, opcode, data):
    f.write(struct.pack("II",4*len(data),opcode)) # Two unsigned 32-bit
    if len(data):
        f.write(data)

# Write samples message data and metadata to a file in messagesInFile=true format
# with a specified message size
def addsamples(f, data, num_cycles,samples_per_message):
    for i in range(num_cycles):
        a = 0
        while a < len(data):
            if len(data) - a < samples_per_message:
                addmsg(f, SAMPLES_OPCODE, data[a:len(data)])
            else:
                addmsg(f, SAMPLES_OPCODE, data[a:a+samples_per_message])
            a+=samples_per_message

# Get samples data from file generated with messagesInFile=true
def parse_samples_data_from_msgs_in_file(file_to_be_parsed):
  #Read data as uint32
  f = open(file_to_be_parsed, 'rb')
  data = np.fromfile(f, dtype=np.uint32, count=-1)
  f.close()
  #Parse messages
  index = 0
  msg_count = 0
  msg = []
  samples_data_array = []
  while index < len(data):
    msg.append(getmsg(data[index:]))
    #print(msg[msg_count])
    if msg[msg_count][message_data] is None:
      if(msg[msg_count][message_opcode]==SAMPLES_OPCODE):
        print("    FAIL - ZLM on samples opcode detected.")
        sys.exit(1)
      else:
        index = index + 2
    else:
        index = index + len(msg[msg_count][message_data]) + 2
    msg_count += 1
  for i in range(0,len(msg)):
    if(msg[i][message_opcode]==SAMPLES_OPCODE):
      samples_data_array.extend(msg[i][message_data])
  return np.array(samples_data_array, dtype=dt_iq_pair)

# Get non-samples messages (data and metadata) from file generated 
# with messagesInFile=true
def parse_non_samples_msgs_from_msgs_in_file(file_to_be_parsed):
  #Read data as uint32
  f = open(file_to_be_parsed, 'rb')
  data = np.fromfile(f, dtype=np.uint32, count=-1)
  f.close()
  #Parse messages
  index = 0
  msg_count = 0
  msg = []
  non_samples_msg_array = []
  while index < len(data):
    msg.append(getmsg(data[index:]))
    if msg[msg_count][message_data] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][message_data]) + 2
    msg_count += 1
  for i in range(0,len(msg)):
    if(msg[i][message_opcode]!=SAMPLES_OPCODE):
      non_samples_msg_array.append(msg[i])
  return non_samples_msg_array

def compare_msgs(msg1,msg2):
  if msg1[message_opcode] != msg2[message_opcode] or msg1[message_length] != msg2[message_length] or \
     not np.array_equal(msg1[message_data],msg2[message_data]):
    print("    FAIL - Messages do not match")
    print("    ",msg1)
    print("    ",msg2)
    sys.exit(1)

# Check interval division for worker which perform decimation 
def check_interval_division(in_int_msg,out_int_msg,divisor):
    #Convert to 64 bit number
    input_interval  = (in_int_msg[message_data][0] << 32) + in_int_msg[message_data][1]
    output_interval = (out_int_msg[message_data][0] << 32) + out_int_msg[message_data][1]
    if output_interval != input_interval // divisor:
      print("    FAIL - Division of interval message data not as expected")
      print("    Output: ", output_interval, " Input/R: ", input_interval // divisor)
      sys.exit(1)

def is_power2(num):
  return num != 0 and ((num & (num - 1)) == 0)
