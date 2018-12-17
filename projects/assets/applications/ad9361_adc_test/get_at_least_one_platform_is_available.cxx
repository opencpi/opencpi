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

#include <cstdlib>    // EXIT_SUCCESS, EXIT_FAILURE
#include <iostream>   // std::cout, std::cerr
#include "OcpiApi.hh" // OCPI::API namespace

namespace OA = OCPI::API;

int main(int argc, char **argv)
{
#define NUM_EXPECTED_PARAMETERS_PASSED_IN 1
  if(argc != NUM_EXPECTED_PARAMETERS_PASSED_IN+1)
  {
    std::cerr << "must pass desired platform as first argument, e.g. get_at_least_one_platform_is_available ml605\n";
    return EXIT_FAILURE;
  }

  try
  {
    OA::Container *container;

    for (unsigned n = 0; (container = OA::ContainerManager::get(n)); n++)
    {
      if(container->platform() == argv[NUM_EXPECTED_PARAMETERS_PASSED_IN])
      {
        std::cout << "true\n";
        return EXIT_SUCCESS;
      }
    }
  }
  catch (std::string &e)
  {
    std::cerr << e << "\n";
    return EXIT_FAILURE;
  }
  catch(...)
  {
    std::cerr << "unknown exeception occured\n";
    return EXIT_FAILURE;
  }
  std::cout << "false\n";
  return EXIT_SUCCESS;
}

