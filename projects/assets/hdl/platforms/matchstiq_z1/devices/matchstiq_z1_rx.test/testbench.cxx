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

#include <stdio.h>
#include <unistd.h>
//#include <cassert>
#include <string.h>
#include "OcpiApi.h"
#include <iostream>
#include <sstream>

namespace OA = OCPI::API;
using namespace std;


int main(int argc, char **argv) {
  try {
  std::string name, value;
  std::string xml_name = "";
  char cin_value;

  xml_name = "testbench_app_file.xml";
  printf("%s\n", xml_name.c_str());
  OA::Application app(xml_name.c_str(), NULL);
  app.initialize();

  // bool isParameter;
  // printf("Dump of all initial property values:\n");
  // for (unsigned n = 0; app.getProperty(n, name, value, false, &isParameter); n++)
  // printf("Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
  //    isParameter ? " (parameter)" : "");

  // set sample rate
  app.setProperty("rx","sample_rate_MHz", "0.200");
  cout << "Set sig gen: freq=300.005 MHz, amp=-55 dBm" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;

  // set sample rate
  app.setProperty("rx","sample_rate_MHz", "0.200");

  app.start();


  // set center freq
  app.setProperty("rx","frequency_MHz", "300.000");
  app.setProperty("rx","bb_gain_dB", "40");
  app.setProperty("rx","rf_cutoff_frequency_MHz", "400");
  usleep(30000);

  // bb_ cutoff test for loop
  double prop;

  for (int i = 1; i < 112; i++)
  {
    prop = i * 0.125;
    std::ostringstream strs;
    strs << prop;
    app.setProperty("rx","bb_cutoff_frequency_MHz", strs.str().c_str());
    strs.str("");
    strs.clear();
    usleep(10000);
  }

  // rf_gain testing for loop
  for (int i = 1; i < 97; i++)
  {
    prop = (i-66) * 0.5;
    std::ostringstream strs;
    strs << prop;
    app.setProperty("rx","rf_gain_dB", strs.str().c_str());
    strs.str("");
    strs.clear();
    usleep(10000);
  }
  app.setProperty("rx","rf_gain_dB", "0");
  usleep(30000);

  // bb_gain testing for loop
  for (int i = 5; i < 60; i++)
  {
    prop = i;
    std::ostringstream strs;
    strs << prop;
    app.setProperty("rx","bb_gain_dB", strs.str().c_str());
    strs.str("");
    strs.clear();
    usleep(10000);
  }

  cout << "Set sig gen: freq=3000.005 MHz, amp=-55 dBm" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  usleep(100000);
  app.setProperty("rx","rf_gain_dB", "16");
  app.setProperty("rx","frequency_MHz", "3000");

  printf("stopping\n");

  usleep(1000000);

  app.stop();

  // printf("Dump of all final property values:\n");
  // for (unsigned n = 0; app.getProperty(n, name, value, false, &isParameter); n++)
  // printf("Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
  //     isParameter ? " (parameter)" : "");

  printf("stopped\n");

  return 1;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }
}
