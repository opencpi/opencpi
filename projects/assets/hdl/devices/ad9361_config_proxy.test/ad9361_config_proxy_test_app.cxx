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

#include <iostream> //std::cout
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE
#include "OcpiApi.hh"

namespace OA = OCPI::API;

int main()
{
  std::string xml_name("ad9361_config_proxy_test_app.xml");
  bool debug_mode = false;

  try
  {
    printf("Application properties are found in XML file: %s\n", xml_name.c_str());

    OA::Application app(xml_name.c_str(), NULL);
    app.initialize();
    printf("App initialized.\n");

    app.start();
    printf("App started.\n");

    app.stop();
    printf("App stopped.\n");

    std::string value;
    app.getProperty("ad9361_config_proxy", "temperature", value);
    std::cout << "AD9361 temperature is: " << value << "\n";

    app.getProperty("ad9361_config_proxy", "temperature", value);
    std::cout << "AD9361 temperature is: " << value << "\n";

    app.getProperty("ad9361_config_proxy", "temperature", value);
    std::cout << "AD9361 temperature is: " << value << "\n";

    // dump all final property values
    if (debug_mode) {
      std::string name, value;
      bool isParameter, hex = false;
      fprintf(stderr, "Dump of all final property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter); n++)
      {
        if (!isParameter)
        {
          fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
        }
        else
        {
          fprintf(stderr, "Parameter %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
        }
      }
    }

  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

