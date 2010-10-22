
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
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
