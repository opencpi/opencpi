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
 * Matchstiq-Z1 I2C component. This file works with HDL implementations only.
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "OcpiApi.h"
#include <sstream>
#include <iostream>
#include <fstream>

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

  //Set up Si5338
  printf("Set Sampling Clock to 200 kHz (100 kSps): \n");
  app.setProperty("clock_gen","channels", "{output_hz 200e3,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}");
  app.start();
  usleep(30000);

  //Test PCA9535
  printf("PCA9535: Starting Test \n");
  printf("PCA9535: Testing filter bandwidth: \n");
  printf("PCA9535: Set unfiltered\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "0");
  usleep(30000);

  printf("PCA9535: Set filter bandwidth to 300 to 700 MHz\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "1");
  usleep(30000);

  printf("PCA9535: Set filter bandwidth to 625 to 1080 MHz\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "2");
  usleep(30000);

  printf("PCA9535: Set filter bandwidth to 1000 to 2100 MHz\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "3");
  usleep(30000);

  printf("PCA9535: Set filter bandwidth to 1700 to 2500 MHz\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "4");
  usleep(30000);

  printf("PCA9535: Set filter bandwidth to 2200 to 3800 MHz\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "5");
  usleep(30000);

  printf("PCA9535: Set filter bandwidth to unfiltered\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","filter_bandwidth", "0");
  usleep(100000);

  printf("PCA9535: Testing Lime RX input:\n");
  printf("PCA9535: Set Lime RX input to 2\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","lime_rx_input", "2");
  usleep(30000);

  printf("PCA9535: Set Lime RX input to 3\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","lime_rx_input", "3");
  usleep(30000);

  //Test LNA
  printf("PCA9535: Testing Pre-lime LNA:\n");
  printf ("PCA9535: Setting Pre-lime LNA off\n");
  app.setProperty("matchstiq_z1_pca9535_proxy","frontLNA_enable", "false");
  usleep(30000);
  printf ("PCA9535: Setting Pre-lime LNA on \n");
  app.setProperty("matchstiq_z1_pca9535_proxy","frontLNA_enable", "true");
  usleep(30000);

  printf("PCA9535: End of Test \n");

  //Matchstiq-Z1 AVR test
  printf("Matchstiq-Z1 AVR: Starting Test \n");

  std::ostringstream strs;
  std::string str; 

  printf("Matchstiq-Z1 AVR: Testing attenuator:\n");
  for(int i = 0; i <= 63; i++)
    {
      strs << i/2.0;
      str = strs.str();
      //printf ("Matchstiq-Z1 AVR: Setting attenuator to %.1f\n", i/2.0);
      app.setProperty("matchstiq_z1_avr_proxy","attenuation", str.c_str());
      strs.clear();
      strs.str("");
      usleep(30000);
    }

  printf("Matchstiq-Z1 AVR: Reset attenuator to 0:\n");
  strs << 0.0;
  str = strs.str();
  app.setProperty("matchstiq_z1_avr_proxy","attenuation", str.c_str());

  printf("Matchstiq-Z1 AVR: Testing LED:\n");

  printf("Matchstiq-Z1 AVR: Set LED off\n");
  app.setProperty("matchstiq_z1_avr_proxy","led", "off");
  sleep(1);

  printf("Matchstiq-Z1 AVR: Set LED green\n");
  app.setProperty("matchstiq_z1_avr_proxy","led", "green");
  sleep(1);

  printf("Matchstiq-Z1 AVR: Set LED red\n");
  app.setProperty("matchstiq_z1_avr_proxy","led", "red");
  sleep(1);

  printf("Matchstiq-Z1 AVR: Set LED orange\n");
  app.setProperty("matchstiq_z1_avr_proxy","led", "orange");
  sleep(1);

  printf("Matchstiq-Z1 AVR: Testing Serial Number:\n");
  app.getProperty("matchstiq_z1_avr_proxy","serial_num", value);
  printf("Matchstiq-Z1 AVR: Serial number is: %s\n", value.c_str());
  if(value.compare("6026")!=0)
    pass=false;

  printf("Matchstiq-Z1 AVR: Testing WARP voltage register: \n");
  printf("Matchstiq-Z1 AVR: Set WARP voltage to 2048\n");
  app.setProperty("matchstiq_z1_avr_proxy","warp_voltage", "2048");

  printf("Matchstiq-Z1 AVR: End of Test \n");

  printf("TMP100: Starting Test \n");
  printf("TMP100: Testing temperature:\n");
  app.getProperty("tmp100_proxy","temperature", value);
  printf("TMP100: Temperature is: %s degrees C\n", value.c_str());
  printf("TMP100: End of Test \n");

  sleep(1);

  app.stop();

    if(pass){
      printf("I2C HW Testbench: Passed.\n");
      std::ofstream outfile ("i2c_hw_testbench.results");
      outfile << "PASSED" << std::endl;
      outfile.close();
    }

  return 1;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
}
