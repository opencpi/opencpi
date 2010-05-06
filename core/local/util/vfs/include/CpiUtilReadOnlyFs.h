// -*- c++ -*-

#ifndef CPIUTILREADONLYFS_H__
#define CPIUTILREADONLYFS_H__

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

#include <CpiUtilFilterFs.h>

namespace CPI {
  namespace Util {
    namespace Vfs {

      /**
       * \brief Denies write access to files in a file system.
       *
       * This class implements the CPI::Util::Vfs::Vfs interface.
       *
       * All read access is delegated to a secondary CPI::Util::Vfs::Vfs.
       * Any write access is denied.
       */

      class ReadOnlyFs : public CPI::Util::Vfs::FilterFs {
      public:
        /**
         * Constructor
         *
         * \param[in] delegatee The secondary Vfs instance to delegate all
         *                      read access to.
         */

        ReadOnlyFs (CPI::Util::Vfs::Vfs & delegatee)
          throw ();

        /**
         * Destructor.
         */

        ~ReadOnlyFs ()
          throw ();

      protected:
        /**
         * \name Implementation of the CPI::Util::Vfs::FilterFs interface.
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
