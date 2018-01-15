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

/*
 * This file is the Application Control Interface API Hardware testbench for the
 * Zipper I2C component. This file works with HDL implementations only.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "OcpiApi.h"
#include <sstream>
#include <iostream>

namespace OA = OCPI::API;

int main(int argc, char **argv) {
  try {
    bool pass;
  std::string name, value;
  std::string xml_name = "";

  //Name of Application XML used for testbench
  xml_name = "./hw_testbench_app_file.xml";
  printf("Application XML used for testbench: %s\n", xml_name.c_str());
  OA::Application app(xml_name.c_str(), NULL);

  printf("Start of Testbench\n");  
  app.initialize();
  pass =true;

  //TEST SI5351
  printf("Set Sampling Clock to 1 MHz (500 kSps): \n");
  app.setProperty("clock_gen","channels", 
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 1e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 1e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 1e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 1e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}");
  sleep(2);

  printf("Set Sampling Clock to 10 MHz (5 MSps): \n");
  app.setProperty("clock_gen","channels", 
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 10e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 10e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 10e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 10e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}");
  sleep(2);

  printf("Set Sampling Clock to 80 MHz (40 MSps): \n");
  app.setProperty("clock_gen","channels", 
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 80e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 80e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 80e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 80e6,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},"
		  "{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}");
  sleep(2);


  app.start();
  sleep(2);
  app.stop();

    if(pass){
      printf("I2C HW Testbench: Passed.\n");
    }

  return 1;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
}
