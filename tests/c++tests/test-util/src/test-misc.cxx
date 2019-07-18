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

#include "gtest/gtest.h"
#include "OcpiUtilMisc.h"

namespace {
  namespace OU = OCPI::Util;
  using namespace std;

// TODO: The rest of this library...

// exclude old compilers that cannot handle the make_tuple stuff
#if (__cplusplus >= 201103)

  // integerToString test cases

  typedef tuple<int, string> its_tuple_t;
  class its_Test :
    public ::testing::TestWithParam<its_tuple_t> {};
  its_tuple_t const its_test_table[] = {
    make_tuple( 0, "0"),
    make_tuple( 1, "1"),
    make_tuple( -5, "-5"),
    make_tuple( 12345, "12345"),
    make_tuple( -12345, "-12345"),
    make_tuple( 2147483647, "2147483647"),
    make_tuple( -2147483647, "-2147483647"),
  };
  TEST_P(its_Test, standard) {
      EXPECT_EQ( OU::integerToString(get<0>(GetParam())), get<1>(GetParam()) );
  }
  INSTANTIATE_TEST_CASE_P(integer_to_string_tests,
                          its_Test,
                          ::testing::ValuesIn(its_test_table) );

  // stringToInteger test cases
  // re-uses its_tuple_t and its_test_table by changing order in calls
  class sti_Test :
    public ::testing::TestWithParam<its_tuple_t> {};
  TEST_P(sti_Test, standard) {
      EXPECT_EQ( OU::stringToInteger(get<1>(GetParam())),
                 get<0>(GetParam()) );
  }
  INSTANTIATE_TEST_CASE_P(string_to_integer_tests,
                          sti_Test,
                          ::testing::ValuesIn(its_test_table) );

  // unsignedToString (unsigned int) test cases

  typedef tuple<unsigned int /* value */, unsigned int /* base */, unsigned int /* mindigits */, char /* pad */, string /* result */> uts1_tuple_t;
  class uts1_Test :
    public ::testing::TestWithParam<uts1_tuple_t> {};
  uts1_tuple_t const uts1_test_table[] = {
    make_tuple( 0, 2, 1, 'X', "0"),
    // Scan bases and lengths
    make_tuple( 0, 2, 1, '0', "0"),
    make_tuple( 0, 3, 2, '0', "00"),
    make_tuple( 0, 4, 3, '0', "000"),
    make_tuple( 0, 5, 4, '0', "0000"),
    make_tuple( 0, 6, 5, '0', "00000"),
    make_tuple( 0, 7, 6, '0', "000000"),
    make_tuple( 0, 8, 7, '0', "0000000"),
    make_tuple( 0, 9, 8, '0', "00000000"),
    make_tuple( 0, 10, 9, '0', "000000000"),
    make_tuple( 0, 11, 10, '0', "0000000000"),
    make_tuple( 0, 12, 11, '0', "00000000000"),
    make_tuple( 0, 13, 12, '0', "000000000000"),
    make_tuple( 0, 14, 13, '0', "0000000000000"),
    make_tuple( 0, 15, 14, '0', "00000000000000"),
    make_tuple( 0, 16, 15, '0', "000000000000000"),
    // Some more base stuff - thanks https://www.rapidtables.com/convert/number/base-converter.html
    make_tuple( 2147483647, 2, 1, '0', "1111111111111111111111111111111"),
    make_tuple( 2147483647, 3, 1, '0', "12112122212110202101"),
    make_tuple( 2147483647, 4, 1, '0', "1333333333333333"),
    make_tuple( 2147483647, 5, 1, '0', "13344223434042"),
    make_tuple( 2147483647, 6, 1, '0', "553032005531"),
    make_tuple( 2147483647, 7, 1, '0', "104134211161"),
    make_tuple( 2147483647, 8, 1, '0', "17777777777"),
    make_tuple( 2147483647, 9, 1, '0', "5478773671"),
    make_tuple( 2147483647, 10, 1, '0', "2147483647"),
    make_tuple( 2147483647, 11, 1, '0', "a02220281"),
    make_tuple( 2147483647, 12, 1, '0', "4bb2308a7"),
    make_tuple( 2147483647, 13, 1, '0', "282ba4aaa"),
    make_tuple( 2147483647, 14, 1, '0', "1652ca931"),
    make_tuple( 2147483647, 15, 1, '0', "c87e66b7"),
    make_tuple( 2147483647, 16, 1, '0', "7fffffff"),
    make_tuple( 4294967295, 2, 1, '0', "11111111111111111111111111111111"),
    make_tuple( 4294967295, 3, 1, '0', "102002022201221111210"),
    make_tuple( 4294967295, 4, 1, '0', "3333333333333333"),
    make_tuple( 4294967295, 5, 1, '0', "32244002423140"),
    make_tuple( 4294967295, 6, 1, '0', "1550104015503"),
    make_tuple( 4294967295, 7, 1, '0', "211301422353"),
    make_tuple( 4294967295, 8, 1, '0', "37777777777"),
    make_tuple( 4294967295, 9, 1, '0', "12068657453"),
    make_tuple( 4294967295, 10, 1, '0', "4294967295"),
    make_tuple( 4294967295, 11, 1, '0', "1904440553"),
    make_tuple( 4294967295, 12, 1, '0', "9ba461593"),
    make_tuple( 4294967295, 13, 1, '0', "535a79888"),
    make_tuple( 4294967295, 14, 1, '0', "2ca5b7463"),
    make_tuple( 4294967295, 15, 1, '0', "1a20dcd80"),
    make_tuple( 4294967295, 16, 1, '0', "ffffffff"),
  };
  uts1_tuple_t const uts1_padding_test_table[] = {
    make_tuple( 0, 2, 2, 'X', "X0"),
    make_tuple( 0, 2, 29, '?', "????????????????????????????0"),
    make_tuple( 4294967295, 16, 11, '#', "###ffffffff"),
  };

  TEST_P(uts1_Test, standard) {
      EXPECT_EQ( OU::unsignedToString(get<0>(GetParam()), get<1>(GetParam()), get<2>(GetParam()), get<3>(GetParam())),
                 get<4>(GetParam()) );
  }
  INSTANTIATE_TEST_CASE_P(unsigned_to_string_tests,
                          uts1_Test,
                          ::testing::ValuesIn(uts1_test_table) );
  INSTANTIATE_TEST_CASE_P(unsigned_to_string_tests_padding,
                          uts1_Test,
                          ::testing::ValuesIn(uts1_padding_test_table) );

  // stringToUnsigned test cases
  // re-uses uts1_tuple_t and uts1_test_table by changing order in calls
  class stus_Test :
    public ::testing::TestWithParam<uts1_tuple_t> {};
  TEST_P(stus_Test, standard) {
      EXPECT_EQ( OU::stringToUnsigned(get<4>(GetParam()), get<1>(GetParam())),
                 get<0>(GetParam()) );
  }
  INSTANTIATE_TEST_CASE_P(string_to_us_tests,
                          stus_Test,
                          ::testing::ValuesIn(uts1_test_table) );

  // unsignedToString (unsigned long long) test cases

  typedef tuple<unsigned long long /* value */, unsigned int /* base */, unsigned int /* mindigits */, char /* pad */, string /* result */> uts2_tuple_t;
  class uts2_Test :
    public ::testing::TestWithParam<uts2_tuple_t> {};
  uts2_tuple_t const uts2_test_table[] = {
    make_tuple( 0, 2, 1, 'X', "0"),
    // Scan bases and lengths
    make_tuple( 0, 2, 1, '0', "0"),
    make_tuple( 0, 3, 2, '0', "00"),
    make_tuple( 0, 4, 3, '0', "000"),
    make_tuple( 0, 5, 4, '0', "0000"),
    make_tuple( 0, 6, 5, '0', "00000"),
    make_tuple( 0, 7, 6, '0', "000000"),
    make_tuple( 0, 8, 7, '0', "0000000"),
    make_tuple( 0, 9, 8, '0', "00000000"),
    make_tuple( 0, 10, 9, '0', "000000000"),
    make_tuple( 0, 11, 10, '0', "0000000000"),
    make_tuple( 0, 12, 11, '0', "00000000000"),
    make_tuple( 0, 13, 12, '0', "000000000000"),
    make_tuple( 0, 14, 13, '0', "0000000000000"),
    make_tuple( 0, 15, 14, '0', "00000000000000"),
    make_tuple( 0, 16, 15, '0', "000000000000000"),
    // Some more base stuff - thanks https://www.rapidtables.com/convert/number/base-converter.html
    make_tuple( 18446744073709551615ull, 2, 1, '0', "1111111111111111111111111111111111111111111111111111111111111111"),
    make_tuple( 18446744073709551615ull, 3, 1, '0', "11112220022122120101211020120210210211220"),
    make_tuple( 18446744073709551615ull, 4, 1, '0', "33333333333333333333333333333333"),
    make_tuple( 18446744073709551615ull, 5, 1, '0', "2214220303114400424121122430"),
    make_tuple( 18446744073709551615ull, 6, 1, '0', "3520522010102100444244423"),
    make_tuple( 18446744073709551615ull, 7, 1, '0', "45012021522523134134601"),
    make_tuple( 18446744073709551615ull, 8, 1, '0', "1777777777777777777777"),
    make_tuple( 18446744073709551615ull, 9, 1, '0', "145808576354216723756"),
    make_tuple( 18446744073709551615ull, 10, 1, '0', "18446744073709551615"),
    make_tuple( 18446744073709551615ull, 11, 1, '0', "335500516a429071284"),
    make_tuple( 18446744073709551615ull, 12, 1, '0', "839365134a2a240713"),
    make_tuple( 18446744073709551615ull, 13, 1, '0', "219505a9511a867b72"),
    make_tuple( 18446744073709551615ull, 14, 1, '0', "8681049adb03db171"),
    make_tuple( 18446744073709551615ull, 15, 1, '0', "2c1d56b648c6cd110"),
    make_tuple( 18446744073709551615ull, 16, 1, '0', "ffffffffffffffff"),
  };
  // These are separated out because stringToULongLong would not accept as input:
  uts2_tuple_t const uts2_padding_test_table[] = {
    make_tuple( 0, 2, 2, 'X', "X0"),
    make_tuple( 0, 2, 29, '?', "????????????????????????????0"),
    make_tuple( 18446744073709551615ull, 16, 19, '#', "###ffffffffffffffff"),
  };

  TEST_P(uts2_Test, standard) {
      EXPECT_EQ( OU::unsignedToString(get<0>(GetParam()), get<1>(GetParam()), get<2>(GetParam()), get<3>(GetParam())),
                 get<4>(GetParam()) );
  }
  INSTANTIATE_TEST_CASE_P(unsigned_to_string_tests_ull,
                          uts2_Test,
                          ::testing::ValuesIn(uts2_test_table) );
  INSTANTIATE_TEST_CASE_P(unsigned_to_string_tests_ull_padding,
                          uts2_Test,
                          ::testing::ValuesIn(uts2_padding_test_table) );

#if 0
// Removed from OcpiUtilMisc
  // stringToULongLong test cases
  // re-uses uts2_tuple_t and uts2_test_table by changing order in calls
  class stull_Test :
    public ::testing::TestWithParam<uts2_tuple_t> {};
  TEST_P(stull_Test, standard) {
      EXPECT_EQ( OU::stringToULongLong(get<4>(GetParam()), get<1>(GetParam())),
                 get<0>(GetParam()) );
  }
  INSTANTIATE_TEST_CASE_P(string_to_ull_tests,
                          stull_Test,
                          ::testing::ValuesIn(uts2_test_table) );
#endif // stringToULongLong
#endif // old compilers guard

  // Test system ID (AV-4173)
  // https://github.com/google/googletest/blob/master/googlemock/docs/CookBook.md#mocking-free-functions
  // says we cannot easily spoof "if_nameindex" like originally planned, so no real test except it doesn't throw
  class TestSystemID : public ::testing::Test {};
  TEST( TestSystemID, getSystemId )
  {
    auto id = OU::getSystemId();
    // cerr << id << endl;
    EXPECT_NE( id, "" );
    EXPECT_NE( id, "00:00:00:00:00:00" );
  }

} // anon namespace
