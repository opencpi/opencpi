
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

#ifndef OCPIOSLOADABLEMODULE_H__
#define OCPIOSLOADABLEMODULE_H__

/**
 * \file
 * \brief Dynamically load modules (a.k.a. shared libraries or DLLs).
 *
 * Revision History:
 *
 *     06/30/2005 - Frank Pilhofer
 *                  Use 64-bit type for our opaque data, to ensure
 *                  alignment.
 *                  Renamed "LoadableModule" (was: SharedLibrary).
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiOsDataTypes.h>
#include <string>

namespace OCPI {
  namespace OS {

    /**
     * \brief Dynamically load modules (a.k.a. shared libraries or DLLs).
     *
     * Dynamically load loadable modules (also known as shared libraries
     * or dynamic link libraries "DLLs") and locate exported code or data
     * symbols within them.
     */

    class LoadableModule {
    public:
      /**
       * Constructor: Initializes the instance, but does not open a
       *              loadable module.
       */

      LoadableModule ()
        throw ();

      /**
       * Constructor: Initializes the instance, then calls
       * #open (\a fileName).
       *
       * \param[in] fileName The native file name of the module to load.
       *
       * \throw std::string See open().
       */

      LoadableModule (const std::string & fileName)
        throw (std::string);

      /**
       * Destructor.
       *
       * \pre An open module must be closed before destruction.
       */

      ~LoadableModule ()
        throw ();

      /**
       * Load a loadable module.
       *
       * \param[in] fileName  The file name of a loadable module, according to
       *                local file name convention. The loadable module
       *                must be in a format that the local operating system
       *                considers appropriate.
       *
       * \pre No loadable module is open in this LoadableModule instance.
       * \throw std::string Operating system error loading the module.
       *
       * \note On VxWorks/DKM, unresolved symbols are logged to the console.
       *
       * \note On VxWorks/DKM, module dependencies are not supported.
       * Dependent modules must be explicitly loaded to avoid unresolved
       * symbols.
       */

      void open (const std::string & fileName)
        throw (std::string);

      /**
       * Locates a symbol within the open loadable module.
       *
       * \param[in] symbolName The name of the symbol.
       * \return          A pointer to the location of the
       *                  symbol within the loadable module. The pointer
       *                  can then be typecast to the expected type
       *                  (e.g., using static_cast<>()).
       *
       * \throw std::string if the symbol is not found in the module.
       *
       * \pre open() succeeded.
       *
       * \note Name mangling may apply to C++ functions that have not be
       * declared to be of C linkage (extern "C").
       */

      void * getSymbol (const std::string & symbolName)
        throw (std::string);

      /**
       * Closes the open loadable module.
       *
       * Releases resources associated with the previously-openend
       * loadable module. Invalidates all pointers that were returned
       * by getSymbol().
       *
       * \throw std::string Operating system error unloading the module.
       */

      void close ()
        throw (std::string);

    private:
      OCPI::OS::uint64_t m_osOpaque[1];

    private:
      /**
       * Not implemented.
       */

      LoadableModule (const LoadableModule &);

      /**
       * Not implemented.
       */

      LoadableModule & operator= (const LoadableModule &);
    };

  }
}

#endif
