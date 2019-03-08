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


"""
This script generates files for the input port for the wsi_width_adapter unit tests

Generate args:
1. Starting message size of input data
2. Ending message size of input data
3. Message size increment of input data
4. Input file name

This script creates a set of input data for the UUT. The input data consists of 8192 messages of length 1-8192. All data is opcode 0. 
"""
import struct
import numpy
import sys
import os.path

if len(sys.argv) != 5 and len(sys.argv) > 1:
    print("Invalid arguments:  usage is: generate.py <starting-message-size> <ending-message-size> <message-size-increment> <output-file>")
    sys.exit(1)
if  len(sys.argv) == 5:
    startingMessageSize=int(sys.argv[1])
    endingMessageSize=int(sys.argv[2])
    messageSizeIncrement=int(sys.argv[3])
    fileName = sys.argv[4]
else: #sensible defaults if no arguments are given
    print "Using default input parameters:"
    print "Starting Message Size: 2"
    print "Ending message size: 8192"
    print "Message size increment: 315"
    print "Output file: test.input.width_adapter"
    startingMessageSize=2
    endingMessageSize=8192
    messageSizeIncrement=315
    fileName = "test.input.width_adapter_test"    
fileType = 'uint8'
packType = 'B'
metadataHeaderSizeInBytes=8
numCases=(endingMessageSize-startingMessageSize)/messageSizeIncrement+1
if numCases <= 0 or (endingMessageSize-startingMessageSize)%messageSizeIncrement != 0 :
    print("Error: (endingMessageSize-startingMessageSize)/messageSizeIncrement must be a postive integer")
    sys.exit(1)
totalMetadataHeaderBytes=metadataHeaderSizeInBytes*numCases
totalDataBytes=sum(range(startingMessageSize,endingMessageSize+1,messageSizeIncrement))
numBytesInOutputFile=totalMetadataHeaderBytes+totalDataBytes+metadataHeaderSizeInBytes #Total data size plus test ZLM
#Create an empty array for the output data
outData = numpy.array(numpy.zeros(numBytesInOutputFile), dtype=fileType)
messageSizeIndex=startingMessageSize
idx=0
#Insert ZLM with opcode 1 to test AV-3201
outData[idx:idx+3] = 0
outData[idx+4:idx+5] = 1
outData[idx+6:idx+7] = 0
idx+=metadataHeaderSizeInBytes
while messageSizeIndex <= endingMessageSize:
    outData[idx] = messageSizeIndex
    outData[idx+1] = messageSizeIndex >> 8
    outData[idx+2:idx+3] = 0
    outData[idx+4:idx+5] = idx % 255
    outData[idx+6:idx+7] = 0
    outData[idx+8:idx+8+messageSizeIndex] = range(0,messageSizeIndex)
    idx+=messageSizeIndex+metadataHeaderSizeInBytes
    messageSizeIndex+=messageSizeIncrement
#Convert array to list
outDataList = outData.tolist()
#Save data file
f = open(fileName, 'wb')
for i in xrange(0,numBytesInOutputFile):
    f.write(struct.pack(packType, outDataList[i]))
f.close()
