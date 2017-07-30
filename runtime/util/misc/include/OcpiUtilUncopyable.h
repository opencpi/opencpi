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

/**
  @file

  @brief
    Provides a mix-in to make a class Uncopyable.

  Revision History:

    10/13/2008 - Michael Pepe
                 Initial version.

************************************************************************** */


#ifndef INCLUDED_OCPI_UTIL_UNCOPYABLE_H
#define INCLUDED_OCPI_UTIL_UNCOPYABLE_H

namespace OCPI
{
  namespace Util
  {
    /**
      @class Uncopyable

      @brief
        Base class designed to prevent copying.

      Private inheritance from the Uncopyable class disallows copy
      construction or copy assignment.

    ********************************************************************* */

    class Uncopyable
    {
      protected:

        Uncopyable ( )
        throw ()
        {
          // Empty
        }

        ~Uncopyable ( )
        throw ()
        {
          // Empty
        }

      private:

        //! Not implemented
        Uncopyable ( const Uncopyable& );

        //! Not implemented
        const Uncopyable& operator= ( const Uncopyable& );
    };

  } // End: namespace Util

} // End: namespace OCPI

#endif // End: #ifndef INCLUDED_OCPI_UTIL_UNCOPYABLE_H

