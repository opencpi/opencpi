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

// -*- c++ -*-

#ifndef OCPIOSSIZECHECK_H__
#define OCPIOSSIZECHECK_H__

/**
 * \file
 *
 * \brief A compile-time "greater than" check.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

namespace {

  template<int i>
    int
    sufficientCapacity ()
    {
      return 1;
    }

  /**
   * A compile-time "greater than" check that causes a compile-time
   * error if the check fails. Use as
   *
   *     assert ((compileTimeSizeCheck<i1,i2> ()));
   *
   * This will throw a "division by zero"  compile-time error if i1 is
   * less than i2, and compile silently if i1 is greater than or equal
   * to i2.
   *
   * Note the extra set parentheses around the expression; this is
   * to please the preprocessor, which otherwise thinks that '<i1,i2>'
   * is the less-than operator, and believes that the comma starts
   * a second parameter to the assert macro.
   */

  template<int i, int j>
    int
    compileTimeSizeCheck ()
    {
      return sufficientCapacity<1 / ((i >= j) ? 1 : 0)> ();
    }

}

#endif
