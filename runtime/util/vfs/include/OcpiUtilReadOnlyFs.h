
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

#ifndef OCPIUTILREADONLYFS_H__
#define OCPIUTILREADONLYFS_H__

/**
 * \file
 * \brief Denies write access to files in a file system.
 *
 * Revision History:
 *
 *     04/19/2005 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <OcpiUtilFilterFs.h>

namespace OCPI {
  namespace Util {
    namespace Vfs {

      /**
       * \brief Denies write access to files in a file system.
       *
       * This class implements the OCPI::Util::Vfs::Vfs interface.
       *
       * All read access is delegated to a secondary OCPI::Util::Vfs::Vfs.
       * Any write access is denied.
       */

      class ReadOnlyFs : public OCPI::Util::Vfs::FilterFs {
      public:
        /**
         * Constructor
         *
         * \param[in] delegatee The secondary Vfs instance to delegate all
         *                      read access to.
         */

        ReadOnlyFs (OCPI::Util::Vfs::Vfs & delegatee)
          throw ();

        /**
         * Destructor.
         */

        ~ReadOnlyFs ()
          throw ();

      protected:
        /**
         * \name Implementation of the OCPI::Util::Vfs::FilterFs interface.
         */

        //@{

        void access (const std::string & name,
                     std::ios_base::openmode mode,
                     bool isDirectory)
          throw (std::string);

        //@}
      };

    }
  }
}

#endif
