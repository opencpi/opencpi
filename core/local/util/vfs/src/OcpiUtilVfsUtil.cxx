
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

#include <OcpiUtilVfsUtil.h>
#include <OcpiUtilVfs.h>
#include <OcpiUtilVfsIterator.h>
#include <OcpiUtilMisc.h>
#include <istream>
#include <string>

/*
 * ----------------------------------------------------------------------
 * isXMLDocument()
 * ----------------------------------------------------------------------
 */

bool
OCPI::Util::Vfs::
isXMLDocument (Vfs * fs, const std::string & fileName)
  throw (std::string)
{
  /*
   * Ask OCPI::Util::Misc::isXMLDocument. Have to open file first.
   */

  bool result;

  try {
    std::istream * istr = fs->openReadonly (fileName);

    try {
      result = OCPI::Util::Misc::isXMLDocument (istr);
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
OCPI::Util::Vfs::
copyFilesRecursively (Vfs * srcFs, const std::string & srcDir,
                      Vfs * destFs, const std::string & destDir,
                      const std::string & pattern)
  throw (std::string)
{
  if (OCPI::Util::Vfs::relativeName (pattern) != pattern) {
    throw std::string ("pattern must not contain directory separator");
  }

  /*
   * Iterate over all subdirectories
   */

  OCPI::Util::Vfs::Iterator * it = srcFs->list (srcDir);

  try {
    while (!it->end()) {
      if (it->isDirectory()) {
        std::string newSrcDir =
          OCPI::Util::Vfs::joinNames (srcDir, it->relativeName());
        std::string newDestDir =
          OCPI::Util::Vfs::joinNames (destDir, it->relativeName());
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
          OCPI::Util::Vfs::joinNames (destDir, it->relativeName());
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
OCPI::Util::Vfs::
copyDirectoryRecursively (Vfs * srcFs, const std::string & srcName,
                          Vfs * destFs, const std::string & destName,
                          const std::string & pattern)
  throw (std::string)
{
  destFs->mkdir (destName);
  copyFilesRecursively (srcFs, srcName, destFs, destName, pattern);
}

void
OCPI::Util::Vfs::
removeFilesRecursively (Vfs * fs,
                        const std::string & dir,
                        const std::string & pattern,
                        bool deleteEmptyDirectories)
  throw (std::string)
{
  if (OCPI::Util::Vfs::relativeName (pattern) != pattern) {
    throw std::string ("pattern must not contain directory separator");
  }

  /*
   * Iterate over all subdirectories
   */

  OCPI::Util::Vfs::Iterator * it = fs->list (dir);

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
          OCPI::Util::Vfs::Iterator * it2 =
            fs->list (it->absoluteName());

          try {
            it2->end ();
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
OCPI::Util::Vfs::
removeDirectoryRecursively (Vfs * fs,
                            const std::string & dir,
                            const std::string & pattern,
                            bool deleteEmptyDirectories)
  throw (std::string)
{
  removeFilesRecursively (fs, dir, pattern, deleteEmptyDirectories);

  if (deleteEmptyDirectories) {
    OCPI::Util::Vfs::Iterator * it = fs->list (dir);

    try {
      it->end ();
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

OCPI::Util::Vfs::EventualEraser::
EventualEraser ()
  throw ()
  : m_fs (0)
{
}

OCPI::Util::Vfs::EventualEraser::
EventualEraser (Vfs * fs, const std::string & name, bool isFile)
  throw ()
  : m_fs (fs),
    m_name (name),
    m_isFile (isFile)
{
}

OCPI::Util::Vfs::EventualEraser::
~EventualEraser ()
  throw ()
{
  if (m_fs) {
    try {
      if (m_isFile) {
        m_fs->remove (m_name);
      }
      else {
        OCPI::Util::Vfs::removeDirectoryRecursively (m_fs, m_name);
      }
    }
    catch (...) {
      // ignore any errors
    }
  }
}

void
OCPI::Util::Vfs::EventualEraser::
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
OCPI::Util::Vfs::EventualEraser::
cancel ()
  throw ()
{
  m_fs = 0;
}

