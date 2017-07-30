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
#include <OcpiUtilHttpClient.h>
#include <OcpiUtilTcpConnector.h>
#include <iostream>
#include <string>
#include <fstream>

int
main (int argc, char *argv[])
{
  if (argc != 2) {
    std::cout << "usage: " << argv[0] << " <URI>" << std::endl;
    return 1;
  }

  OCPI::Util::Uri uri (argv[1]);
  OCPI::Util::Http::Client<OCPI::Util::Tcp::Connector> stream;

 again:
  try {
    stream.get (uri);
  }
  catch (const OCPI::Util::Http::Redirection & ex) {
    std::cout << "request redirected to "
              << ex.newLocation
              << std::endl;
    uri = ex.newLocation;
    goto again;
  }
  catch (const OCPI::Util::Http::HttpError & ex) {
    std::cout << "server status "
              << ex.statusCode << " "
              << ex.reasonPhrase
              << std::endl;
    return 1;
  }
  catch (const std::string & ex) {
    std::cout << "error: " << ex << std::endl;
    return 1;
  }

  if (!stream.good()) {
    std::cout << "oops, stream is no good." << std::endl;
    return 1;
  }

  OCPI::Util::Http::Headers::const_iterator it;
  for (it = stream.responseHeaders().begin();
       it != stream.responseHeaders().end(); it++) {
    std::cout << "    " << (*it).first
              << ": " << (*it).second
              << std::endl;
  }

  std::string filename = uri.getFileName ();

  if (!filename.length()) {
    filename = "index.html";
  }

  std::ofstream out (filename.c_str(), std::ios_base::binary);

  if (!out.good()) {
    std::cout << "error: cannot open "
              << filename
              << " for output"
              << std::endl;
    return 1;
  }

  std::cout << "downloading "
            << uri.get()
            << " as "
            << filename
            << " "
            << std::flush;

  char buffer[10240];
  while (stream.good() && !stream.eof() && out.good()) {
    stream.read (buffer, 10240);
    out.write (buffer, stream.gcount());
    std::cout << "." << std::flush;
  }

  if (!stream.eof() && !stream.good()) {
    std::cout << " error." << std::endl;
  }
  else if (!out.good()) {
    std::cout << " error." << std::endl;
  }
  else {
    std::cout << " done." << std::endl;
  }

  stream.close ();
  out.close ();
  return 0;
}
