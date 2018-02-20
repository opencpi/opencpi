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

#include <OcpiUtilVfsIterator.h>

namespace {
  bool
  glob (const std::string & str,
        const std::string & pat)
    throw ()
  {
    size_t strIdx = 0, strLen = str.length ();
    size_t patIdx = 0, patLen = pat.length ();
    const char * name = str.data ();
    const char * pattern = pat.data ();

    while (strIdx < strLen && patIdx < patLen) {
      if (*pattern == '*') {
        pattern++;
        patIdx++;
        while (strIdx < strLen) {
          if (glob (name, pattern)) {
            return true;
          }
          strIdx++;
          name++;
        }
        return (patIdx < patLen) ? false : true;
      }
      else if (*pattern == '?' || *pattern == *name) {
        pattern++;
        patIdx++;
        name++;
        strIdx++;
      }
      else {
        return false;
      }
    }
    
    while (*pattern == '*' && patIdx < patLen) {
      pattern++;
      patIdx++;
    }
    
    if (patIdx < patLen || strIdx < strLen) {
      return false;
    }
    
    return true;
  }

}

namespace OCPI { namespace Util { namespace Vfs {

Iterator::Iterator(Vfs &fs, const std::string &dir, const char *pattern, bool recursive)
  throw (std::string)
  : m_fs(fs), m_dirName(dir), m_pattern(pattern), m_recursive(recursive), m_dir(&fs.openDir(dir)),
    m_skipLength(dir == "/" ? 1 : dir.length() + 1)
{
}

Iterator::~Iterator () throw ()
{
  delete m_dir;
}

bool Iterator::next (std::string &str, bool &isDir) throw (std::string) {
  do {
    std::string entry;
    if (m_dir->next(entry, isDir)) {
      if (m_recursive && isDir)
	m_dir = m_dir->pushDir(joinNames(m_dir->m_name, entry));
      else if (glob(entry, m_pattern)) {
	str = joinNames(m_dir->m_name, entry).substr(m_skipLength);
	return true;
      }
    } else 
      m_dir = m_dir->popDir();
  } while (m_dir);
  return false;
}

    }
  }
}
