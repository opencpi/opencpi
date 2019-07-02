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
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "OcpiApi.h"
#include <iostream>
#include <stdlib.h>

namespace OA = OCPI::API;
using namespace std;

enum HdlPlatform {matchstiq_z1, zed, other};

static void usage(const char *name, const char *error_message, HdlPlatform currentPlatform) {
  fprintf(stderr,
	  "%s\n"
	  "Usage is: %s <rf_tune_freq> <data_bw> <rf_bw> <rf_gain> <bb_bw> <bb_gain> <runtime> \n"
	  "    rf_tune_freq       # RF tuning frequency (232.5 - 3720) MHz\n"
	  "    data_bw            # Bandwidth of the data being written to file (%s) MS/s\n"
	  "    rf_bw              # RF bandwidth (%s) MHz\n"
	  "    rf_gain            # RF gain (%s) dB\n"
	  "    bb_bw              # Baseband bandwidth (0.125 - 14) MHz\n"
	  "    bb_gain            # Baseband gain (5 - 60) dB\n"
	  "    runtime            # Runtime of app (1 - 5) seconds\n"
	  "Example: ./target-xilinx13_3/testbench 1000 5 400 10 9 14 1\n",
       	  error_message, 
	  name, 
	  currentPlatform==matchstiq_z1 ? "0.125 - 5" : "3.84",
	  currentPlatform==matchstiq_z1 ? "0 or 400" : "-1",
	  currentPlatform==matchstiq_z1 ? "-32.5 - +16" : "-6 - +6");
  exit(1);
}


static void print_limits(const char *error_message, double current_value, double lower_limit, double upper_limit) {
  fprintf(stderr,
  "%s"
  "Value given of %f is outside the range of %f to %f.\n",
  error_message, current_value, lower_limit, upper_limit);
  exit(1);
}

int main(int argc, char **argv) {
  OA::Container *container;
  HdlPlatform currentPlatform = other;
  std::string xml_name;

  //Check what platform we are on
  for (unsigned n = 0; (container = OA::ContainerManager::get(n)); n++)
  {
    if (container->platform() == "matchstiq_z1")
    {
      currentPlatform = matchstiq_z1;
    }
    else if (container->model() == "hdl" && (container->platform() == "zed" || container->platform() == "zed_ise"))
    {
      currentPlatform = zed;
    }
  }

  //Assign the appropriate application XML file
  if (currentPlatform == matchstiq_z1)
  {
    xml_name = "app_matchstiq_z1.xml";
  }
  else if (currentPlatform == zed)
  {
    xml_name = "app_zipper_dc_offset_filter_filew.xml";
  }
  else
  {
    fprintf(stderr,"Error: not on a valid platform\n");
    exit(1);
  }

  const char *argv0 = strrchr(argv[0], '/');
  argv0 = argv[0];
  bool overrunFlag = false;
  if (argc != 8) //Check number of inputs
    usage(argv0,"Error: wrong number of arguments.\n",currentPlatform);

  //Verify inputs are within valid ranges
  double rf_tune_freq = strtod(argv[1],NULL);
  double data_bw = strtod(argv[2],NULL);
  double rf_bw = strtod(argv[3],NULL);
  double rf_gain = strtod(argv[4],NULL);
  double bb_bw = strtod(argv[5],NULL);
  double bb_gain = strtod(argv[6],NULL);
  uint8_t runtime = atoi(argv[7]);
  char rx_sample_rate_str [25];
  double rx_sample_rate;
  std::string name, value;

  //Check arguments NOT associated with RF frontend
  //  if (data_bw < 0.1 || data_bw > 5)
  if (data_bw < 0.1 || data_bw > 10)
  {
    print_limits("Error: invalid data_bw.\n", data_bw, 0.1, 5);
  }
  if (runtime < 1 || runtime > 5)
  {
    print_limits("Error: invalid runtime.\n", data_bw, 1, 5);
  }

  try {
    printf("RX App\n");
    printf("ADC->DC Offset Filter->File Write\n");
    printf("Other application properties in XML file: %s\n", xml_name.c_str());

    OA::Application app(xml_name.c_str(), NULL);
    app.initialize();

    //bool isParameter;
    // printf("Dump of all initial property values:\n");
    // for (unsigned n = 0; app.getProperty(n, name, value, false, &isParameter); n++)
    // printf("Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
    //    isParameter ? " (parameter)" : "");


    //Check arguments associated with RF frontend
    double rx_sample_rate_min_MHz, rx_sample_rate_max_MHz;
    double rx_frequency_min_MHz, rx_frequency_max_MHz;
    double rx_rf_cutoff_frequency_min_MHz, rx_rf_cutoff_frequency_max_MHz;
    double rx_rf_gain_min_dB, rx_rf_gain_max_dB;
    double rx_bb_cutoff_frequency_min_MHz, rx_bb_cutoff_frequency_max_MHz;
    double rx_bb_gain_min_dB, rx_bb_gain_max_dB;
  
    app.getProperty("rx","sample_rate_min_MHz", value);
    rx_sample_rate_min_MHz = atof(value.c_str());
    app.getProperty("rx","sample_rate_max_MHz", value);
    rx_sample_rate_max_MHz = atof(value.c_str());

    // multiply by 8 to compensate for cic decimate by 8
    //  rx_sample_rate = data_bw * 8;
    rx_sample_rate = data_bw;
    if (rx_sample_rate < rx_sample_rate_min_MHz || rx_sample_rate > rx_sample_rate_max_MHz)
      {
	print_limits("Error: invalid data_bw.\n", data_bw, rx_sample_rate_min_MHz, rx_sample_rate_max_MHz);
      }

    app.getProperty("rx","frequency_min_MHz", value);
    rx_frequency_min_MHz = atof(value.c_str());
    app.getProperty("rx","frequency_max_MHz", value);
    rx_frequency_max_MHz = atof(value.c_str());
    if (rf_tune_freq < rx_frequency_min_MHz || rf_tune_freq > rx_frequency_max_MHz)
      {
	print_limits("Error: invalid rx_rf_center_freq.\n", rf_tune_freq, rx_frequency_min_MHz, rx_frequency_max_MHz);
      }

    app.getProperty("rx","rf_cutoff_frequency_min_MHz", value);
    rx_rf_cutoff_frequency_min_MHz = atof(value.c_str());
    app.getProperty("rx","rf_cutoff_frequency_max_MHz", value);
    rx_rf_cutoff_frequency_max_MHz = atof(value.c_str());
    if (rf_bw < rx_rf_cutoff_frequency_min_MHz || rf_bw > rx_rf_cutoff_frequency_max_MHz)
      {
	print_limits("Error: invalid rf_bw.\n", rf_bw, rx_rf_cutoff_frequency_min_MHz, rx_rf_cutoff_frequency_max_MHz);
      }

    app.getProperty("rx","rf_gain_min_dB", value);
    rx_rf_gain_min_dB = atof(value.c_str());
    app.getProperty("rx","rf_gain_max_dB", value);
    rx_rf_gain_max_dB = atof(value.c_str());
    if (rf_gain < rx_rf_gain_min_dB || rf_gain > rx_rf_gain_max_dB)
      {
	print_limits("Error: invalid rf_gain.\n", rf_gain, rx_rf_gain_min_dB, rx_rf_gain_max_dB);
      }

    app.getProperty("rx","bb_cutoff_frequency_min_MHz", value);
    rx_bb_cutoff_frequency_min_MHz = atof(value.c_str());
    app.getProperty("rx","bb_cutoff_frequency_max_MHz", value);
    rx_bb_cutoff_frequency_max_MHz = atof(value.c_str());
    if (bb_bw < rx_bb_cutoff_frequency_min_MHz || bb_bw > rx_bb_cutoff_frequency_max_MHz)
      {
	print_limits("Error: invalid bb_bw.\n", bb_bw, rx_bb_cutoff_frequency_min_MHz, rx_bb_cutoff_frequency_max_MHz);
      }

    app.getProperty("rx","bb_gain_min_dB", value);
    rx_bb_gain_min_dB = atof(value.c_str());
    app.getProperty("rx","bb_gain_max_dB", value);
    rx_bb_gain_max_dB = atof(value.c_str());
    if (bb_gain < rx_bb_gain_min_dB || bb_gain > rx_bb_gain_max_dB)
      {
	print_limits("Error: invalid bb_gain.\n", bb_gain, rx_bb_gain_min_dB, rx_bb_gain_max_dB);
      }

    //Setup Matchstiq-Z1 front end
    app.setProperty("rx","frequency_Mhz", argv[1]);

    sprintf(rx_sample_rate_str,  "%f", rx_sample_rate); /// @todo / FIXME - handle return value?
    app.setProperty("rx","sample_rate_Mhz", rx_sample_rate_str);


    app.setProperty("rx","rf_cutoff_frequency_Mhz", argv[3]);
    app.setProperty("rx","rf_gain_dB", argv[4]);
    app.setProperty("rx","bb_cutoff_frequency_Mhz", argv[5]);
    app.setProperty("rx","bb_gain_dB", argv[6]);

    printf("RF tune frequency  : %s MHz\n", argv[1]);
    printf("Data bandwidth     : %s MS/s\n",argv[2]);
    printf("RF cutoff frequency: %s MHz\n", argv[3]);
    printf("RF gain            : %s dB\n",  argv[4]);
    printf("BB cutoff frequency: %s MHz\n", argv[5]);
    printf("BB gain            : %s dB\n",  argv[6]);
    std::string value;
    app.getProperty("time_server","status",value);
    uint32_t valid1pps=atol(value.c_str())&0x08000000;
    printf("Valid 1 PPS        : %s \n", valid1pps ? "true" : "false");

    app.setProperty("time_server","timeNow","0");
    app.start();

    // workaround for broken property
    app.setProperty("qadc","overrun", "false");

    printf("App runs for %i seconds...\n",runtime);
    while (runtime > 0)
      {
	app.getProperty("qadc","overrun", value);
	if (strcmp("true", value.c_str()) == 0)
	  {
	    overrunFlag  = true;
	  }
	sleep(1);
	runtime --;
      }

    app.stop();

    // printf("Dump of all final property values:\n");
    // for (unsigned n = 0; app.getProperty(n, name, value, false, &isParameter); n++)
    // printf("Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(),
    //    isParameter ? " (parameter)" : "");


    app.getProperty("file_write","bytesWritten", value);
    printf("Bytes to file : %s\n", value.c_str());
    if (overrunFlag)
      {
	printf("ERROR: Unable to write to file fast enough, try a lower sample rate\n");
      }

    //Query Peak values
    printf("Peak values observed by RX app components\n");  
    app.getProperty("dc_offset_filter","peak", value);
    printf("DC Offset Peak         = %s\n", value.c_str());

    // copy file to local location from ram
    system("mv /var/volatile/output_file.bin odata/output_file.bin"); /// @todo / FIXME - handle return value?

    printf("Application complete\n");

  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
  }


  return 0;
}

