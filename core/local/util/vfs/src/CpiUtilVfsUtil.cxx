#include <CpiUtilVfsUtil.h>
#include <CpiUtilVfs.h>
#include <CpiUtilVfsIterator.h>
#include <CpiUtilMisc.h>
#include <istream>
#include <string>

/*
 * ----------------------------------------------------------------------
 * isXMLDocument()
 * ----------------------------------------------------------------------
 */

bool
CPI::Util::Vfs::
isXMLDocument (Vfs * fs, const std::string & fileName)
  throw (std::string)
{
  /*
   * Ask CPI::Util::Misc::isXMLDocument. Have to open file first.
   */

  bool result;

  try {
    std::istream * istr = fs->openReadonly (fileName);

    try {
      result = CPI::Util::Misc::isXMLDocument (istr);
    }
    catch (...) {
      try {
        fs->close (istr);
      }
      catch (...) {
      }
      throw;
    }

    fs->close (istr);
  }
  catch (const std::string & oops) {
    std::string reason = "Cannot determine file type: ";
    reason += oops;
    throw reason;
  }

  return result;
}

/*
 * ----------------------------------------------------------------------
 * Recursive Copying and Deleting
 * ----------------------------------------------------------------------
 */

void
CPI::Util::Vfs::
copyFilesRecursively (Vfs * srcFs, const std::string & srcDir,
                      Vfs * destFs, const std::string & destDir,
                      const std::string & pattern)
  throw (std::string)
{
  if (CPI::Util::Vfs::relativeName (pattern) != pattern) {
    throw std::string ("pattern must not contain directory separator");
  }

  /*
   * Iterate over all subdirectories
   */

  CPI::Util::Vfs::Iterator * it = srcFs->list (srcDir);

  try {
    while (!it->end()) {
      if (it->isDirectory()) {
        std::string newSrcDir =
          CPI::Util::Vfs::joinNames (srcDir, it->relativeName());
        std::string newDestDir =
          CPI::Util::Vfs::joinNames (destDir, it->relativeName());
        copyDirectoryRecursively (srcFs, newSrcDir,
                                  destFs, newDestDir,
                                  pattern);
      }
      it->next ();
    }
  }
  catch (...) {
    srcFs->closeIterator (it);
    throw;
  }

  srcFs->closeIterator (it);

  /*
   * Copy all matching files
   */

  it = srcFs->list (srcDir, pattern);

  try {
    while (!it->end()) {
      if (!it->isDirectory()) {
        std::string destFileName =
          CPI::Util::Vfs::joinNames (destDir, it->relativeName());
        srcFs->copy (it->absoluteName(), destFs, destFileName);
      }
      it->next ();
    }
  }
  catch (...) {
    srcFs->closeIterator (it);
    throw;
  }

  srcFs->closeIterator (it);
}

void
CPI::Util::Vfs::
copyDirectoryRecursively (Vfs * srcFs, const std::string & srcName,
                          Vfs * destFs, const std::string & destName,
                          const std::string & pattern)
  throw (std::string)
{
  destFs->mkdir (destName);
  copyFilesRecursively (srcFs, srcName, destFs, destName, pattern);
}

void
CPI::Util::Vfs::
removeFilesRecursively (Vfs * fs,
                        const std::string & dir,
                        const std::string & pattern,
                        bool deleteEmptyDirectories)
  throw (std::string)
{
  if (CPI::Util::Vfs::relativeName (pattern) != pattern) {
    throw std::string ("pattern must not contain directory separator");
  }

  /*
   * Iterate over all subdirectories
   */

  CPI::Util::Vfs::Iterator * it = fs->list (dir);

  try {
    while (!it->end()) {
      if (it->isDirectory()) {
        removeFilesRecursively (fs, it->absoluteName(), pattern,
                                deleteEmptyDirectories);

        /*
         * If deleteEmptyDirectories is true, see if that directory
         * is now empty. If yes, delete it.
         */

        if (deleteEmptyDirectories) {
          CPI::Util::Vfs::Iterator * it2 =
            fs->list (it->absoluteName());

          bool isempty;

          try {
            isempty = it2->end ();
          }
          catch (...) {
            fs->closeIterator (it2);
            throw;
          }

          fs->closeIterator (it2);
          fs->rmdir (it->absoluteName());
        }
      }
      it->next ();
    }
  }
  catch (...) {
    fs->closeIterator (it);
    throw;
  }

  fs->closeIterator (it);

  /*
   * Delete all matching files
   */

  it = fs->list (dir, pattern);

  try {
    while (!it->end()) {
      if (!it->isDirectory()) {
        fs->remove (it->absoluteName());
      }
      it->next ();
    }
  }
  catch (...) {
    fs->closeIterator (it);
    throw;
  }

  fs->closeIterator (it);
}

void
CPI::Util::Vfs::
removeDirectoryRecursively (Vfs * fs,
                            const std::string & dir,
                            const std::string & pattern,
                            bool deleteEmptyDirectories)
  throw (std::string)
{
  removeFilesRecursively (fs, dir, pattern, deleteEmptyDirectories);

  if (deleteEmptyDirectories) {
    CPI::Util::Vfs::Iterator * it = fs->list (dir);
    bool isempty;

    try {
      isempty = it->end ();
    }
    catch (...) {
      fs->closeIterator (it);
      throw;
    }

    fs->closeIterator (it);
    fs->rmdir (dir);
  }
}

/*
 * ----------------------------------------------------------------------
 * EventualEraser
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::EventualEraser::
EventualEraser ()
  throw ()
  : m_fs (0)
{
}

CPI::Util::Vfs::EventualEraser::
EventualEraser (Vfs * fs, const std::string & name, bool isFile)
  throw ()
  : m_fs (fs),
    m_name (name),
    m_isFile (isFile)
{
}

CPI::Util::Vfs::EventualEraser::
~EventualEraser ()
  throw ()
{
  if (m_fs) {
    try {
      if (m_isFile) {
        m_fs->remove (m_name);
      }
      else {
        CPI::Util::Vfs::removeDirectoryRecursively (m_fs, m_name);
      }
    }
    catch (...) {
      // ignore any errors
    }
  }
}

void
CPI::Util::Vfs::EventualEraser::
eventuallyErase (Vfs * fs, const std::string & name, bool isFile)
  throw (std::string)
{
  if (m_fs) {
    throw std::string ("already in use");
  }

  m_fs = fs;
  m_name = name;
  m_isFile = isFile;
}

void
CPI::Util::Vfs::EventualEraser::
cancel ()
  throw ()
{
  m_fs = 0;
}

