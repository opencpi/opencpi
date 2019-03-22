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
FIR Complex: Validate output data for Symmetric FIR Complex (NUM_TAPS_p taps)
using the ComplexShortWithMetadata protocol

Validation args:
- output data filename
- input data file

To test the FIR Complex filter, a binary data file is generated containing all of the
opcodes of the ComplexShortWithMetadata protocol in the following sequence:
1. Interval
2. Sync (this opcode is expected after an Interval opcode)
3. Time
4. Samples (impulse with length numtaps*2)
4. Samples (impulse with length numtaps*2)
5. Flush
6. Samples (impulse with length numtaps*2)
7. Sync
8. Samples (impulse with length numtaps*2)

The worker will pass through the interval and time opcodes. The samples opcode followed
by flush or done will output an impulse response, showing the symmetric tap values. The
samples opcode followed by sync will produce the first numtaps*2-group_delay tap values.
In addition to the samples data, the worker also passes along the zlms.
"""
import numpy as np
import sys
import os.path

def getmsg(m):
    length = m[0]
    opcode = m[1]
    data   = None
    if length > 0:
        data = m[2:length/4+2].tolist()
    return [opcode, length, data]

if len(sys.argv) < 2:
    print("Usage expected: output filename, input filename\n")
    sys.exit(1)

#Generate text file of symmetric tap values for comparison
group_delay=int(os.environ.get("OCPI_TEST_GROUP_DELAY_p"))
num_taps=int(os.environ.get("OCPI_TEST_NUM_TAPS_p"))
taps=list(map(int, os.environ.get("OCPI_TEST_taps").split(","))) #tap values are comma-separated

output_file=sys.argv[1]
input_file=sys.argv[2]

#I/Q pair in a 32-bit vector (31:0) is Q(0) Q(1) I(0) I(1) in bytes 0123 little-Endian
#Thus Q is indexed at byte 0 and I is indexed at byte 2
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,2), 'imag_idx':(np.int16,0)}))

#Check to confirm number of taps is as expected
if num_taps != 2*len(taps):
    print('  FAILED: Actual number of taps does not match specified NUM_TAPS_p')
    sys.exit(1)

#Check to make sure not all taps are zero
if( sum(map(abs,taps)) == 0):
    print('  FAILED: taps are all zero')
    sys.exit(1)

#Read output data file as uint32
ofile = open(output_file, 'rb')
dout = np.fromfile(ofile, dtype=np.uint32, count=-1)
ofile.close()

#Read output data file as uint32
ifile = open(input_file, 'rb')
din = np.fromfile(ifile, dtype=np.uint32, count=-1)
ifile.close()

#Ensure output data is not all zeros
if all(dout == 0):
    print('  FAILED: values are all zero')
    sys.exit(1)

#Parse messages within output data
index = 0
msg_count = 0
msg = []
samples = []
dout_non_sample_msg = []
while index < len(dout):
    msg.append(getmsg(dout[index:]))
    if msg[msg_count][2] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][2]) + 2
    msg_count += 1
for i in range(0,len(msg)):
    if(msg[i][0]==0):
        samples.append(msg[i][2])
    else:
        dout_non_sample_msg.append(msg[i])

#Parse messages within input data
index = 0
msg_count = 0
msg = []
din_non_sample_msg = []
while index < len(din):
    msg.append(getmsg(din[index:]))
    if msg[msg_count][2] is None:
        index = index + 2
    else:
        index = index + len(msg[msg_count][2]) + 2
    msg_count += 1
for i in range(0,len(msg)):
    if(msg[i][0]!=0):
        din_non_sample_msg.append(msg[i])

#All non-sample data should match exactly
if dout_non_sample_msg != din_non_sample_msg:
    print(din_non_sample_msg)
    print(dout_non_sample_msg)
    print('  FAILED: Non-sample data does not match between input and output files')
    sys.exit(1)

#Compare symmetric taps file to output
first_message_size=num_taps-(group_delay-1)
flush_message_size=group_delay-1
tolerance=1
taps.extend(reversed(taps)); #extend the taps to be symmetric
for a in samples:
    isamples = ([b & 0xffff for b in a])
    qsamples = ([b >> 16 & 0xffff for b in a])
    #3 cases
    #1 initial or post-flush messages are length numtaps-(group delay-1)
    if len(a) == first_message_size:
        #Check if tap values are within 1
        if not (np.isclose(isamples,taps[0:first_message_size],atol=tolerance).all() and 
                np.isclose(qsamples,taps[0:first_message_size],atol=tolerance).all()):
            print ('  FAILED: Initial or post flush sample data does not match expected result')
            sys.exit(1)
    #2 in between messages are length numtaps
    elif len(a) == num_taps:
        #Rotate taps by group delay and check if tap values are within 1 
        if not (np.isclose(isamples,np.roll(taps,group_delay-1),atol=tolerance).all() and 
                np.isclose(qsamples,np.roll(taps,group_delay-1),atol=tolerance).all()):
            print ('  FAILED: Sample data does not match expected result')
            sys.exit(1)
    #3 flush message is length group delay-1
    elif len(a) == flush_message_size:
        if not (np.isclose(isamples,taps[first_message_size:first_message_size+flush_message_size],atol=tolerance).all() and
                np.isclose(qsamples,taps[first_message_size:first_message_size+flush_message_size],atol=tolerance).all()):
            print ('  FAILED: Flush sample data does not match expected result')
            sys.exit(1)
    else:
        print('  FAILED: Unexpected message length from samples opcode: ' + str(len(a)))
        sys.exit(1)
