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
#include <iostream>   // std::cout
#include "OcpiApi.hh" // OCPI::API namespace

namespace OA = OCPI::API;

int main()
{
  try
  {
    OA::Container *container;

    bool already_found_at_least_one_HDL_container = false;
    for (unsigned n = 0; (container = OA::ContainerManager::get(n)); n++)
    {
      if(container->model() == "hdl")
      {
        if(already_found_at_least_one_HDL_container)
        {
          std::cout << ",";
        }
        std::cout << container->platform();
        already_found_at_least_one_HDL_container = true;
      }
    }
    std::cout << "\n";
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
  return EXIT_SUCCESS;
}

