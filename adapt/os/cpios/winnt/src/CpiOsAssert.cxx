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

#include <CpiOsAssert.h>
#include <iostream>
#include <cstdlib>

#if !defined (NDEBUG)
#include <CpiOsDebug.h>
#endif

namespace {

  CPI::OS::AssertionCallback g_assertionCallback = 0;

  void
  defaultAssertionCallback (const char * cond,
			    const char * file,
			    unsigned int line)
  {
    std::cerr << std::endl
	      << "Assertion failed: " << cond << " is false at "
	      << file << ":" << line << "."
	      << std::endl;

#if !defined (NDEBUG)
    std::cerr << "Stack Dump:"
	      << std::endl;
    CPI::OS::dumpStack (std::cerr);
    CPI::OS::debugBreak ();
#endif

    std::abort ();
  }

}

void
CPI::OS::setAssertionCallback (AssertionCallback cb)
  throw ()
{
  g_assertionCallback = cb;
}

bool
CPI::OS::assertionFailed (const char * cond, const char * file, unsigned int line)
  throw ()
{
  if (g_assertionCallback) {
    (*g_assertionCallback) (cond, file, line);
  }
  else {
    defaultAssertionCallback (cond, file, line);
  }

  // the assertion callback shall not return, so we should not be here.
  std::abort ();
  return false;
}
