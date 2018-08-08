/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Aug 23 15:32:36 2017 EDT
 * BASED ON THE FILE: prop_mem_align_info.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the prop_mem_align_info worker in C++
 */

#include <iostream> // std::cout
#include "prop_mem_align_info-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Prop_mem_align_infoWorkerTypes;

#define PRINT_VAR(x) std::cout << #x"=" << x << std::endl;
#define PRINT_VAR_HEX(x) std::cout << #x"=0x" << std::hex << x << std::endl;

class Prop_mem_align_infoWorker : public Prop_mem_align_infoWorkerBase {

  uint64_t get_prop_mem_offset(uint8_t* pprop0, uint8_t* pprop1)
  {
    const uint64_t offset = (uint64_t) (pprop0 - pprop1);
    return offset;
  }


  RCCResult run(bool /*timedout*/) {
    const uint64_t offset_bool_test_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop1),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop1);
    const uint64_t offset_bool_test_prop2 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop2),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop2);
    const uint64_t offset_bool_test_prop3 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop3),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop3);
    const uint64_t offset_bool_test_prop4 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop4),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop4);
    const uint64_t offset_bool_test_prop5 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop5),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop5);
    const uint64_t offset_bool_test_prop6 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop6),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop6);
    const uint64_t offset_bool_test_prop7 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.bool_test_prop7),
                            (uint8_t *)&(m_properties.bool_test_alignment_pad));
    PRINT_VAR(offset_bool_test_prop7);
    const uint64_t offset_char_test_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop1),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop1);
    const uint64_t offset_char_test_prop2 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop2),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop2);
    const uint64_t offset_char_test_prop3 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop3),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop3);
    const uint64_t offset_char_test_prop4 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop4),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop4);
    const uint64_t offset_char_test_prop5 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop5),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop5);
    const uint64_t offset_char_test_prop6 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop6),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop6);
    const uint64_t offset_char_test_prop7 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.char_test_prop7),
                            (uint8_t *)&(m_properties.char_test_alignment_pad));
    PRINT_VAR(offset_char_test_prop7);
    const uint64_t offset_uchar_test_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop1),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop1);
    const uint64_t offset_uchar_test_prop2 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop2),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop2);
    const uint64_t offset_uchar_test_prop3 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop3),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop3);
    const uint64_t offset_uchar_test_prop4 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop4),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop4);
    const uint64_t offset_uchar_test_prop5 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop5),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop5);
    const uint64_t offset_uchar_test_prop6 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop6),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop6);
    const uint64_t offset_uchar_test_prop7 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.uchar_test_prop7),
                            (uint8_t *)&(m_properties.uchar_test_alignment_pad));
    PRINT_VAR(offset_uchar_test_prop7);
    const uint64_t offset_short_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.short_test_0_prop),
                            (uint8_t *)&(m_properties.short_test_0_alignment_pad));
    PRINT_VAR(offset_short_test_0_prop);
    const uint64_t offset_short_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.short_test_1_prop),
                            (uint8_t *)&(m_properties.short_test_1_alignment_pad));
    PRINT_VAR(offset_short_test_1_prop);
    const uint64_t offset_ushort_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ushort_test_0_prop),
                            (uint8_t *)&(m_properties.ushort_test_0_alignment_pad));
    PRINT_VAR(offset_ushort_test_0_prop);
    const uint64_t offset_ushort_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ushort_test_1_prop),
                            (uint8_t *)&(m_properties.ushort_test_1_alignment_pad));
    PRINT_VAR(offset_ushort_test_1_prop);
    const uint64_t offset_long_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.long_test_0_prop),
                            (uint8_t *)&(m_properties.long_test_0_alignment_pad));
    PRINT_VAR(offset_long_test_0_prop);
    const uint64_t offset_long_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.long_test_1_prop),
                            (uint8_t *)&(m_properties.long_test_1_alignment_pad));
    PRINT_VAR(offset_long_test_1_prop);
    const uint64_t offset_long_test_2_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.long_test_2_prop),
                            (uint8_t *)&(m_properties.long_test_2_alignment_pad));
    PRINT_VAR(offset_long_test_2_prop);
    const uint64_t offset_long_test_3_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.long_test_3_prop),
                            (uint8_t *)&(m_properties.long_test_3_alignment_pad));
    PRINT_VAR(offset_long_test_3_prop);
    const uint64_t offset_ulong_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulong_test_0_prop),
                            (uint8_t *)&(m_properties.ulong_test_0_alignment_pad));
    PRINT_VAR(offset_ulong_test_0_prop);
    const uint64_t offset_ulong_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulong_test_1_prop),
                            (uint8_t *)&(m_properties.ulong_test_1_alignment_pad));
    PRINT_VAR(offset_ulong_test_1_prop);
    const uint64_t offset_ulong_test_2_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulong_test_2_prop),
                            (uint8_t *)&(m_properties.ulong_test_2_alignment_pad));
    PRINT_VAR(offset_ulong_test_2_prop);
    const uint64_t offset_ulong_test_3_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulong_test_3_prop),
                            (uint8_t *)&(m_properties.ulong_test_3_alignment_pad));
    PRINT_VAR(offset_ulong_test_3_prop);
    const uint64_t offset_longlong_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_0_prop),
                            (uint8_t *)&(m_properties.longlong_test_0_alignment_pad));
    PRINT_VAR(offset_longlong_test_0_prop);
    const uint64_t offset_longlong_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_1_prop),
                            (uint8_t *)&(m_properties.longlong_test_1_alignment_pad));
    PRINT_VAR(offset_longlong_test_1_prop);
    const uint64_t offset_longlong_test_2_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_2_prop),
                            (uint8_t *)&(m_properties.longlong_test_2_alignment_pad));
    PRINT_VAR(offset_longlong_test_2_prop);
    const uint64_t offset_longlong_test_3_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_3_prop),
                            (uint8_t *)&(m_properties.longlong_test_3_alignment_pad));
    PRINT_VAR(offset_longlong_test_3_prop);
    const uint64_t offset_longlong_test_4_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_4_prop),
                            (uint8_t *)&(m_properties.longlong_test_4_alignment_pad));
    PRINT_VAR(offset_longlong_test_4_prop);
    const uint64_t offset_longlong_test_5_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_5_prop),
                            (uint8_t *)&(m_properties.longlong_test_5_alignment_pad));
    PRINT_VAR(offset_longlong_test_5_prop);
    const uint64_t offset_longlong_test_6_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_6_prop),
                            (uint8_t *)&(m_properties.longlong_test_6_alignment_pad));
    PRINT_VAR(offset_longlong_test_6_prop);
    const uint64_t offset_longlong_test_7_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.longlong_test_7_prop),
                            (uint8_t *)&(m_properties.longlong_test_7_alignment_pad));
    PRINT_VAR(offset_longlong_test_7_prop);
    const uint64_t offset_ulonglong_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_0_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_0_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_0_prop);
    const uint64_t offset_ulonglong_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_1_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_1_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_1_prop);
    const uint64_t offset_ulonglong_test_2_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_2_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_2_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_2_prop);
    const uint64_t offset_ulonglong_test_3_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_3_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_3_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_3_prop);
    const uint64_t offset_ulonglong_test_4_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_4_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_4_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_4_prop);
    const uint64_t offset_ulonglong_test_5_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_5_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_5_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_5_prop);
    const uint64_t offset_ulonglong_test_6_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_6_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_6_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_6_prop);
    const uint64_t offset_ulonglong_test_7_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.ulonglong_test_7_prop),
                            (uint8_t *)&(m_properties.ulonglong_test_7_alignment_pad));
    PRINT_VAR(offset_ulonglong_test_7_prop);
    const uint64_t offset_struct_largest_1_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_0_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_0_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_0_prop);
    const uint64_t offset_struct_largest_1_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_1_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_1_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_1_prop);
    const uint64_t offset_struct_largest_1_test_2_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_2_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_2_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_2_prop);
    const uint64_t offset_struct_largest_1_test_3_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_3_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_3_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_3_prop);
    const uint64_t offset_struct_largest_1_test_4_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_4_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_4_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_4_prop);
    const uint64_t offset_struct_largest_1_test_5_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_5_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_5_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_5_prop);
    const uint64_t offset_struct_largest_1_test_6_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_6_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_6_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_6_prop);
    const uint64_t offset_struct_largest_1_test_7_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_7_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_7_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_7_prop);
    const uint64_t offset_struct_largest_1_test_8_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_8_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_8_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_8_prop);
    const uint64_t offset_struct_largest_1_test_9_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_9_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_9_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_9_prop);
    const uint64_t offset_struct_largest_1_test_10_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_10_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_10_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_10_prop);
    const uint64_t offset_struct_largest_1_test_11_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_1_test_11_prop),
                            (uint8_t *)&(m_properties.struct_largest_1_test_11_alignment_pad));
    PRINT_VAR(offset_struct_largest_1_test_11_prop);
    const uint64_t offset_struct_largest_2_test_0_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_0_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_0_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_0_prop);
    const uint64_t offset_struct_largest_2_test_1_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_1_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_1_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_1_prop);
    const uint64_t offset_struct_largest_2_test_2_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_2_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_2_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_2_prop);
    const uint64_t offset_struct_largest_2_test_3_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_3_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_3_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_3_prop);
    const uint64_t offset_struct_largest_2_test_4_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_4_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_4_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_4_prop);
    const uint64_t offset_struct_largest_2_test_5_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_5_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_5_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_5_prop);
    const uint64_t offset_struct_largest_2_test_6_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_6_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_6_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_6_prop);
    const uint64_t offset_struct_largest_2_test_6_prop_after = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_6_prop_after),
                            (uint8_t *)&(m_properties.struct_largest_2_test_6_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_6_prop_after);
    const uint64_t offset_struct_largest_2_test_7_prop = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_7_prop),
                            (uint8_t *)&(m_properties.struct_largest_2_test_7_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_7_prop);
    const uint64_t offset_struct_largest_2_test_7_prop_after = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_largest_2_test_7_prop_after),
                            (uint8_t *)&(m_properties.struct_largest_2_test_7_alignment_pad));
    PRINT_VAR(offset_struct_largest_2_test_7_prop_after);
    const uint64_t offset_struct_array_largest_2_test_0_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_0_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_0_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_0_prop0);
    const uint64_t offset_struct_array_largest_2_test_1_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_1_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_1_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_1_prop0);
    const uint64_t offset_struct_array_largest_2_test_2_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_2_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_2_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_2_prop0);
    const uint64_t offset_struct_array_largest_2_test_3_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_3_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_3_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_3_prop0);
    const uint64_t offset_struct_array_largest_2_test_4_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_4_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_4_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_4_prop0);
    const uint64_t offset_struct_array_largest_2_test_5_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_5_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_5_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_5_prop0);
    const uint64_t offset_struct_array_largest_2_test_6_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_6_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_6_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_6_prop0);
    const uint64_t offset_struct_array_largest_2_test_7_prop0 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_7_prop[0]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_7_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_7_prop0);
    const uint64_t offset_struct_array_largest_2_test_0_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_0_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_0_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_0_prop1);
    const uint64_t offset_struct_array_largest_2_test_1_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_1_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_1_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_1_prop1);
    const uint64_t offset_struct_array_largest_2_test_2_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_2_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_2_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_2_prop1);
    const uint64_t offset_struct_array_largest_2_test_3_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_3_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_3_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_3_prop1);
    const uint64_t offset_struct_array_largest_2_test_4_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_4_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_4_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_4_prop1);
    const uint64_t offset_struct_array_largest_2_test_5_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_5_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_5_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_5_prop1);
    const uint64_t offset_struct_array_largest_2_test_6_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_6_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_6_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_6_prop1);
    const uint64_t offset_struct_array_largest_2_test_7_prop1 = 
        get_prop_mem_offset((uint8_t *)&(m_properties.struct_array_largest_2_test_7_prop[1]),
                            (uint8_t *)&(m_properties.struct_array_largest_2_test_7_alignment_pad));
    PRINT_VAR(offset_struct_array_largest_2_test_7_prop1);

    return RCC_DONE; // change this as needed for this worker to do something useful
    // return RCC_ADVANCE; when all inputs/outputs should be advanced each time "run" is called.
    // return RCC_ADVANCE_DONE; when all inputs/outputs should be advanced, and there is nothing more to do.
    // return RCC_DONE; when there is nothing more to do, and inputs/outputs do not need to be advanced.
  }
};

PROP_MEM_ALIGN_INFO_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PROP_MEM_ALIGN_INFO_END_INFO
