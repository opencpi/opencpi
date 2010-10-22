
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

#include <OcpiUtilZipFs.h>
#include <OcpiUtilFileFs.h>
#include <OcpiOsFileSystem.h>
#include <iostream>
#include <iomanip>
#include <string>

int
main (int argc, char *argv[])
{
  if (argc != 2 && argc != 3 && argc != 4) {
    std::cout << "usage: " << argv[0] << " <zipfile> [<fileinzip> [<localname>]]" << std::endl;
    return 1;
  }

  OCPI::Util::FileFs::FileFs localFs ("/");
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

  OCPI::Util::Vfs::Iterator * contents = zipFs.list (zipFs.cwd());

  while (!contents->end()) {
    if (contents->isDirectory()) {
      std::cout << "     (dir)";
    }
    else {
      std::cout << std::setw (10) << contents->size();
    }

    std::cout << "    "
              << contents->relativeName ()
              << std::endl;

    contents->next ();
  }

  std::cout << std::endl;
  zipFs.closeIterator (contents);
  
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
