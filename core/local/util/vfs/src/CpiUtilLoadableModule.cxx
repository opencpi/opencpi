/*
 * Revision History:
 *
 *     04/07/2009 - Frank Pilhofer
 *                  - Allow to set the directory that loadable modules are
 *                    copied into for loading.
 *                  - Bugfix: Don't delete loadable modules immediately after
 *                    loading them.  The debugger needs them.
 *                  - Bugfix: If loadable modules are placed in a separate
 *                    directory, don't mangle the file name.  Otherwise,
 *                    dependencies between shared libraries don't work.
 */

#include <CpiOsAssert.h>
#include <CpiOsMutex.h>
#include <CpiOsMisc.h>
#include <CpiOsLoadableModule.h>
#include <CpiOsFileSystem.h>
#include <CpiUtilAutoMutex.h>
#include <CpiUtilMisc.h>
#include <CpiUtilVfs.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string>
#include <cerrno>
#include <cstdlib>
#include "CpiUtilLoadableModule.h"

#if defined (_WRS_KERNEL)
#include <ioLib.h>
#endif

#if defined (WIN32)
#include <fcntl.h>
#include <io.h>
#define S_IRWXU 0777
#endif


namespace {
  static CPI::OS::Mutex g_loadableModuleMutex;
  static unsigned int g_loadableModuleNumber;
  static std::string g_loadableModulePrefix = "cpi_loadable_module_";
  static std::string g_loadableModuleLocation;

  enum {
    DEFAULT_BUFFER_SIZE = 4096
  };
}

CPI::Util::LoadableModule::
LoadableModule ()
  throw ()
  : m_open (false)
{
}

CPI::Util::LoadableModule::
LoadableModule (CPI::Util::Vfs::Vfs * vfs,
                const std::string & fileName)
  throw (std::string)
  : m_open (false)
{
  open (vfs, fileName);
}

CPI::Util::LoadableModule::
~LoadableModule ()
  throw ()
{
  if (m_open) {
    try {
      close ();
    }
    catch (...) {
    }
  }
}

void
CPI::Util::LoadableModule::
open (CPI::Util::Vfs::Vfs * vfs,
      const std::string & fileName)
  throw (std::string)
{
  cpiAssert (!m_open);
  CPI::Util::AutoMutex lock (g_loadableModuleMutex);

  /*
   * We need to download the file to the local file system.  Find a nice
   * place for it.
   */

  if (!g_loadableModuleLocation.length()) {
    bool isdir=false, exists=false;
    const char * tmpDir;
    std::string ntd;

    if ((tmpDir = std::getenv ("TEMP"))) {
      try {
        ntd = CPI::OS::FileSystem::fromNativeName (tmpDir);
        exists = CPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
        exists = false;
      }
    }

    if ((!exists || !isdir) && (tmpDir = std::getenv ("TMP"))) {
      try {
        ntd = CPI::OS::FileSystem::fromNativeName (tmpDir);
        exists = CPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
        exists = false;
      }
    }

    if (!exists || !isdir) {
      try {
        ntd = CPI::OS::FileSystem::fromNativeName ("/tmp");
        exists = CPI::OS::FileSystem::exists (ntd, &isdir);
      }
      catch (...) {
        exists = false;
      }
    }

    if (!exists || !isdir) {
      throw std::string ("No temp directory found");
    }

    g_loadableModuleLocation = ntd;
  }

  /*
   * Compose a local file name for the DLL.
   */

  std::string relFileName;

  if (g_loadableModulePrefix.length()) {
    relFileName = g_loadableModulePrefix;
    relFileName += CPI::Util::Misc::unsignedToString (g_loadableModuleNumber++);
    relFileName += "_";
    relFileName += CPI::Util::Misc::unsignedToString (static_cast<unsigned int> (CPI::OS::getProcessId()));
    relFileName += "_";
    relFileName += CPI::Util::Vfs::relativeName (fileName);
  }
  else {
    relFileName = CPI::Util::Vfs::relativeName (fileName);
  }

  std::string absFileName = CPI::Util::Vfs::joinNames (g_loadableModuleLocation,
                                                       relFileName);
  m_fileName = CPI::OS::FileSystem::toNativeName (absFileName);

  /*
   * Can release the global mutex now.
   */

  lock.unlock ();

  /*
   * Now we can download the file.
   */

  std::istream * in = vfs->openReadonly (fileName, std::ios_base::binary);

  try {
    /*
     * Must use open instead of std::ofstream here, because the latter does
     * not allow setting the file mode, and Linux insists that the executable
     * bit be set on loadable modules.
     */

    int out = ::open (m_fileName.c_str(),
                      O_CREAT | O_WRONLY | O_TRUNC,
                      S_IRWXU);

    if (out < 0) {
      std::string oops = "Failed to open \"";
      oops += m_fileName;
      oops += "\" for writing: ";
      oops += CPI::Util::Misc::integerToString (errno);
      throw oops;
    }

    try {
      char buffer[DEFAULT_BUFFER_SIZE];

      while (!in->eof() && in->good()) {
        in->read (buffer, DEFAULT_BUFFER_SIZE);
        if (::write (out, buffer, in->gcount()) != in->gcount()) {
          break;
        }
      }

      if (!in->eof() && in->fail()) {
        std::string oops = "Error reading from \"";
        oops += fileName;
        oops += "\"";
        throw oops;
      }

      if (!in->eof()) {
        std::string oops = "Error writing to \"";
        oops += m_fileName;
        oops += "\"";
        throw oops;
      }
    }
    catch (const std::string &) {
      ::close (out);
      throw;
    }

    if (::close (out) != 0) {
      std::string oops = "Error flushing \"";
      oops += m_fileName;
      oops += "\"";
      throw oops;
    }
  }
  catch (const std::string &) {
    try {
      vfs->close (in);
    }
    catch (const std::string &) {
    }

    try {
      CPI::OS::FileSystem::remove (m_fileName);
    }
    catch (...) {
    }

    throw;
  }

  try {
    vfs->close (in);
  }
  catch (const std::string &) {
    try {
      CPI::OS::FileSystem::remove (m_fileName);
    }
    catch (...) {
    }

    throw;
  }

  /*
   * Looks like the download was successful. Now open the module.
   */

  try {
    m_lm.open (m_fileName);
  }
  catch (const std::string & oops) {
    try {
      CPI::OS::FileSystem::remove (m_fileName);
    }
    catch (...) {
    }

    std::string msg = "Error loading \"";
    msg += fileName;
    msg += "\": ";
    msg += oops;
    throw msg;
  }

  /*
   * Now we should be able to delete the file.  The OS will keep it until
   * the module is closed.  That way, the file will be gone even if we get
   * shut down.  If this doesn't work -- we'll just try again after closing
   * the module.
   *
   * Don't do that if we have a location to store loadable modules.  The
   * presence of shared library files is required (a) to satisfy shared
   * library dependencies and (b) for debugging, if someone attaches to
   * the process after the libraries are loaded.
   */

  if (g_loadableModulePrefix.length()) {
    try {
      CPI::OS::FileSystem::remove (m_fileName);
    }
    catch (...) {
    }
  }

  m_open = true;
}

void *
CPI::Util::LoadableModule::
getSymbol (const std::string & functionName)
  throw (std::string)
{
  cpiAssert (m_open);
  return m_lm.getSymbol (functionName);
}

void
CPI::Util::LoadableModule::
close ()
  throw (std::string)
{
  cpiAssert (m_open);
  m_open = false;

  try {
    m_lm.close ();
  }
  catch (...) {
    try {
      CPI::OS::FileSystem::remove (m_fileName);
    }
    catch (...) {
    }

    throw;
  }

  try {
    CPI::OS::FileSystem::remove (m_fileName);
  }
  catch (...) {
  }
}

void
CPI::Util::LoadableModule::
setTemporaryFileLocation (const std::string & dirName)
  throw ()
{
  CPI::Util::AutoMutex lock (g_loadableModuleMutex);
  g_loadableModuleLocation = dirName;
  g_loadableModulePrefix.clear ();
}
