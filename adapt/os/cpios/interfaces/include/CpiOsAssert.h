// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

// -*- c++ -*-

#ifndef CPIOSASSERT_H__
#define CPIOSASSERT_H__

/**
 * \file
 * \brief The cpiAssert() macro.
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Moved over from CpiOsDebug.h
 */

#include <iostream>

namespace CPI {
  namespace OS {

    /**
     * \brief Typedef for an assertion callback function.
     *
     * The callback takes three parameters:
     *
     * - \em cond The expression that was tested, and which evaluates to
     *            false.
     * - \em file The name of the source file that contains this assertion.
     * - \em line The line in the source file where this expression tested
     *            false.
     *
     * The callback shall not return.
     */

    typedef void (*AssertionCallback) (const char * cond,
				       const char * file,
				       unsigned int line);

    /**
     * Set a callback function to call when an assertion fails.
     *
     * \param[in] cb The callback function.
     */

    void setAssertionCallback (AssertionCallback cb)
      throw ();

    /**
     * An internal function used by the cpiAssert macro.
     *
     * "Test" an assertion condition. There isn't really anything to test.
     * This is just to avoid warnings like "statement has no effect" when
     * testing for something like cpiAssert (sizeof(int) == 4).
     *
     * Not called by user code.
     */

    bool testAssertion (bool)
      throw ();

    /**
     * Default assertion callback.
     *
     * Prints a message and a stack trace (if built with debugging enabled)
     * to standard error (std::cerr), then aborts.
     *
     * This function can be used with setAssertionCallback() to reset the
     * assertion callback to the default.
     */

    bool assertionFailed (const char *, const char *, unsigned int)
      throw ();

  }
}

inline
bool
CPI::OS::testAssertion (bool cond)
  throw ()
{
  return !!cond;
}

/**
 * \def cpiAssert(cond)
 *
 * Tests a "sanity check" condition. When the NDEBUG preprocessor
 * symbol is not defined, the condition is evaluated. If true, the
 * program continues. If false, a message and stack trace are
 * printed to std::cout.
 * \n
 * If the "sanity check" failed, the function does not return, but
 * first breaks into the debugger using debugBreak(), and then
 * aborts the program.
 * \n
 * If the NDEBUG preprocessor symbol is defined, then the condition
 * is not evaluated, and the program continues regardless of the
 * condition's value.
 */

#if defined (cpiAssert)
#undef cpiAssert
#endif

#if defined (NDEBUG)
#define cpiAssert(cond) ((void)0)
#else
#define cpiAssert(cond) ((::CPI::OS::testAssertion ((cond) ? true : false)) || ::CPI::OS::assertionFailed (#cond, __FILE__, __LINE__))
#endif

#endif
