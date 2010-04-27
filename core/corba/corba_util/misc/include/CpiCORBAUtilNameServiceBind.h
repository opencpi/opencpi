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

#ifndef CPI_CORBAUTIL_NAMESERVICEBIND_H__
#define CPI_CORBAUTIL_NAMESERVICEBIND_H__

/**
 * \file
 * \brief Bind an object in the Naming Service.
 *
 * Revision History:
 *
 *     07/07/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <string>
#include <CosNaming.h>

namespace CPI {
  namespace CORBAUtil {
    /**
     * \brief Miscellaneous CORBA-related utilities.
     */

    namespace Misc {

      /**
       * \brief Helper function to bind an object in the Naming Service.
       *
       * \param[in] ns      The Naming Service.
       * \param[in] obj     The object to bind.
       * \param[in] name    The stringified name to bind the object to.  This
       *                    name can contain multiple name components which
       *                    will be interpreted relative to the naming context
       *                    passed in the \a ns parameter.
       * \param[in] createPath If true, then any missing naming contexts will
       *                    be created.  If false, then a missing naming
       *                    context results in an exception.
       * \param[in] rebind  If true, then an existing binding is overwritten.
       *                    If false, an exception is thrown if the name
       *                    already exists.
       */

      void nameServiceBind (CosNaming::NamingContextExt_ptr ns,
			    CORBA::Object_ptr obj,
			    const std::string & name,
			    bool createPath = true,
			    bool rebind = true)
	throw (std::string);

    }
  }
}

#endif
