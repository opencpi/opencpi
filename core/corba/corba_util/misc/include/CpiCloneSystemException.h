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

#ifndef CPI_CORBAUTIL_CLONESYSTEMEXCEPTION_H__
#define CPI_CORBAUTIL_CLONESYSTEMEXCEPTION_H__

/**
 * \file
 * \brief Helper function to clone a CORBA system exception.
 */

#include <corba.h>

namespace CPI {
  namespace CORBAUtil {
    /**
     * \brief Miscellaneous CORBA-related utilities.
     */

    namespace Misc {

      /**
       * \brief Clone a CORBA system exception.
       *
       * This function is useful e.g., when a system exception is caught
       * and needs to be stored for future use.  Within the <tt>catch</tt>
       * block, the exception exists in the local scope only.  As a base
       * class, it is not possible to use the assignment operator.
       *
       * \param[in] ex A CORBA system exception.
       * \return A clone of \a ex.  Must eventually be deleted.
       */

      CORBA::SystemException *
      cloneSystemException (const CORBA::SystemException & ex)
        throw ();

    }
  }
}

#endif

