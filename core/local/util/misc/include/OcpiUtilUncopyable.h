
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

