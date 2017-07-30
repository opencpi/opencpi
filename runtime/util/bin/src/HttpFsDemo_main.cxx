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

  OCPI::Util::FileFs localFs;
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
