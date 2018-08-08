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
#include <string.h>
#include "OcpiApi.h"
#include <sstream>
#include <iostream>
#include <fstream>

namespace OA = OCPI::API;
using namespace std;

void run_test(OA::Application* app)
{
  printf("turn up vga1\n");
  app->setProperty("tx","rf_gain_dB", "0");
  sleep(2);

  printf("turn up vga1\n");
  app->setProperty("tx","rf_gain_dB", "10");
  sleep(2);

  printf("turn up vga1\n");
  app->setProperty("tx","rf_gain_dB", "25");
  sleep(2);

  printf("turn up vga2\n");
  app->setProperty("tx","bb_gain_dB", "-35");
  sleep(2);

  printf("turn up vga2\n");
  app->setProperty("tx","bb_gain_dB", "-15");
  sleep(2);

  printf("turn up vga2\n");
  app->setProperty("tx","bb_gain_dB", "-4");
  sleep(2);

  app->setProperty("tx","bb_gain_dB", "-20");
  sleep(2);

  printf("changing lpf\n");
  app->setProperty("tx","bb_cutoff_frequency_MHz", "0.75");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "0.875");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "1.25");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "1.375");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "1.5");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "1.92");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "2.5");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "2.75");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "3");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "3.5");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "4.375");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "5");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "6");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "7");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "10");
  sleep(2);
  app->setProperty("tx","bb_cutoff_frequency_MHz", "14");
  sleep(2);
}

int main(int argc, char **argv)
{
  try{
  std::string name, value;
  std::string xml_name = "";
  char cin_value;

  xml_name = "testbench_app_tx.xml";
  printf("%s\n", xml_name.c_str());
  OA::Application* app = new OA::Application("testbench_app_tx.xml", NULL);
  app->initialize();

  // bool isParameter;
  // printf("Dump of all initial property values:\n");
  // for (unsigned n = 0; app->getProperty(n, name, value, false, &isParameter); n++)
  // printf("Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
  //    isParameter ? " (parameter)" : "");

  app->setProperty("clock_gen","channels", "{output_hz 500e3,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z},{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}");

  app->start();
  cout << "Set spectrum anaylzer: fc=300 MHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("tx","frequency_MHz", ".300e3");
  sleep(2);
  run_test(app);

  cout << "Set spectrum anaylzer: fc=1 GHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("tx","frequency_MHz", "1.00e3");
  sleep(2);
  run_test(app);

  cout << "Set spectrum anaylzer: fc=2 GHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("tx","frequency_MHz", "2.00e3");
  sleep(2);
  run_test(app);

  cout << "Set spectrum anaylzer: fc=3 GHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("tx","frequency_MHz", "3.00e3");
  sleep(2);
  run_test(app);

  app->stop();

  // printf("Dump of all final property values:\n");
  // for (unsigned n = 0; app->getProperty(n, name, value, false, &isParameter); n++)
  // printf("Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
  //     isParameter ? " (parameter)" : "");

  printf("stopped\n");

  return 1;
  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }

}
