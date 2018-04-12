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

import struct
import numpy as np
import sys
import os.path

def get_bit(idx,val):
  return (val & pow(2,idx)) >> idx

def ocpi_prop_bool_array_string_to_bit_list(prop_bool_array_string,
                                            prop_array_length):
  l = prop_bool_array_string.split(",")
  # expand list to account for opencpi compacting all last repeating values
  while len(l) != prop_array_length:
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

ocpirun_prop_val_DATA_BIT_WIDTH_p  = int(os.environ.get("OCPI_TEST_DATA_BIT_WIDTH_p"))
ocpirun_prop_val_LFSR_POLYNOMIAL_p = os.environ.get("OCPI_TEST_LFSR_POLYNOMIAL_p")
ocpirun_prop_val_LFSR_SEED_p      = os.environ.get("OCPI_TEST_LFSR_SEED_p")
ocpirun_prop_val_num_samples      = int(os.environ.get("OCPI_TEST_num_samples"))
ocpirun_prop_val_mode             = os.environ.get("OCPI_TEST_mode")
ocpirun_prop_val_enable           = os.environ.get("OCPI_TEST_enable")
ocpirun_prop_val_fixed_value      = os.environ.get("OCPI_TEST_fixed_value")
ocpirun_prop_val_LFSR_bit_reverse = os.environ.get("OCPI_TEST_LFSR_bit_reverse")

LFSR_seed_list = ocpi_prop_bool_array_string_to_bit_list(
    ocpirun_prop_val_LFSR_SEED_p,
    ocpirun_prop_val_DATA_BIT_WIDTH_p)
if all(bit == 0 for bit in LFSR_seed_list):
  print "    failed: LFSR_SEED_p parameter value contains all zeros"
  sys.exit(1)
else:
  print "    pass: LFSR_SEED_p parameter value does not contain all zeros"

#Open output file and grab samples as int32 
OFILENAME = open(sys.argv[1], 'rb')
dt_iq_pair = np.dtype((np.uint32, {'imag_idx':(np.int16,2), 'real_idx':(np.int16,0)}))
odata = np.fromfile(OFILENAME, dtype=dt_iq_pair, count=-1)
OFILENAME.close()

# Test that odata file length matches expected number of samples
num_samples_saved_out = len(odata)
if ocpirun_prop_val_enable == "false":
  if num_samples_saved_out == 0:
    print "    pass: output file length = 0 for 'enable' property value = false"
  else:
    print "    failed: output file length =", num_samples_saved_out, \
          " samples, while expected length is 0 samples for 'enable'" \
          " property value = false"
    sys.exit(1)
else: # string(ocpirun_prop_val_enable) = "true"
  if ocpirun_prop_val_num_samples == -1:
    if num_samples_saved_out >= 1:
      print "    pass: output file length >= 1 samples for 'num_samples'" \
            " property value = -1"
    else:
      print "    failed: output file length =", num_samples_saved_out, \
            " samples, while expected length is >=1 for 'num_samples'" \
            " property value = -1"
      sys.exit(1)
  else:
    if num_samples_saved_out == ocpirun_prop_val_num_samples:
      print "    pass: output file length = input file length"
    else:
      print "    failed: output file length =", num_samples_saved_out, \
            "samples, while expected length =", ocpirun_prop_val_num_samples, \
            "samples for 'num_samples' property value =", \
            ocpirun_prop_val_num_samples
      sys.exit(1)

#Test odata file binary contents are as expected
if num_samples_saved_out < 1:
    print "    skipping file binary contents check since num samples saved out" \
          " was 0"
else:
  I_width=Q_width=16 # from iqstream_protocol 'Short'
  fail=0
  val_count = 0
  val_walking = pow(2,ocpirun_prop_val_DATA_BIT_WIDTH_p-1)

  val_LFSR_list = LFSR_seed_list
  LFSR_polynomial_list = ocpi_prop_bool_array_string_to_bit_list(
      ocpirun_prop_val_LFSR_POLYNOMIAL_p,
      ocpirun_prop_val_DATA_BIT_WIDTH_p)

  val_binary_list = list()
  for idx_samp in range(0,len(odata)):
    samp_I = odata['real_idx'][idx_samp]
    samp_Q = odata['imag_idx'][idx_samp]
    if ocpirun_prop_val_mode == "count":
      val_binary_list = [get_bit(ocpirun_prop_val_DATA_BIT_WIDTH_p-1,
                                 val_count)]
      for idx in range(1,ocpirun_prop_val_DATA_BIT_WIDTH_p):
        val_binary_list.append(get_bit(ocpirun_prop_val_DATA_BIT_WIDTH_p-1-idx,
                                       val_count))
      if val_count == pow(2,ocpirun_prop_val_DATA_BIT_WIDTH_p)-1:
        val_count = 0
      else:
        val_count += 1
    elif ocpirun_prop_val_mode == "walking":
      val_binary_list = [get_bit(ocpirun_prop_val_DATA_BIT_WIDTH_p-1,
                                 val_walking)]
      for idx in range(1,ocpirun_prop_val_DATA_BIT_WIDTH_p):
        val_binary_list.append(get_bit(ocpirun_prop_val_DATA_BIT_WIDTH_p-1-idx,
                                       val_walking))
      if val_walking == 1:
        val_walking = pow(2,ocpirun_prop_val_DATA_BIT_WIDTH_p-1)
      else:
        val_walking >>= 1
    elif ocpirun_prop_val_mode == "LFSR":
      if ocpirun_prop_val_LFSR_bit_reverse == 'true':
        val_binary_list = [ val_LFSR_list[(len(val_LFSR_list)-1)] ]
        for idx in range(0,len(val_LFSR_list)-1):
          val_binary_list.append(val_LFSR_list[len(val_LFSR_list)-2-idx])
      else:
        val_binary_list = list(val_LFSR_list)

      feedback = 0
      old_val = list(val_LFSR_list)
      if(len(old_val) != len(LFSR_polynomial_list)):
        exit(1)
      if(len(old_val) != len(val_LFSR_list)):
        exit(1)
      for idx in range(0,len(old_val)):
        if idx >= 1:
          val_LFSR_list[idx] = old_val[idx-1]
        if LFSR_polynomial_list[len(LFSR_polynomial_list)-1-idx] == 1:
          feedback = feedback ^ old_val[idx]
      val_LFSR_list[0] = feedback
    else: # ocpirun_prop_val_mode == "fixed":
      val_binary_list = ocpi_prop_bool_array_string_to_bit_list(
          ocpirun_prop_val_fixed_value, ocpirun_prop_val_DATA_BIT_WIDTH_p)

    # convert val_binary_list to expected I (using two's complement)
    val_I = 0
    for idx in range(1,len(val_binary_list)):
      if val_binary_list[idx] == 1:
        val_I = val_I | (1 << (ocpirun_prop_val_DATA_BIT_WIDTH_p-1 - idx))
    expected_val_I = val_I << \
                      (I_width - ocpirun_prop_val_DATA_BIT_WIDTH_p)
    if val_binary_list[0] == 1:
      expected_val_I -= pow(2,I_width-1)

    # convert val_binary_list to expected Q (using two's complement)
    val_Q = 0
    for idx in range(1,len(val_binary_list)):
      if val_binary_list[ocpirun_prop_val_DATA_BIT_WIDTH_p-1-idx] == 1:
        val_Q = val_Q | (1 << (ocpirun_prop_val_DATA_BIT_WIDTH_p-1 - idx))
    expected_val_Q = val_Q << \
                      (Q_width - ocpirun_prop_val_DATA_BIT_WIDTH_p)
    if val_binary_list[ocpirun_prop_val_DATA_BIT_WIDTH_p-1] == 1:
      expected_val_Q -= pow(2,Q_width-1)

    # do value comparison
    if (samp_I != expected_val_I) or \
       (samp_Q != expected_val_Q):
      fail=1
      if ocpirun_prop_val_mode == "fixed":
        print "    FAILED: output file did not match expected value for" \
              " 'mode' property value =",ocpirun_prop_val_mode , \
              " 'fixed_value' property value =",ocpirun_prop_val_fixed_value
      else:
        print "    FAILED: output file did not match expected fixed value for" \
              " 'mode' property value =",ocpirun_prop_val_mode
      break

  if fail == 0:
    print "    PASS: output file content matches expected content"
  else:
    sys.exit(1)
      
