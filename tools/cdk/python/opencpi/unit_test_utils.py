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
Unit Test Utils

Commonly used functions and definitions used in the generation and verification
unit test scripts
"""
import sys
import numpy as np
import struct

# Declare complex data type
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))

def is_power2(num):
    """
    This function returns 1 if num is a power of 2 and 0 if not

    >>> is_power2(64)
    True
    >>> is_power2(65)
    False
    """
    return num != 0 and ((num & (num - 1)) == 0)

def compare_arrays(a1,a2):
    """
    Compare two numpy arrays and provide helpful output if they are not the same

    >>> compare_arrays(np.array([1,2,3,4]),np.array([1,2,3,4]))
    >>> compare_arrays(np.array([1,2,3,4]),np.array([1,2,3,4,5]))
    Traceback (most recent call last):
    AssertionError: Array shapes do not match. (4,) vs. (5,)
    >>> compare_arrays(np.array([1,2,3,4]),np.array([1,2,3,5]))
    Traceback (most recent call last):
    AssertionError: Arrays do not match. First discrepancy is index = 3
    """
    assert(a1.shape == a2.shape),( 
           "Array shapes do not match. {} vs. {}".format(a1.shape,a2.shape))
    assert(np.array_equal(a1,a2)),( 
           "Arrays do not match. First discrepancy is index = {}".format(np.where(a1!=a2)[0][0]))
    
# MessagesInFile structure
MESSAGE_LENGTH = 0
MESSAGE_OPCODE = 1
MESSAGE_DATA   = 2

def get_msg(msgs_in_file_array):
    """
    Get data and metadata from first message of uint32 array (msgs_in_file_array) 
    read out of a file generated with messagesInFile=true.
    This function assumes that data granularity is 32 bits

    >>> get_msg([16, 0, 1, 2, 3, 4])
    (16, 0, [1, 2, 3, 4])
    """
    length = msgs_in_file_array[MESSAGE_LENGTH]
    opcode = msgs_in_file_array[MESSAGE_OPCODE]
    data   = None
    if length > 0:
        data = msgs_in_file_array[MESSAGE_DATA:length/4+2]
    return (length,opcode,data)

def add_msg(f, opcode, data):
    """
    Write single message (data and metadata) to a file in messagesInFile=true format
    """
    f.write(struct.pack("II",4*len(data),opcode)) # Two unsigned 32-bit
    if len(data):
        f.write(data)

def compare_msgs(msg1,msg2):
    """
    Compare two messages (data and metadata) which originated from messagesInFile=true file
    and have the format (length,opcode,data)

    >>> compare_msgs((16, 0, np.array([1, 2, 3, 4])),(16, 0, np.array([1, 2, 3, 4])))
    >>> compare_msgs((16, 0, np.array([1, 2, 3, 4])),(17, 0, np.array([1, 2, 3, 4])))
    Traceback (most recent call last):
    AssertionError: Message lengths do not match
    >>> compare_msgs((16, 0, np.array([1, 2, 3, 4])),(16, 1, np.array([1, 2, 3, 4])))
    Traceback (most recent call last):
    AssertionError: Message opcodes do not match
    >>> compare_msgs((16, 0, None),(16, 0, None))
    >>> compare_msgs((16, 0, None),(16, 0, np.array([1, 2, 3, 4])))
    Traceback (most recent call last):
    AssertionError: Message data does not match
    """
    assert(msg1[MESSAGE_LENGTH] == msg2[MESSAGE_LENGTH]), "Message lengths do not match"
    assert(msg1[MESSAGE_OPCODE] == msg2[MESSAGE_OPCODE]), "Message opcodes do not match"
    #np.array_equal (and not compare_arrays) is used here because MESSAGE_DATA may be ZLMs
    assert(np.array_equal(msg1[MESSAGE_DATA],msg2[MESSAGE_DATA])), "Message data does not match"
