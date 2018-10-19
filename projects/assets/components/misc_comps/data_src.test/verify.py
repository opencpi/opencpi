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

import struct
import numpy as np
import sys
import os.path
import math

# this exists for optimization purposes, gen_bit_reversal_table.py originally
# generated this file
import bit_reverse_table

# this exists for optimization purposes, gen_lfsr_tables.py originally
# generated this file
import lfsr_tables

DATA_BIT_WIDTH_p = int(os.environ.get("OCPI_TEST_DATA_BIT_WIDTH_p"))
num_samples      = int(os.environ.get("OCPI_TEST_num_samples"))
mode             = os.environ.get("OCPI_TEST_mode")
enable           = os.environ.get("OCPI_TEST_enable")
LFSR_bit_reverse = os.environ.get("OCPI_TEST_LFSR_bit_reverse")
fixed_value      = os.environ.get("OCPI_TEST_fixed_value")

if DATA_BIT_WIDTH_p > 16:
    print("    ERROR: test could not complete, received invalid ",
          "DATA_BIT_WIDTH_p of",DATA_BIT_WIDTH_p,
          ">16 not supported by verify script")
    sys.exit(100)

if (LFSR_bit_reverse != 'true') and (LFSR_bit_reverse != 'false'):
    print("    ERROR: invalid LFSR_bit_reverse property value of",
          LFSR_bit_reverse, " received, expected true or false")
    sys.exit(100)

OFILENAME = open(sys.argv[1], 'rb')
dt_iq_pair = np.dtype((np.uint32, {'real_idx':(np.int16,0), 'imag_idx':(np.int16,2)}))
odata = np.fromfile(OFILENAME, dtype=dt_iq_pair, count=-1)
OFILENAME.close()

def comma_sep_bool_string_to_bit_list(bool_array_string,
                                      array_length):
    l = bool_array_string.split(",")
    # expand list to account for opencpi compacting all last repeating values
    while len(l) != array_length:
        l.append(l[len(l)-1])
    if(l[0] == 'true'):
        bit_list = [1]
    else:
        bit_list = [0]
    for idx in range(1,len(l)):
        if(l[idx] == 'true'):
            bit_list.append(1)
        else:
            bit_list.append(0)
    return list(bit_list)

def test_odata_file_length_zero_when_enable_prop_false():
    if enable == "false":
        if len(odata) == 0:
            print("    PASS: output file length = 0 32-bit words for 'enable'",
                  "property = false")
        else:
            print("    FAIL: output file length =", len(odata), "32-bit"
                  "words, while expected length is 0 32-bit words for",
                  "'enable' property = false")
            sys.exit(1)

def test_odata_file_length_matches_expected_num_samples():
    if enable == "true":
        if num_samples == -1:
            if len(odata) >= 1:
                print("    PASS: output file length >= 1 samples for 'num_samples'" \
                      " property value = -1")
            else:
                print("    FAIL: output file length =", len(odata), \
                      " samples, while expected length is >=1 for 'num_samples'" \
                      " property value = -1")
                sys.exit(1)
        else:
            if len(odata) == num_samples:
                print("    PASS: output file length = input file length")
            else:
                print("    FAIL: output file length =", len(odata), \
                      "samples, while expected length =", num_samples, \
                      "samples for 'num_samples' property value =", \
                      num_samples)
                sys.exit(1)

def test_odata_file_binary_contents_are_as_expected():
    if num_samples == 0:
        print("    skipping file binary contents check since num samples saved " \
              " out was 0")
    else:
        # generate expected DATA_BIT_WIDTH_p bit wide data
        if mode == "count":
            # counter is a DATA_BIT_WIDTH_p bit wide unsigned counter (see
            # component data sheet)
            data = np.remainder(np.arange(len(odata)), 2**DATA_BIT_WIDTH_p)
        elif mode == "walking":
            # e.g. b'100 -> b'010 -> b'001 -> b'100 ->etc  (see
            # component data sheet)
            tmp = np.arange(DATA_BIT_WIDTH_p-1,DATA_BIT_WIDTH_p-len(odata)-1,-1)
            data = 2**np.remainder(tmp, DATA_BIT_WIDTH_p)
        elif mode == "LFSR":
            if LFSR_bit_reverse == 'true':
                tmp = lfsr_tables.lfsr_table_rev
                tmp *= math.ceil(len(odata)/len(lfsr_tables.lfsr_table_rev))
            else: # LFSR_bit_reverse == 'false'
                tmp = lfsr_tables.lfsr_table
                tmp *= math.ceil(len(odata)/len(lfsr_tables.lfsr_table))
            data_lfsr = tmp[:len(odata)]
            data = np.array(data_lfsr)
        elif mode == "fixed":
            val_bit_list = comma_sep_bool_string_to_bit_list(fixed_value,
                    DATA_BIT_WIDTH_p)
            val = 0
            for idx in range(0, len(val_bit_list)):
                val |= val_bit_list[idx] << (DATA_BIT_WIDTH_p-1-idx)

            data = (np.ones(len(odata)) * val).astype(int)
        else:
            print("    ERROR: test could not complete, received unexpected",
                  " mode of ",mode)
            sys.exit(100)

        # from the component data sheet: "Data_Src selects one DATA_BIT_WIDTH_p
        # bits-wide data bus from multiple data generation sources, packs the
        # DATA_BIT_WIDTH_p bits in bit-forward order in the least significant bits of
        # the I data bus and bit-reverse order in the most significant bits of the Q
        # data bus"
        data_I = data
        data_Q = np.zeros(len(odata))
        for ii in range(0, len(odata)):
            data_Q[ii] = bit_reverse_table.bit_reverse_table[data[ii]]
        data_I_signed = data_I
        data_I_signed[np.where(data_I_signed > 2**(DATA_BIT_WIDTH_p-1)-1)] -= \
            2**DATA_BIT_WIDTH_p
        data_Q_signed = data_Q
        data_Q_signed[np.where(data_Q_signed > 2**(DATA_BIT_WIDTH_p-1)-1)] -= \
            2**DATA_BIT_WIDTH_p
        I_expected = data_I_signed * 2**(16-DATA_BIT_WIDTH_p)
        Q_expected = data_Q_signed * 2**(16-DATA_BIT_WIDTH_p)

        if not np.array_equal(I_expected, odata['real_idx']):
            bad_idxs = np.where(I_expected != odata['real_idx'])
            bad_idx = bad_idxs[0][0]
            print("    FAILED: output file did not match expected I values for",
                  " 'mode' property value =", mode, ", sample index =", bad_idx,
                  ", expected I value =", I_expected[bad_idx],
                  ", actual I value =", odata['real_idx'][bad_idx])
            sys.exit(1)

        if not np.array_equal(Q_expected, odata['imag_idx']):
            bad_idxs = np.where(Q_expected != odata['imag_idx'])
            bad_idx = bad_idxs[0][0]
            print("    FAILED: output file did not match expected Q values for",
                  " 'mode' property value =", mode, ", sample index =", bad_idx,
                  ", expected Q value =", Q_expected[bad_idx],
                  ", actual Q value =", odata['imag_idx'][bad_idx])
            sys.exit(1)
    print("    PASS: output file content matches expected content")

def main():
    test_odata_file_length_zero_when_enable_prop_false()
    test_odata_file_length_matches_expected_num_samples()
    if len(odata) > 0:
        test_odata_file_binary_contents_are_as_expected()

if __name__ == "__main__":
    main()
