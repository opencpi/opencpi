// -*- c++ -*-

#ifndef CPIUTILLOADABLEMODULE_H__
#define CPIUTILLOADABLEMODULE_H__

/**
 * \file
 * \brief Dynamically load modules from a VFS.
 *
 * Revision History:
 *
 *     04/07/2009 - Frank Pilhofer
 *                  Allow to set the directory that loadable modules are
 *                  copied into for loading.
 *
 *     09/04/2008 - Frank Pilhofer
 *                  Initial version.
 *
 */

#include <CpiOsLoadableModule.h>
#include <CpiUtilVfs.h>
#include <string>

namespace CPI {
  namespace Util {

    /**
     * \brief Dynamically load modules from a VFS.
     *
     * Dynamically load loadable modules (also known as shared libraries
     * or dynamic link libraries "DLLs") and locate exported code or data
     * symbols within them.
     *
     * \note Uses CPI::OS::LoadableModule.
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
       * #open (\a vfs, \a fileName).
       *
       * \param[in] vfs The file system to load a module from.
       * \param[in] fileName The absolute file name of the module to load.
       *
       * \throw std::string See open().
       */

      LoadableModule (CPI::Util::Vfs::Vfs * vfs,
                      const std::string & fileName)
        throw (std::string);

      /**
       * Destructor.
       *
       * Calls close() if a module is loaded.
       */

      ~LoadableModule ()
        throw ();

      /**
       * Load a loadable module.
       *
       * \param[in] vfs The file system to load the module from.
       * \param[in] fileName  The absolute file name of a loadable
       *                module within the file system identified by
       *                \a vfs.
       *
       * \pre No loadable module is open in this LoadableModule instance.
       * \throw std::string Operating system error loading the module.
       */

      void open (CPI::Util::Vfs::Vfs * vfs,
                 const std::string & fileName)
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

      /**
       * Set location for temporary files.
       *
       * If the OS does not support loading modules from memory, this
       * sets the location where they are stored on disk.  This function
       * is optional.  If not called, a default location is chosen.
       *
       * On an OS that does support loading modules from memory, this
       * function is a no-op.
       *
       * \param[in] dirName The name of a directory to use for downloaded
       *                    modules files.
       */

      static void setTemporaryFileLocation (const std::string & dirName)
        throw ();

    private:
      bool m_open;
      CPI::OS::LoadableModule m_lm;
      std::string m_fileName;

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
