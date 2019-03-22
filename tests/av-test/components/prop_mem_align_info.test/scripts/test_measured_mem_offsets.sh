#!/bin/bash
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

if [ -z "$1" ]; then
  printf first argument must specify input measurement file, exiting now
  exit 1
fi

MEASUREMENT_FILE=$1_stripped.out
EXPECTED_FILE=expected.out
# Remove any "OCPI" debug lines that may have been added if log levels are too high
grep -v 'OCPI(' $1 > $MEASUREMENT_FILE

if [ -f $EXPECTED_FILE ]; then
  rm -rf $EXPECTED_FILE
fi

#
# Each "test" is implemented as a group of prop_mem_align_info.rcc properties:
#   1. an alignment property of type ulonglong (which is 8 bytes long),
#      followed by
#   2. one or more properties of type char whose purpose is to provide
#      test-specific padding in order to observe alignment for that test,
#      followed by
#   3. one or more properties with a name:
#      <test_description>_prop<extra_info>.
# A memory offset for each <test_description>_prop<extra_info> property is
# calculated and printed to stdout during runtime, with the printouts in the
# format: offset_<test_description>_prop<extra_info>=<value>.

# Note that tests are performed for the following property types:
#   * bool
#   * char
#   * uchar
#   * short
#   * ushort
#   * long
#   * ulong
#   * longlong
#   * ulonglong
#   * structs of primitive types
#   * arrays of structs of primitive types
# and not:
#   * floats
#   * doubles
#   * structs with widest member being 4 or 8 bytes
#   * structs of structs (of ...)
#   * structs of arrays (of ...)
#   * structs of sequences (of ...)
#   * multidimensional arrays
#   * sequences
#   * multidimensional sequences
#   * sequence of arrays (see OpenCPI_Component_Development.pdf - Section
#     5.3.6)

alignmentpadnumbytes=8 # this accounts for the alignment property

# 1-byte wide types will be aligned by 1 byte offsets, so each test should increment offset by 1 byte
expected_offset_bool_test_prop1=$((alignmentpadnumbytes+1))
expected_offset_bool_test_prop2=$((alignmentpadnumbytes+2))
expected_offset_bool_test_prop3=$((alignmentpadnumbytes+3))
expected_offset_bool_test_prop4=$((alignmentpadnumbytes+4))
expected_offset_bool_test_prop5=$((alignmentpadnumbytes+5))
expected_offset_bool_test_prop6=$((alignmentpadnumbytes+6))
expected_offset_bool_test_prop7=$((alignmentpadnumbytes+7))
expected_offset_char_test_prop1=$((alignmentpadnumbytes+1))
expected_offset_char_test_prop2=$((alignmentpadnumbytes+2))
expected_offset_char_test_prop3=$((alignmentpadnumbytes+3))
expected_offset_char_test_prop4=$((alignmentpadnumbytes+4))
expected_offset_char_test_prop5=$((alignmentpadnumbytes+5))
expected_offset_char_test_prop6=$((alignmentpadnumbytes+6))
expected_offset_char_test_prop7=$((alignmentpadnumbytes+7))
expected_offset_uchar_test_prop1=$((alignmentpadnumbytes+1))
expected_offset_uchar_test_prop2=$((alignmentpadnumbytes+2))
expected_offset_uchar_test_prop3=$((alignmentpadnumbytes+3))
expected_offset_uchar_test_prop4=$((alignmentpadnumbytes+4))
expected_offset_uchar_test_prop5=$((alignmentpadnumbytes+5))
expected_offset_uchar_test_prop6=$((alignmentpadnumbytes+6))
expected_offset_uchar_test_prop7=$((alignmentpadnumbytes+7))

# 2-byte wide types will be aligned by 2 byte offsets, so each test have the same 2 byte offset for varying padding
expected_offset_short_test_0_prop=$((alignmentpadnumbytes+2))
expected_offset_short_test_1_prop=$((alignmentpadnumbytes+2))
expected_offset_ushort_test_0_prop=$((alignmentpadnumbytes+2))
expected_offset_ushort_test_1_prop=$((alignmentpadnumbytes+2))

# 4-byte wide types will be aligned by 4 byte offsets, so each test have the same 4 byte offset for varying padding
expected_offset_long_test_0_prop=$((alignmentpadnumbytes+4))
expected_offset_long_test_1_prop=$((alignmentpadnumbytes+4))
expected_offset_long_test_2_prop=$((alignmentpadnumbytes+4))
expected_offset_long_test_3_prop=$((alignmentpadnumbytes+4))
expected_offset_ulong_test_0_prop=$((alignmentpadnumbytes+4))
expected_offset_ulong_test_1_prop=$((alignmentpadnumbytes+4))
expected_offset_ulong_test_2_prop=$((alignmentpadnumbytes+4))
expected_offset_ulong_test_3_prop=$((alignmentpadnumbytes+4))

# 8-byte wide types will be aligned by 8 byte offsets, so each test have the same 8 byte offset for varying padding
expected_offset_longlong_test_0_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_1_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_2_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_3_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_4_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_5_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_6_prop=$((alignmentpadnumbytes+8))
expected_offset_longlong_test_7_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_0_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_1_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_2_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_3_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_4_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_5_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_6_prop=$((alignmentpadnumbytes+8))
expected_offset_ulonglong_test_7_prop=$((alignmentpadnumbytes+8))

# struct types with the widest member being 1 byte will be aligned by 1 byte offsets, so each test should decrememnt offset by 1 byte
expected_offset_struct_largest_1_test_0_prop=$((alignmentpadnumbytes+4))
expected_offset_struct_largest_1_test_1_prop=$((alignmentpadnumbytes+3))
expected_offset_struct_largest_1_test_2_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_1_test_3_prop=$((alignmentpadnumbytes+1))
expected_offset_struct_largest_1_test_4_prop=$((alignmentpadnumbytes+4))
expected_offset_struct_largest_1_test_5_prop=$((alignmentpadnumbytes+3))
expected_offset_struct_largest_1_test_6_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_1_test_7_prop=$((alignmentpadnumbytes+1))
expected_offset_struct_largest_1_test_8_prop=$((alignmentpadnumbytes+4))
expected_offset_struct_largest_1_test_9_prop=$((alignmentpadnumbytes+3))
expected_offset_struct_largest_1_test_10_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_1_test_11_prop=$((alignmentpadnumbytes+1))

# struct types with the widest member being 2 byte will be aligned by 2 byte offsets, so each test should have the same 2 byte offset for varying padding, we also test for trailing padding within the struct here (hence the prop_after + 7)
expected_offset_struct_largest_2_test_0_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_1_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_2_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_3_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_4_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_5_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_6_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_6_prop_after=$((alignmentpadnumbytes+7))
expected_offset_struct_largest_2_test_7_prop=$((alignmentpadnumbytes+2))
expected_offset_struct_largest_2_test_7_prop_after=$((alignmentpadnumbytes+7))

# here we test that arrays of structs have the expected struct width and trailing struct padding
expected_offset_struct_array_largest_2_test_0_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_1_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_2_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_3_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_4_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_5_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_6_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_7_prop0=$((alignmentpadnumbytes+2))
expected_offset_struct_array_largest_2_test_0_prop1=$((alignmentpadnumbytes+4))
expected_offset_struct_array_largest_2_test_1_prop1=$((alignmentpadnumbytes+4))
expected_offset_struct_array_largest_2_test_2_prop1=$((alignmentpadnumbytes+6))
expected_offset_struct_array_largest_2_test_3_prop1=$((alignmentpadnumbytes+6))
expected_offset_struct_array_largest_2_test_4_prop1=$((alignmentpadnumbytes+8))
expected_offset_struct_array_largest_2_test_5_prop1=$((alignmentpadnumbytes+8))
expected_offset_struct_array_largest_2_test_6_prop1=$((alignmentpadnumbytes+8))
expected_offset_struct_array_largest_2_test_7_prop1=$((alignmentpadnumbytes+8))

printf "offset_bool_test_prop1=" > $EXPECTED_FILE
printf $expected_offset_bool_test_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_bool_test_prop2=" >> $EXPECTED_FILE
printf $expected_offset_bool_test_prop2 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_bool_test_prop3=" >> $EXPECTED_FILE
printf $expected_offset_bool_test_prop3 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_bool_test_prop4=" >> $EXPECTED_FILE
printf $expected_offset_bool_test_prop4 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_bool_test_prop5=" >> $EXPECTED_FILE
printf $expected_offset_bool_test_prop5 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_bool_test_prop6=" >> $EXPECTED_FILE
printf $expected_offset_bool_test_prop6 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_bool_test_prop7=" >> $EXPECTED_FILE
printf $expected_offset_bool_test_prop7 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop1=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop2=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop2 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop3=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop3 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop4=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop4 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop5=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop5 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop6=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop6 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_char_test_prop7=" >> $EXPECTED_FILE
printf $expected_offset_char_test_prop7 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop1=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop2=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop2 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop3=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop3 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop4=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop4 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop5=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop5 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop6=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop6 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_uchar_test_prop7=" >> $EXPECTED_FILE
printf $expected_offset_uchar_test_prop7 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_short_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_short_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_short_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_short_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ushort_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_ushort_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ushort_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_ushort_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_long_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_long_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_long_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_long_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_long_test_2_prop=" >> $EXPECTED_FILE
printf $expected_offset_long_test_2_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_long_test_3_prop=" >> $EXPECTED_FILE
printf $expected_offset_long_test_3_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulong_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulong_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulong_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulong_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulong_test_2_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulong_test_2_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulong_test_3_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulong_test_3_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_2_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_2_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_3_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_3_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_4_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_4_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_5_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_5_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_6_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_6_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_longlong_test_7_prop=" >> $EXPECTED_FILE
printf $expected_offset_longlong_test_7_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_2_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_2_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_3_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_3_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_4_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_4_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_5_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_5_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_6_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_6_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_ulonglong_test_7_prop=" >> $EXPECTED_FILE
printf $expected_offset_ulonglong_test_7_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_2_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_2_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_3_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_3_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_4_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_4_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_5_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_5_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_6_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_6_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_7_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_7_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_8_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_8_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_9_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_9_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_10_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_10_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_1_test_11_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_1_test_11_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_0_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_0_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_1_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_1_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_2_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_2_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_3_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_3_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_4_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_4_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_5_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_5_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_6_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_6_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_6_prop_after=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_6_prop_after >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_7_prop=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_7_prop >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_largest_2_test_7_prop_after=" >> $EXPECTED_FILE
printf $expected_offset_struct_largest_2_test_7_prop_after >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_0_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_0_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_1_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_1_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_2_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_2_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_3_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_3_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_4_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_4_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_5_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_5_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_6_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_6_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_7_prop0=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_7_prop0 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_0_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_0_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_1_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_1_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_2_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_2_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_3_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_3_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_4_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_4_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_5_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_5_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_6_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_6_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE
printf "offset_struct_array_largest_2_test_7_prop1=" >> $EXPECTED_FILE
printf $expected_offset_struct_array_largest_2_test_7_prop1 >> $EXPECTED_FILE
echo >> $EXPECTED_FILE

#printf "MEASURED:                                                    EXPECTED:"
#echo
#diff -y $MEASUREMENT_FILE $EXPECTED_FILE # busybox doesn't support -y
diff $MEASUREMENT_FILE $EXPECTED_FILE
DIFF_EXIT=$?
echo "--------------------------------------------------------------------------------"
if [ "$DIFF_EXIT" == "0" ]; then
  echo PASSED
else
  echo FAILED
fi
echo "--------------------------------------------------------------------------------"

exit $DIFF_EXIT
