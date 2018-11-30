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

#include <iostream> // std::cout
#include <string> // std::string
#include "UtilValidRangesTester.hh"
#include "UtilValidRanges.hh"

namespace OCPIProjects {

namespace Util {

bool ValidRangesTester::run_tests() const {
  try {
    // @TODO / FIXME - expand these tests to do more than just catch exceptions
    {
      // add_valid_range() example
      OCPIProjects::Util::ValidRanges<Range<double> > r;
      std::cout << "Adding valid range: [1. 3.]\n";
      r.add_valid_range(1.,3.);
      std::cout << "Contents of ValidRanges object: " << r << "\n";
      std::cout << "Adding valid range: [10. 20.]\n";
      r.add_valid_range(10.,20.);
      std::cout << "Contents of ValidRanges object: " << r << "\n";
      std::cout << "Adding valid range: [9. 30.]\n";
      r.add_valid_range(9.,30.);
      std::cout << "Contents of ValidRanges object: " << r << "\n";
    }
    {
      // add_invalid_range() example
      OCPIProjects::Util::ValidRanges<Range<double> > r;
      std::cout << "Adding valid range: [1. 3.]\n";
      r.add_valid_range(1.,3.);
      std::cout << "Adding valid range: [10. 20.]\n";
      r.add_valid_range(10,20.);
      std::cout << "Adding valid range: [20. 30.]\n";
      r.add_valid_range(20,30.);
      std::cout << "Contents of ValidRanges object: " << r << "\n";
      std::cout << "Adding invalid range: (2. 30.)\n";
      r.add_invalid_range(2,30.);
      std::cout << "Contents of ValidRanges object: " << r << "\n";
    }
    {
      //overlap() example
      OCPIProjects::Util::ValidRanges<Range<double> > r;
      r.add_valid_range(1.,3.);
      r.add_valid_range(10.,20.);
      r.add_valid_range(30.,40.);
      OCPIProjects::Util::ValidRanges<Range<double> > r2;
      r2.add_valid_range(2.,4.);
      r2.add_valid_range(10.,10.);
      r2.add_valid_range(29.,41.);
      std::cout << "      Overlapping " << r;
      std::cout << " with "       << r2 << "\n";
      r.overlap(r2);
      std::cout << "      Result of overlap: " << r << "\n";
    }
    {
      //is_valid() example
      OCPIProjects::Util::ValidRanges<Range<double> > r;
      std::cout << "      Adding valid range: [1. 3.]\n";
      r.add_valid_range(1.,3.);
      std::cout << "      Contents of ValidRanges object: " << r <<"\n";
      std::cout << "      2.999 is valid: " << (r.is_valid(2.999) ? "true":"false");
      std::cout << "\n";
      std::cout << "      3.    is valid: " << (r.is_valid(3.   ) ? "true":"false");
      std::cout << "\n";
      std::cout << "      3.001 is valid: " << (r.is_valid(3.001) ? "true":"false");
      std::cout << "\n";
    }
    {
      //clear() example
      OCPIProjects::Util::ValidRanges<Range<double> > r;
      std::cout << "      Adding valid range: [1. 3.]\n";
      r.add_valid_range(1.,3.);
      std::cout << "      Contents of ValidRanges object: " << r <<"\n";
      std::cout << "      Clearing\n";
      r.clear();
      std::cout << "      Contents of ValidRanges object: " << r <<"\n";
    }
    {
      //<< example
      OCPIProjects::Util::ValidRanges<Range<double> > r;
      std::cout << "      Adding valid range: [1. 3.]\n";
      r.add_valid_range(1.,3.);
      std::cout << "      Contents of ValidRanges object: " << r <<"\n";
    }

    test_type<int16_t >(true,  "int16_t" );
    test_type<uint16_t>(false, "uint16_t");
    test_type<int32_t >(true,  "int32_t" );
    test_type<uint32_t>(false, "uint32_t");
    test_type<int64_t >(true,  "int64_t" );
    test_type<uint64_t>(false, "uint64_t");
    test_type<float   >(true,  "float"   );
    test_type<double  >(true,  "double"  );
  }
  catch(std::string& e) {
    std::cerr << "Caught exception: " << e << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "Caught unkown exception\n";
    return false;
  }
  std::cout << "PASSED\n";
  return true;
}

template<typename T>
bool ValidRangesTester::test_type(bool type_supports_negative,
    const char* type) const {

  std::cout << "************* testing type:  ";
  std::cout << type << "   *************\n";
  std::cout << "************* calling constructor *************\n";
  OCPIProjects::Util::ValidRanges<Range<T> > ranges;
  std::cout << ranges << "\n";
  std::cout << "************* clearing ranges     *************\n";
  ranges.clear();
  std::cout << ranges << "\n";
  test_add_valid_range(ranges,(T)0,(T)1);
  test_add_valid_range(ranges,(T)2,(T)3);
  test_add_valid_range(ranges,(T)100,(T)200);
  test_add_valid_range(ranges,(T)50,(T)120);
  test_add_valid_range(ranges,(T)140,(T)150);
  test_add_valid_range(ranges,(T)160,(T)170);
  test_add_valid_range(ranges,(T)210,(T)220);
  test_add_valid_range(ranges,(T)230,(T)240);
  test_add_valid_range(ranges,(T)0,(T)0);
  test_add_valid_range(ranges,(T)0,(T)-0);
  test_add_valid_range(ranges,(T)-0,(T)0);
  test_add_valid_range(ranges,(T)-0,(T)-0);
  if(type_supports_negative) {
    test_add_valid_range(ranges,(T)-1,(T)1);
    test_is_valid(ranges, (T)-1, type_supports_negative );
  }
  test_is_valid(ranges, (T)0,  true );
  test_is_valid(ranges, (T)1,  true );
  test_is_valid(ranges, (T)2,  true );
  test_is_valid(ranges, (T)3,  true );
  test_is_valid(ranges, (T)4,  false);
  test_is_valid(ranges, (T)49, false);
  test_is_valid(ranges, (T)50, true );
  test_is_valid(ranges, (T)51, true );
  test_is_valid(ranges, (T)99, true );
  test_is_valid(ranges, (T)100,true );
  test_is_valid(ranges, (T)101,true );
  test_is_valid(ranges, (T)119,true );
  test_is_valid(ranges, (T)120,true );
  test_is_valid(ranges, (T)121,true );
  test_is_valid(ranges, (T)139,true );
  test_is_valid(ranges, (T)140,true );
  test_is_valid(ranges, (T)141,true );
  test_is_valid(ranges, (T)150,true );
  test_is_valid(ranges, (T)149,true );
  test_is_valid(ranges, (T)151,true );
  test_is_valid(ranges, (T)160,true );
  test_is_valid(ranges, (T)159,true );
  test_is_valid(ranges, (T)161,true );
  test_is_valid(ranges, (T)169,true );
  test_is_valid(ranges, (T)170,true );
  test_is_valid(ranges, (T)171,true );
  test_is_valid(ranges, (T)199,true );
  test_is_valid(ranges, (T)200,true );
  test_is_valid(ranges, (T)201,false);
  test_is_valid(ranges, (T)209,false);
  test_is_valid(ranges, (T)210,true );
  test_is_valid(ranges, (T)211,true );
  test_is_valid(ranges, (T)219,true );
  test_is_valid(ranges, (T)220,true );
  test_is_valid(ranges, (T)221,false);
  test_is_valid(ranges, (T)229,false);
  test_is_valid(ranges, (T)230,true );
  test_is_valid(ranges, (T)231,true );
  test_is_valid(ranges, (T)239,true );
  test_is_valid(ranges, (T)240,true );
  test_is_valid(ranges, (T)241,false);
  test_add_invalid_range(ranges,(T)45,(T)46);
  test_add_invalid_range(ranges,(T)45,(T)50);
  test_add_invalid_range(ranges,(T)45,(T)51);
  test_add_invalid_range(ranges,(T)55,(T)60);
  test_add_invalid_range(ranges,(T)0,(T)0);
  test_add_invalid_range(ranges,(T)0,(T)-0);
  test_add_invalid_range(ranges,(T)-0,(T)0);
  test_add_invalid_range(ranges,(T)-0,(T)-0);
  test_impose_min_for_all_ranges(ranges,(T)53);
  test_impose_max_for_all_ranges(ranges,(T)123);
  if(type_supports_negative) {
    test_add_invalid_range(ranges,(T)-1,(T)241);
    test_add_valid_range(ranges,(T)-20,(T)20);
    test_add_invalid_range(ranges,(T)-1,(T)1);
  }
  ranges.clear();
  test_add_valid_range(ranges,(T)1.,(T)10.);
  test_impose_min_for_all_ranges(ranges,(T)10.);
  ranges.clear();
  test_add_valid_range(ranges,(T)1.,(T)10.);
  test_add_valid_range(ranges,(T)20.,(T)30.);
  OCPIProjects::Util::ValidRanges<Range<T> > rangeso;
  rangeso.add_valid_range((T)5,(T)50);
  ranges.overlap(rangeso);
  std::cout << "************* overlapping range [ 5 - 50 ]    *************\n";
  std::cout << ranges << "\n";
  return true;
}

template<typename T>
void ValidRangesTester::test_add_valid_range(ValidRanges<Range<T> >& ranges,
    const T min, const T max) const {

  std::cout << "************* adding valid range [" << min << "," << max << "]    *************\n";
  ranges.add_valid_range(min, max);
  std::cout << ranges << "\n";
}

template<typename T>
void ValidRangesTester::test_add_invalid_range(ValidRanges<Range<T> >& ranges,
    const T min, const T max) const {

  std::cout << "************* adding invalid range [" << min << "," << max << "]    *************\n";
  ranges.add_invalid_range(min, max);
  std::cout << ranges << "\n";
}

template<typename T>
void ValidRangesTester::test_impose_min_for_all_ranges(ValidRanges<Range<T> >& ranges,
    const T min) const {

  std::cout << "************* imposing min " << min << "    *************\n"; \
  ranges.impose_min_for_all_ranges(min);
  std::cout << ranges << "\n";
}

template<typename T>
void ValidRangesTester::test_impose_max_for_all_ranges(ValidRanges<Range<T> >& ranges,
    const T max) const {

  std::cout << "************* imposing max " << max << "    *************\n"; \
  ranges.impose_max_for_all_ranges(max);
  std::cout << ranges << "\n";
}

template<typename T>
void ValidRangesTester::test_is_valid(ValidRanges<Range<T> >& ranges,
    const T val, const bool desired) const {

  std::cout << "TEST: value " << val << " is valid:\t"; 
  const bool is_valid = ranges.is_valid(val);
  std::cout << (is_valid? "true " : "false");
  if(is_valid == desired)
  {
    std::cout << "\tEXPECTED\n";
  }
  else
  {
    std::cout << "\tUNEXPECTED\nFAILED\n";
    throw std::string("FAILED");
  }
}

} // namespace Util

} // namespace OCPIProjects
