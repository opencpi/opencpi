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

#ifndef CPI_CORBAUTIL_STRINGIFYCORBAEXCEPTION_H__
#define CPI_CORBAUTIL_STRINGIFYCORBAEXCEPTION_H__

/**
 * \file
 * \brief Create a human-readable string from a CORBA exception.
 *
 * Revision History:
 *
 *     05/09/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <corba.h>

namespace CPI {
  namespace CORBAUtil {
    /**
     * \brief Miscellaneous CORBA-related utilities.
     */

    namespace Misc {

      /**
       * \brief Create a human-readable string from a CORBA exception.
       *
       * This function works best for CORBA system exceptions; for user
       * exceptions, only the name of the exception is returned.
       *
       * \param[in] ex A CORBA exception.
       * \return A human-readable string describing the exception.
       */

      std::string stringifyCorbaException (const CORBA::Exception & ex)
	throw ();

    }
  }
}

#endif
