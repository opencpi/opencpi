
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

#include <OcpiUtilUri.h>
#include <OcpiUtilHttpFs.h>
#include <OcpiUtilTcpConnector.h>
#include <OcpiUtilFileFs.h>
#include <OcpiUtilUriFs.h>
#include <OcpiOsFileSystem.h>
#include <iostream>
#include <string>

int
main (int argc, char *argv[])
{
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <URI>" << std::endl;
    return 1;
  }

  std::string currentDir = OCPI::OS::FileSystem::cwd ();
  OCPI::Util::FileFs::FileFs localFs (currentDir);
  OCPI::Util::Http::HttpFs<OCPI::Util::Tcp::Connector> remoteFs;
  OCPI::Util::Vfs::UriFs ufs;

  ufs.mount (&localFs);
  ufs.mount (&remoteFs);

  OCPI::Util::Uri uri (argv[1]);
  std::string localName = uri.getFileName ();

  if (!localName.length()) {
    localName = "index.html";
  }

  std::string localURI = localFs.nameToURI (localName);
  std::string localAbsName = ufs.URIToName (localURI);
  std::string remoteAbsName = ufs.URIToName (uri.get());

  std::cout << "FileFs base URI is " << localFs.baseURI() << std::endl;
  std::cout << "HttpFs base URI is " << remoteFs.baseURI() << std::endl;

  std::cout << "Copying " << remoteAbsName
            << " to " << localAbsName
            << " ... " << std::flush;

  try {
    ufs.copy (remoteAbsName, &ufs, localAbsName);
    std::cout << "done." << std::endl;
  }
  catch (const std::string & err) {
    std::cout << " failed: " << err << std::endl;
  }

  return 0;
}
