#include <CpiUtilReadOnlyFs.h>

/*
 * ----------------------------------------------------------------------
 * ReadOnlyFS implementation
 * ----------------------------------------------------------------------
 */

CPI::Util::Vfs::ReadOnlyFs::ReadOnlyFs (CPI::Util::Vfs::Vfs & delegatee)
  throw ()
  : FilterFs (delegatee)
{
}

CPI::Util::Vfs::ReadOnlyFs::~ReadOnlyFs ()
  throw ()
{
}

void
CPI::Util::Vfs::ReadOnlyFs::access (const std::string &,
                                    std::ios_base::openmode mode,
                                    bool)
  throw (std::string)
{
  if ((mode & std::ios_base::out) || (mode & std::ios_base::trunc)) {
    throw std::string ("readonly file system");
  }
}
