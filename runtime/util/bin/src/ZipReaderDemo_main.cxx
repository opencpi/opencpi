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
#include <OcpiUtilZipFs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiOsFileSystem.h>
#include <iostream>
#include <iomanip>
#include <string>
#include <memory>

int
main (int argc, char *argv[])
{
  if (argc != 2 && argc != 3 && argc != 4) {
    std::cout << "usage: " << argv[0] << " <zipfile> [<fileinzip> [<localname>]]" << std::endl;
    return 1;
  }

  OCPI::Util::FileFs localFs ("/");
  std::string zipName = OCPI::OS::FileSystem::fromNativeName (argv[1]);
  std::string absZipName = OCPI::OS::FileSystem::absoluteName (zipName);
  OCPI::Util::ZipFs::ZipFs zipFs;

  try {
    zipFs.openZip (&localFs, absZipName, std::ios_base::in);
  }
  catch (const std::string & oops) {
    std::cout << "error: " << oops << std::endl;
    return 1;
  }

  std::cout << "Zip File Contents" << std::endl;
  std::cout << "-----------------" << std::endl;

  std::auto_ptr<OCPI::Util::Vfs::Iterator> contents(zipFs.list (zipFs.cwd()));

  std::string name;
  bool isDir;
  while (contents->next(name, isDir)) {
    if (isDir) {
      std::cout << "     (dir)";
    }
    else {
      std::cout << std::setw (10) << zipFs.size(name);
    }

    std::cout << "    "
              << name
              << std::endl;

  }

  std::cout << std::endl;
  
  if (argc == 3 || argc == 4) {
    std::string localNativeName;

    if (argc == 3) {
      localNativeName = argv[2];
    }
    else {
      localNativeName = argv[3];
    }

    std::string localName =
      OCPI::OS::FileSystem::fromNativeName (localNativeName);
    std::string absLocalName =
      OCPI::OS::FileSystem::absoluteName (localName);

    std::cout << "Copying " << argv[2]
              << " from Zip file to " << absLocalName
              << " ... " << std::flush;

    try {
      zipFs.copy (argv[2], &localFs, absLocalName);
      std::cout << "done." << std::endl;
    }
    catch (const std::string & err) {
      std::cout << " failed: " << err << std::endl;
    }
  }

  return 0;
}
