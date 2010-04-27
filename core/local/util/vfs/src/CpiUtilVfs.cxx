#include <CpiUtilVfs.h>
#include <CpiOsFileSystem.h>
#include <iostream>
#include <string>

/*
 * Default buffer size for copying
 */

namespace {
  enum {
    DEFAULT_BUFFER_SIZE = 16384
  };
}

/*
 * ----------------------------------------------------------------------
 * File Name Helpers
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Vfs::joinNames (const std::string & dir,
			   const std::string & name)
  throw (std::string)
{
  return CPI::OS::FileSystem::joinNames (dir, name);
}

std::string
CPI::Util::Vfs::directoryName (const std::string & name)
  throw (std::string)
{
  std::string::size_type slashPos = name.rfind ('/');

  if (slashPos == std::string::npos) {
    throw std::string ("single path component");
  }

  if (slashPos == 0) {
    return "/";
  }

  return name.substr (0, slashPos);
}

std::string
CPI::Util::Vfs::relativeName (const std::string & name)
  throw (std::string)
{
  return CPI::OS::FileSystem::relativeName (name);
}

/*
 * ----------------------------------------------------------------------
 * Vfs Constructor and Destructor
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::Vfs::Vfs ()
  throw ()
{
}

CPI::Util::Vfs::Vfs::~Vfs ()
  throw ()
{
}

/*
 * ----------------------------------------------------------------------
 * File Name Mapping
 * ----------------------------------------------------------------------
 */

std::string
CPI::Util::Vfs::Vfs::absoluteName (const std::string & name) const
  throw (std::string)
{
  return CPI::Util::Vfs::joinNames (cwd(), name);
}

std::string
CPI::Util::Vfs::Vfs::directoryName (const std::string & name) const
  throw (std::string)
{
  return CPI::Util::Vfs::directoryName (absoluteName (name));
}

/*
 * ----------------------------------------------------------------------
 * File System Operations
 * ----------------------------------------------------------------------
 */

/*
 * The default implementation of copy copies the file, chunk by chunk
 */

void
CPI::Util::Vfs::Vfs::copy (const std::string & src,
			   Vfs * destfs,
			   const std::string & dest)
  throw (std::string)
{
  std::istream * srcStream = openReadonly (src, std::ios_base::binary);
  std::ostream * destStream;

  try {
    destStream = destfs->openWriteonly (dest, std::ios_base::trunc | std::ios_base::binary);
  }
  catch (...) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    throw;
  }

  try {
    char buffer[DEFAULT_BUFFER_SIZE];

    while (!srcStream->eof() && srcStream->good() && destStream->good()) {
      srcStream->read (buffer, DEFAULT_BUFFER_SIZE);
      destStream->write (buffer, srcStream->gcount());
    }
  }
  catch (...) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw;
  }

  if (srcStream->bad()) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw std::string ("error reading source file");
  }

  if (!destStream->good()) {
    try {
      close (srcStream);
    }
    catch (...) {
    }

    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw std::string ("error writing target file");
  }

  try {
    close (srcStream);
  }
  catch (...) {
    try {
      destfs->close (destStream);
    }
    catch (...) {
    }

    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw;
  }

  try {
    destfs->close (destStream);
  }
  catch (...) {
    try {
      destfs->remove (dest);
    }
    catch (...) {
    }

    throw;
  }
}

/*
 * Default implementation of rename
 */

void
CPI::Util::Vfs::Vfs::rename (const std::string & src,
			     const std::string & dest)
  throw (std::string)
{
  copy (src, this, dest);
  remove (src);
}

/*
 * The default implementation of move is naive.
 */

void
CPI::Util::Vfs::Vfs::move (const std::string & src,
			   Vfs * destfs,
			   const std::string & dest)
  throw (std::string)
{
  if (this == destfs) {
    rename (src, dest);
  }
  else {
    copy (src, destfs, dest);
    remove (src);
  }
}
