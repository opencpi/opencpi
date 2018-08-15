/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed May 23 15:17:05 2018 EDT
 * BASED ON THE FILE: proxy1.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the proxy1 worker in C++
 */

#include "proxy1-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Proxy1WorkerTypes;
using namespace std;

class Proxy1Worker : public Proxy1WorkerBase {
  RCCResult run(bool /*timedout*/) {

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
    slaves.first_wkr1.set_my_enum(First_wkr1WorkerTypes::My_enum::MY_ENUM_THIRD_ENUM);

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
    slaves.second_wkr1.set_my_enum(Second_wkr1WorkerTypes::My_enum::MY_ENUM_THIRD_ENUM);

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
    std::string temp_str;
    slaves.first_wkr1.get_my_string(temp_str);
    cout << "slaves.first_wkr1.get_my_string(test_string): " << temp_str
         << endl;
    cout << "slaves.first_wkr1.set_my_enum(THIRD_ENUM) (2): " << slaves.first_wkr1.get_my_enum()
         << endl;

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
    slaves.second_wkr1.get_my_string(temp_str);
    cout << "slaves.second_wkr1.get_my_string(test_string): " << temp_str
         << endl;
    cout << "slaves.second_wkr1.set_my_enum(THIRD_ENUM) (2): " << slaves.second_wkr1.get_my_enum()
         << endl;

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
