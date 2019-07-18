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

#ifndef _OCPI_PROJECTS_UTIL_VALID_RANGES_TESTER_HH
#define _OCPI_PROJECTS_UTIL_VALID_RANGES_TESTER_HH

#include "UtilValidRanges.hh"

namespace OCPIProjects {

namespace Util {

class ValidRangesTester {

///@return Boolean indicator of successful completion of all tests.
public    : bool run_tests() const;

protected : template<typename T> bool test_type(bool type_supports_negative,
                const char* type) const;
protected : template<typename T> void test_add_valid_range(
                ValidRanges<Range<T> >& ranges, T min, T max) const;
protected : template<typename T> void test_add_invalid_range(
                ValidRanges<Range<T> >& ranges, T min, T max) const;
protected : template<typename T> void test_impose_min_for_all_ranges(
                ValidRanges<Range<T> >& ranges, T min) const;
protected : template<typename T> void test_impose_max_for_all_ranges(
                ValidRanges<Range<T> >& ranges, T max) const;
protected : template<typename T> void test_is_valid(
                ValidRanges<Range<T> >& ranges, T val, bool desired) const;
}; // class ValidRangesTester 

} // namespace Util

} // namespace OCPIProjects

#include "UtilValidRangesTester.cc"

#endif // _OCPI_PROJECTS_UTIL_VALID_RANGES_TESTER_HH
