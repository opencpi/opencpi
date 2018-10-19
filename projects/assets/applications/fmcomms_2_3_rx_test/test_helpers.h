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

#ifndef _TEST_HELPERS_H
#define _TEST_HELPERS_H

#include <sstream> // std::ostringstream
#include <cstdio>  // printf()
#include <cmath>   // std::abs()
#include <iomanip> // std::setprecision()

#include "ocpi_component_prop_type_helpers.h" // append_to_oss()

#define TEST_EXPECTED_VAL(var, expected) test_expected_val(#var, var, expected); if(!(var == expected)) { printf("UNEXPECTED\n"); return false; } else { printf("EXPECTED\n"); }

#define TEST_EXPECTED_VAL_DIFF(var, expected, max_diff) test_expected_val_diff(#var, var, expected, max_diff); if(std::abs(var-expected) > max_diff) { printf("UNEXPECTED\n"); return false; } else { printf("EXPECTED\n"); }

//! @todo TODO/FIXME - this function should be deleted, it is a hack
void inline append_to_oss(std::ostringstream& oss, std::string val)
{
  oss << val;
}

//! @todo TODO/FIXME - this function should be deleted, it is a hack
void inline append_to_oss(std::ostringstream& oss, double val)
{
  oss << std::setprecision(15) << val;
}

template<typename T>
void test_expected_val(const char* var, T actual, T expected)
{
  std::ostringstream oss;
  oss << "      variable: " << var;
  oss << ",\texpected value: ";
  append_to_oss(oss, expected);
  oss << ",\tactual value: ";
  append_to_oss(oss, actual);
  oss << "\t";
  printf(oss.str().c_str());
}

void test_expected_val_diff(const char* var, double actual, double expected, double diff)
{
  std::ostringstream oss;
  oss << "      variable: " << var;
  oss << ",\texpected value: ";
  append_to_oss(oss, expected);
  oss << ",\tactual value: ";
  append_to_oss(oss, actual);
  oss << ",\tdiff: ";
  append_to_oss(oss, actual-expected);
  oss << ",\tmax allowable abs(diff): ";
  append_to_oss(oss, diff);
  oss << "\t";
  printf(oss.str().c_str());
}

#endif // _TEST_HELPERS_H
