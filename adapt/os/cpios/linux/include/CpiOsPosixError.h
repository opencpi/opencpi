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

#ifndef CPIOSPOSIXERROR_H__
#define CPIOSPOSIXERROR_H__

#include <string>

/*
 * Returns the message associated with an error code, i.e., one that
 * was returned from errno.
 */

namespace CPI {
  namespace OS {
    namespace Posix {

      std::string getErrorMessage (int errorCode)
	throw ();

    }
  }
}

#endif
