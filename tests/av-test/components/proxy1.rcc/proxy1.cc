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
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed May 23 15:17:05 2018 EDT
 * BASED ON THE FILE: proxy1.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the proxy1 worker in C++
 */

#include "proxy1-worker.hh"
//#include "temp_worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Proxy1WorkerTypes;
using namespace std;

class Proxy1Worker : public Proxy1WorkerBase {
  RCCResult run(bool /*timedout*/) {
    int i;
    std::string temp_str;
    slaves.first_wkr1.set_test_double(5.0);
    slaves.first_wkr1.set_test_ulong(10);
    slaves.first_wkr1.set_test_bool(true);
    slaves.first_wkr1.set_test_char('F');
    slaves.first_wkr1.set_test_float(2.0);
    slaves.first_wkr1.set_test_long(25);
    slaves.first_wkr1.set_test_longlong(250);
    slaves.first_wkr1.set_test_short(6);
    slaves.first_wkr1.set_test_uchar('G');
    slaves.first_wkr1.set_test_ulonglong(350);
    slaves.first_wkr1.set_test_ushort(16);
    slaves.first_wkr1.set_my_string("test_string");
    slaves.first_wkr1.setProperty_test_seq_ulong("1,2,3,4,6,7,8,100");
    slaves.first_wkr1.setProperty_test_seq_str("\"one\",\"two\",\"three\",\"four\",\"five\"");
    slaves.first_wkr1.setProperty_test_seq_of_ulong_arrays("{1,2,3,4,6,7,8,101},"
                                                           "{1,2,3,4,6,7,8,102},"
                                                           "{1,2,3,4,6,7,8,103},"
                                                           "{1,2,3,4,6,7,8,104},"
                                                           "{1,2,3,4,6,7,8,105},"
                                                           "{1,2,3,4,6,7,8,106},"
                                                           "{1,2,3,4,6,7,8,107},"
                                                           "{1,2,3,4,6,7,8,108}");
    slaves.first_wkr1.setProperty_test_struct("struct_bool true,struct_ulong 10,struct_char K");
    slaves.first_wkr1.setProperty_test_struct_of_seq("struct_char M, struct_ulong_seq {1,2,3}");
    slaves.first_wkr1.setProperty_test_array_of_struct(
        "{test_ulong 10,test_bool true,test_char A},"
        "{test_ulong 11,test_bool true,test_char B},"
        "{test_ulong 12,test_bool true,test_char C},"
        "{test_ulong 13,test_bool true,test_char D},"
        "{test_ulong 14,test_bool true,test_char E},"
        "{test_ulong 15,test_bool true,test_char F},"
        "{test_ulong 16,test_bool true,test_char G},"
        "{test_ulong 17,test_bool true,test_char H},"
        "{test_ulong 18,test_bool true,test_char I},"
        "{test_ulong 19,test_bool true,test_char J}");
    slaves.first_wkr1.setProperty_test_seq_of_structs(
        "{struct_bool true,struct_ulong 20,struct_char D},"
        "{struct_bool true,struct_ulong 21,struct_char E},"
        "{struct_bool true,struct_ulong 22,struct_char F},"
        "{struct_bool true,struct_ulong 23,struct_char G},"
        "{struct_bool true,struct_ulong 24,struct_char H},"
        "{struct_bool true,struct_ulong 25,struct_char I},"
        "{struct_bool true,struct_ulong 26,struct_char J}");
    slaves.first_wkr1.set_my_enum(First_wkr1WorkerTypes::My_enum::MY_ENUM_THIRD_ENUM);
    for (i = 0; i < 10; i++)
    {
      slaves.first_wkr1.set_test_array_ulong(i, i*10);
      slaves.first_wkr1.set_test_array_of_str(i, "that");
    }

    slaves.second_wkr1.set_test_double(5.0);
    slaves.second_wkr1.set_test_ulong(10);
    slaves.second_wkr1.set_test_bool(true);
    slaves.second_wkr1.set_test_char('F');
    slaves.second_wkr1.set_test_float(2.0);
    slaves.second_wkr1.set_test_long(25);
    slaves.second_wkr1.set_test_longlong(250);
    slaves.second_wkr1.set_test_short(6);
    slaves.second_wkr1.set_test_uchar('G');
    slaves.second_wkr1.set_test_ulonglong(350);
    slaves.second_wkr1.set_test_ushort(16);
    slaves.second_wkr1.set_my_string("test_string");
    slaves.second_wkr1.setProperty_test_seq_ulong("1,2,3,4,6,7,8,100");
    slaves.second_wkr1.setProperty_test_seq_str("\"one\",\"two\",\"three\",\"four\",\"five\"");
    slaves.second_wkr1.setProperty_test_seq_of_ulong_arrays("{1,2,3,4,6,7,8,101},"
                                                            "{1,2,3,4,6,7,8,102},"
                                                            "{1,2,3,4,6,7,8,103},"
                                                            "{1,2,3,4,6,7,8,104},"
                                                            "{1,2,3,4,6,7,8,105},"
                                                            "{1,2,3,4,6,7,8,106},"
                                                            "{1,2,3,4,6,7,8,107},"
                                                            "{1,2,3,4,6,7,8,108}");
    slaves.second_wkr1.setProperty_test_struct("struct_bool true,struct_ulong 10,struct_char K");
    slaves.second_wkr1.setProperty_test_struct_of_seq("struct_char M, struct_ulong_seq {1,2,3}");
    slaves.second_wkr1.setProperty_test_array_of_struct(
        "{test_ulong 10,test_bool true,test_char A},"
        "{test_ulong 11,test_bool true,test_char B},"
        "{test_ulong 12,test_bool true,test_char C},"
        "{test_ulong 13,test_bool true,test_char D},"
        "{test_ulong 14,test_bool true,test_char E},"
        "{test_ulong 15,test_bool true,test_char F},"
        "{test_ulong 16,test_bool true,test_char G},"
        "{test_ulong 17,test_bool true,test_char H},"
        "{test_ulong 18,test_bool true,test_char I},"
        "{test_ulong 19,test_bool true,test_char J}");
    slaves.second_wkr1.setProperty_test_seq_of_structs(
        "{struct_bool true,struct_ulong 20,struct_char D},"
        "{struct_bool true,struct_ulong 21,struct_char E},"
        "{struct_bool true,struct_ulong 22,struct_char F},"
        "{struct_bool true,struct_ulong 23,struct_char G},"
        "{struct_bool true,struct_ulong 24,struct_char H},"
        "{struct_bool true,struct_ulong 25,struct_char I},"
        "{struct_bool true,struct_ulong 26,struct_char J}");
    slaves.second_wkr1.set_my_enum(Second_wkr1WorkerTypes::My_enum::MY_ENUM_THIRD_ENUM);
    for (i = 0; i < 10; i++)
    {
      slaves.second_wkr1.set_test_array_ulong(i, i*10);
      slaves.second_wkr1.set_test_array_of_str(i, "that");
    }

    slaves.wkr2.set_test_double(5.0);
    slaves.wkr2.set_test_ulong(10);
    slaves.wkr2.set_test_bool(true);
    slaves.wkr2.set_test_char('F');
    slaves.wkr2.set_test_float(2.0);
    slaves.wkr2.set_test_long(25);
    slaves.wkr2.set_test_longlong(250);
    slaves.wkr2.set_test_short(6);
    slaves.wkr2.set_test_uchar('G');
    slaves.wkr2.set_test_ulonglong(350);
    slaves.wkr2.set_test_ushort(16);

    cout << "slaves.first_wkr1.set_test_double(5.0): " << slaves.first_wkr1.get_test_double()
         << endl;
    cout << "slaves.first_wkr1.set_test_ulong(10): " << slaves.first_wkr1.get_test_ulong()
         << endl;
    cout << "slaves.first_wkr1.set_test_bool(true): " << slaves.first_wkr1.get_test_bool()
         << endl;
    cout << "slaves.first_wkr1.set_test_char('F'): " << slaves.first_wkr1.get_test_char()
         << endl;
    cout << "slaves.first_wkr1.set_test_float(2.0): " << slaves.first_wkr1.get_test_float()
         << endl;
    cout << "slaves.first_wkr1.set_test_long(25): " << slaves.first_wkr1.get_test_long()
         << endl;
    cout << "slaves.first_wkr1.set_test_longlong(250): " << slaves.first_wkr1.get_test_longlong()
         << endl;
    cout << "slaves.first_wkr1.set_test_short(6): " << slaves.first_wkr1.get_test_short()
         << endl;
    cout << "slaves.first_wkr1.set_test_uchar('G'): " << slaves.first_wkr1.get_test_uchar()
         << endl;
    cout << "slaves.first_wkr1.set_test_ulonglong(350): " << slaves.first_wkr1.get_test_ulonglong()
         << endl;
    cout << "slaves.first_wkr1.set_test_ushort(16): " << slaves.first_wkr1.get_test_ushort()
         << endl;
    cout << "slaves.first_wkr1.get_my_string(test_string): "
         << slaves.first_wkr1.get_my_string(temp_str) << endl;
    cout << "slaves.first_wkr1.set_my_enum(THIRD_ENUM) (2): " << slaves.first_wkr1.get_my_enum()
         << endl;
    cout << "slaves.first_wkr1.get_test_struct: "
         << slaves.first_wkr1.getProperty_test_struct(temp_str) << endl;
    cout << "slaves.first_wkr1.get_test_struct_of_seq: "
         << slaves.first_wkr1.getProperty_test_struct_of_seq(temp_str) << endl;
    for (int i = 0; i < 10; i++)
    {
      cout << "slaves.first_wkr1.get_test_array_ulong [" << i << "]: "
           <<  slaves.first_wkr1.get_test_array_ulong(i) << endl;
      cout << "slaves.first_wkr1.get_test_array_of_str [" << i << "]: "
           << slaves.first_wkr1.get_test_array_of_str(i, temp_str) << endl;
    }
    cout << "slaves.first_wkr1.get_test_array_of_struct: "
         << slaves.first_wkr1.getProperty_test_array_of_struct(temp_str) << endl;
    cout << "slaves.first_wkr1.get_test_ulong_param: "
         << slaves.first_wkr1.get_test_ulong_param() << endl;
    cout << "slaves.second_wkr1.set_test_double(5.0): " << slaves.second_wkr1.get_test_double()
         << endl;
    cout << "slaves.second_wkr1.set_test_ulong(10): " << slaves.second_wkr1.get_test_ulong()
         << endl;
    cout << "slaves.second_wkr1.set_test_bool(true): " << slaves.second_wkr1.get_test_bool()
         << endl;
    cout << "slaves.second_wkr1.set_test_char('F'): " << slaves.second_wkr1.get_test_char()
         << endl;
    cout << "slaves.second_wkr1.set_test_float(2.0): " << slaves.second_wkr1.get_test_float()
         << endl;
    cout << "slaves.second_wkr1.set_test_long(25): " << slaves.second_wkr1.get_test_long()
         << endl;
    cout << "slaves.second_wkr1.set_test_longlong(250): " << slaves.second_wkr1.get_test_longlong()
         << endl;
    cout << "slaves.second_wkr1.set_test_short(6): " << slaves.second_wkr1.get_test_short()
         << endl;
    cout << "slaves.second_wkr1.set_test_uchar('G'): " << slaves.second_wkr1.get_test_uchar()
         << endl;
    cout << "slaves.second_wkr1.set_test_ulonglong(350): "
         << slaves.second_wkr1.get_test_ulonglong() << endl;
    cout << "slaves.second_wkr1.set_test_ushort(16): " << slaves.second_wkr1.get_test_ushort()
         << endl;
    cout << "slaves.second_wkr1.get_my_string(test_string): "
         << slaves.second_wkr1.get_my_string(temp_str) << endl;
    cout << "slaves.second_wkr1.set_my_enum(THIRD_ENUM) (2): " << slaves.second_wkr1.get_my_enum()
         << endl;
    cout << "slaves.second_wkr1.get_test_struct: "
         << slaves.second_wkr1.getProperty_test_struct(temp_str) << endl;
    cout << "slaves.second_wkr1.get_test_struct_of_seq: "
         << slaves.second_wkr1.getProperty_test_struct_of_seq(temp_str) << endl;
    for (int i = 0; i < 10; i++)
    {
      cout << "slaves.second_wkr1.get_test_array_ulong [" << i << "]: "
           <<  slaves.second_wkr1.get_test_array_ulong(i) << endl;
      cout << "slaves.second_wkr1.get_test_array_of_str [" << i << "]: "
           << slaves.second_wkr1.get_test_array_of_str(i, temp_str) << endl;
    }
    cout << "slaves.second_wkr1.get_test_array_of_struct: "
         << slaves.second_wkr1.getProperty_test_array_of_struct(temp_str) << endl;
    cout << "slaves.second_wkr1.get_test_ulong_param: "
         << slaves.second_wkr1.get_test_ulong_param() << endl;

    cout << "slaves.first_wkr1.get_test_my_param1(): " << slaves.first_wkr1.get_my_param1() << endl;
    cout << "slaves.first_wkr1.get_test_my_param2(): " << slaves.first_wkr1.get_my_param1() << endl;
    slaves.first_wkr1.set_my_debug1(34);
    cout << "slaves.first_wkr1.get_test_my_debug1(): " << slaves.first_wkr1.get_my_debug1() << endl;
    cout << "slaves.wkr2.set_test_double(5.0): " << slaves.wkr2.get_test_double() << endl;
    cout << "slaves.wkr2.set_test_ulong(10): " << slaves.wkr2.get_test_ulong() << endl;
    cout << "slaves.wkr2.set_test_bool(true): " << slaves.wkr2.get_test_bool() << endl;
    cout << "slaves.wkr2.set_test_char('F'): " << slaves.wkr2.get_test_char() << endl;
    cout << "slaves.wkr2.set_test_float(2.0): " << slaves.wkr2.get_test_float() << endl;
    cout << "slaves.wkr2.set_test_long(25): " << slaves.wkr2.get_test_long() << endl;
    cout << "slaves.wkr2.set_test_longlong(250): " << slaves.wkr2.get_test_longlong() << endl;
    cout << "slaves.wkr2.set_test_short(25): " << slaves.wkr2.get_test_short() << endl;
    cout << "slaves.wkr2.set_test_uchar('G'): " << slaves.wkr2.get_test_uchar() << endl;
    cout << "slaves.wkr2.set_test_ulonglong(350): " << slaves.wkr2.get_test_ulonglong() << endl;
    cout << "slaves.wkr2.set_test_ushort(16): " << slaves.wkr2.get_test_ushort() << endl;
    bool caught = false;
    try { slaves.wkr2.set_my_debug1(123); } catch(...) { caught = true; }
    if (!caught)
      return setError("setting debug property in a non-debug worker did not fail");
    cout << "slaves.wkr2.set_my_debug1(123) failed as expected\n";
    caught = false;
    try { slaves.wkr2.get_my_debug1(); } catch(...) { caught = true; }
    if (!caught)
      return setError("getting debug property in a non-debug worker did not fail");
    cout << "slaves.wkr2.get_my_debug2() failed as expected\n";

    if(isOperating()) {
      cout << "proxy1 its operating" << endl;
    }

    if (slaves.first_wkr1.isOperating()) {
      cout << "first_wkr1 its operating" << endl;
    }
    if (slaves.second_wkr1.isOperating()) {
      cout << "first_wkr1 its operating" << endl;
    }
    if (slaves.wkr2.isOperating()) {
      cout << "first_wkr1 its operating" << endl;
    }
    return RCC_DONE; // change this as needed for this worker to do something useful
    //return RCC_ADVANCE; //when all inputs/outputs should be advanced each time "run" is called.
    // return RCC_ADVANCE_DONE; when all inputs/outputs should be advanced, and there is nothing more to do.
    // return RCC_DONE; when there is nothing more to do, and inputs/outputs do not need to be advanced.
  }
};

PROXY1_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PROXY1_END_INFO
