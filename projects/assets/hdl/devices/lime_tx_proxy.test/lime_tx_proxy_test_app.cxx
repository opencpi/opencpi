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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <iomanip>
#include <math.h>
#include <memory>
#include "OcpiApi.h"

namespace OA = OCPI::API;
using namespace std;

static void printLimits(const char *error_message, double current_value, double lower_limit, double upper_limit) {
  fprintf(stderr,
  "%s"
  "Value given of %f is outside the range of %f to %f.\n",
  error_message, current_value, lower_limit, upper_limit);
  exit(1);
}


void run_test(std::unique_ptr<OA::Application>& app)
{
  printf("turn up vga1\n");
  app->setProperty("rf_tx_proxy","output_gain_db", "0");
  sleep(2);

  printf("turn up vga1\n");
  app->setProperty("rf_tx_proxy","output_gain_db", "10");
  sleep(2);

  printf("turn up vga1\n");
  app->setProperty("rf_tx_proxy","output_gain_db", "25");
  sleep(2);

  printf("turn up vga2\n");
  app->setProperty("rf_tx_proxy","post_lpf_gain_db", "-35");
  sleep(2);

  printf("turn up vga2\n");
  app->setProperty("rf_tx_proxy","post_lpf_gain_db", "-15");
  sleep(2);

  printf("turn up vga2\n");
  app->setProperty("rf_tx_proxy","post_lpf_gain_db", "-4");
  sleep(2);

  app->setProperty("rf_tx_proxy","post_lpf_gain_db", "-20");
  sleep(2);

  printf("changing tx_output\n");
  app->setProperty("rf_tx_proxy","output_select", "2");
  sleep(2);
  app->setProperty("rf_tx_proxy","output_select", "0");
  sleep(2);
  app->setProperty("rf_tx_proxy","output_select", "1");
  sleep(2);

  printf("changing lpf\n");
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "0.75e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "0.875e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "1.25e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "1.375e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "1.5e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "1.92e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "2.5e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "2.75e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "3e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "3.5e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "4.375e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "5e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "6e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "7e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "10e6");
  sleep(2);
  app->setProperty("rf_tx_proxy","lpf_bw_hz", "14e6");
  sleep(2);
}

enum HdlPlatformRFFrontend {matchstiq_z1_Frontend, zipperFrontend};

int main(int argc, char **argv)
{
  OA::Container *container;
  std::string container_name, xml_name, input;
  const HdlPlatformRFFrontend defaultFrontend = matchstiq_z1_Frontend;
  HdlPlatformRFFrontend currentFrontend = defaultFrontend;

  // Check that the container is supported and use the 
  // container information to customize execution of the app->
  bool validContainerFound = false;
  for (unsigned n = 0; (container = OA::ContainerManager::get(n)); n++)
  {
    printf("Container discovered: %s\n", container->platform().c_str()) ;
    if (container->platform() == "matchstiq_z1")
    {
      currentFrontend = matchstiq_z1_Frontend;
      container_name = "matchstiq_z1";
      validContainerFound = true;
    }
    else if ((container->platform() == "zed" || container->platform() == "zed_ise") && container->model() =="hdl")
    { 
      currentFrontend = zipperFrontend;
      container_name = "zed";
      validContainerFound = true;
    }
    else if (container->platform() == "alst4")
    { 
      currentFrontend = zipperFrontend;
      container_name = "alst4";
      validContainerFound = true;
    }
    else if (container->platform() == "alst4x")
    { 
      ///@TODO add support
      //currentFrontend = zipperFrontend;
      container_name = "alst4x";
      fprintf(stderr,"Error: not on a valid platform (this app does not currently support alst4x containers)\n");
      return 1;
    }
    else if (container->platform() == "ml605")
    { 
      currentFrontend = zipperFrontend;
      container_name = "ml605";
      validContainerFound = true;
    }
    if (validContainerFound)
    {
      break;
    }
  }

  if (!validContainerFound)
  { fprintf(stderr,"Error: not on a valid platform (no containers found)\n");
    return 1;
  }

  //Assign the appropriate application XML file
  if (currentFrontend == matchstiq_z1_Frontend)
  {
    xml_name = "lime_tx_proxy_test_matchstiq_z1_app.xml";
  }
  else
  {
    // currentFrontend == zipperFrontend
    xml_name = "lime_tx_proxy_test_zipper_app.xml";
  }

  try{
  std::unique_ptr<OA::Application> app (new OA::Application(xml_name.c_str(), NULL));
  char cin_value;
  double sample_rate;// = 1; // default
  double sample_rate_min;
  double sample_rate_max;
  std::string ClkInStr;
  std::string rxClkInStr = std::string("0"); // Forced to 0 during Tx
  std::string txClkInStr;
  std::ostringstream clock_gen_config;

  app->initialize();

  if (currentFrontend == matchstiq_z1_Frontend)
    {
      sample_rate = 0.25; // default
      sample_rate_min = 0.1; // FIXME - read this value from runtime implementation of rf_rx-spec
      sample_rate_max = 40.0; // FIXME - read this value from runtime implementation of rf_rx-spec

      // Prompt user to set sampling rate and verify entry
      cout << setprecision(5) << "Enter sampling rate: (" 
  	   << sample_rate_min << " to " << sample_rate_max 
  	   << " MSps) [default = " << sample_rate << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
  	{
  	  std::istringstream stream(input);
  	  stream >> sample_rate;
  	}
      if (sample_rate < sample_rate_min || sample_rate > sample_rate_max)
  	{
  	  printLimits("Error: invalid sample_rate.\n", sample_rate, sample_rate_min, sample_rate_max);
  	}

      // Si5338:CLKIN (i.e. Si5338:CH0 = Rx clock to Lime transceiver) = 2 x sample_rate x 1 MHz
      ClkInStr = to_string(2 * sample_rate * 1e6).c_str();

      // Only required to set CH0 of the Clock Synthesis device
      clock_gen_config <<
  	// CH0
  	"{output_hz " << ClkInStr <<
  	",source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
  	// CH1
  	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
  	// CH2
  	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
  	// CH3
  	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}";
    }
  else
    { // i.e. currentFrontend == zipperFrontend
            sample_rate = 0.5; // default
            sample_rate_min = 0.500;
      //      sample_rate = 0.025; // default
      //      sample_rate_min = 0.025;
      if (container_name == "alst4") 
        // imposing further restriction due to known runtime limitations on alst4,
	// setting to floor(<worst case sample rate that's still reliable> - 10%)
        // i.e. floor(22 Msps *  0.9) = 19
        // 10% is included because the error varies from bitstream to bitstream,
        // so we're accounting for a reasonable amount of uncertainty in the
        // 22 Msps measurement
	sample_rate_max = 22.0;
      else
        // imposing further restriction due to known runtime limitations on ml605,
	// setting to floor(<worst case sample rate that's still reliable> - 10%)
        // i.e. floor(34 Msps * 0.9) = 30
        // 10% is included because the error varies from bitstream to bitstream,
        // so we're accounting for a reasonable amount of uncertainty in the
        // 34 Msps measurement
	sample_rate_max = 30.0;

      // Prompt user to set sampling rate and verify entry
      cout << setprecision(5) << "Enter sampling rate: (" 
      	   << sample_rate_min << " to " << sample_rate_max 
      	   << " MSps) [default = " << sample_rate << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      	{
      	  std::istringstream stream(input);
      	  stream >> sample_rate;
      	}
      if (sample_rate < sample_rate_min || sample_rate > sample_rate_max)
      	{
      	  printLimits("Error: invalid sample_rate.\n", sample_rate, sample_rate_min, sample_rate_max);
      	}

      // Si5351:CLKIN (i.e. Si5351:CH4 & 5 = Tx clock to Lime transceiver & FPGA) = 2 x sample_rate x 1 MHz
      txClkInStr = to_string(2 * sample_rate * 1e6).c_str();

      // CH 4&5 tx CH 2&3 rx
      clock_gen_config <<
      	// CH0
      	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH1
      	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH2
      	"{output_hz " << rxClkInStr <<
      	",source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH3
      	"{output_hz " << rxClkInStr <<
      	",source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH4
	"{output_hz " << txClkInStr <<
      	",source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH5
	"{output_hz " << txClkInStr <<
      	",source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH6
      	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}," <<
      	// CH7
      	"{output_hz 0,source 0x0,inverted false,spread none,spreadAmount 0,disabled_mode z}";
    }

  // Configure clock synthesis device
  app->setProperty("clock_gen", "channels", clock_gen_config.str().c_str());

  app->start();

  cout << "Set spectrum anaylzer: fc=300 MHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("rf_tx_proxy","center_freq_hz", ".300e9");
  sleep(2);
  run_test(app);

  cout << "Set spectrum anaylzer: fc=1 GHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("rf_tx_proxy","center_freq_hz", "1.00e9");
  sleep(2);
  run_test(app);

  cout << "Set spectrum anaylzer: fc=2 GHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("rf_tx_proxy","center_freq_hz", "2.00e9");
  sleep(2);
  run_test(app);

  cout << "Set spectrum anaylzer: fc=3 GHz, span=60 MHz" << endl;
  cout << "Enter y to Continue" << endl;
  cin >> cin_value;
  app->setProperty("rf_tx_proxy","center_freq_hz", "3.00e9");
  sleep(2);
  run_test(app);

  // TODO: Improve the test to emphasize the value of setting these registers
  // printf("changing leakage values\n");
  // app->setProperty("rf_tx_proxy","pre_mixer_dc_offset_i", "0x43");
  // app->setProperty("rf_tx_proxy","pre_mixer_dc_offset_q", "0x07");
  // sleep(2);
  
  app->stop();

  printf("Lime TX Testbench: Complete\n");

  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }

}
