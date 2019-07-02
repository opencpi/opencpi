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

#include <cstdio>   // printf()
#include <cstdlib>  // EXIT_SUCCESS, EXIT_FAILURE

#include "test_parsers.h"                 // did_pass_test_no_ocpi_app_parsers()
#include "test_app_defaults.h"            // did_pass_test_ocpi_app_default_values()
#include "test_app_mins.h"                // did_pass_test_ocpi_app_min_values()
#include "test_app_maxes.h"               // did_pass_test_ocpi_app_max_values()
#include "test_app_delays.h"              // did_pass_test_ocpi_app_AD9361_...() functions
#include "test_app_rx_value_corruption.h" // did_pass_test_ocpi_app_rx_value_corruption()

int main(int argc, char **argv)
{
  bool swonly = false;
  if (argc > 1)
  {
    std::string arg_swonly(argv[1]);
    if(arg_swonly == "swonly")
    {
      swonly = true;
    }
    else
    {
      printf("invalid argument: %s (must either be 'swonly' or nothing')", argv[1]);
      goto failed;
    }
  }
  if (argc > 2)
  {
    printf("too many arguments, (must either have no arguments or single argument 'swonly'')");
    goto failed;
  }

  if(!did_pass_test_no_ocpi_app_parsers())                        { goto failed; }
  
  if(not swonly)
  {
    if(!did_pass_test_ocpi_app_default_values())                   { goto failed; }
    if(!did_pass_test_ocpi_app_min_values())                       { goto failed; }
    if(!did_pass_test_ocpi_app_max_values())                       { goto failed; }
    if(!did_pass_test_ocpi_app_AD9361_FB_CLK_Delay_enforcement())  { goto failed; }
    if(!did_pass_test_ocpi_app_AD9361_Tx_Data_Delay_enforcement()) { goto failed; }
    if(!did_pass_test_ocpi_app_rx_value_corruption())              { goto failed; }
  }
 
  printf("PASSED\n");
  return EXIT_SUCCESS;

  failed:
  printf("FAILED\n");
  return EXIT_FAILURE;
}

