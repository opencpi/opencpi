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
#include "OcpiApi.hh"

namespace OA = OCPI::API;
using namespace std;

#define PROMPT(app,val,worker,propmin,propmax) prompt_auto_range(app, val, #val, "Error: invalid "#val".\n",worker,propmin,propmax);

static void usage(const char *name, const char *error_message) {
  fprintf(stderr,
  "%s\n"
  "Usage is: %s <mode> <optional debug_mode>\n"
  "    mode       # Test mode of the application. Valid modes are 'rx', 'tx', 'txrx', 'filerw', or 'bbloopback'.\n"
  "    debug_mode # Optional debug mode which performs both an initial and final dump of all properties. Use 'd'.\n"
  "Example: FSK filerw d\n",
  error_message,name);
  exit(1);
}

static void printLimits(const char *error_message, double current_value, double lower_limit, double upper_limit) {
  fprintf(stderr,
  "%s"
  "Value given of %f is outside the range of %f to %f.\n",
  error_message, current_value, lower_limit, upper_limit);
  exit(1);
}

/*! @brief Compare value x against min and max values read from OpenCPI app
 *         worker(worker)-specified min and max properties and exit (this)
 *         application when limits are invalid.
 ******************************************************************************/
template<typename T> void validate_limits(const T val,
                                          const char* error_message,
                                          T min, T max) {

  if((val<min) or (val>max))
  {
    printLimits(error_message, val, min, max);
  }
}

// just allows skipping creating the property object
template<typename T> T get_worker_prop_val(OA::Application& app,
    const char* worker_app_inst_name, const char* prop_name)
{
  OA::Property prop(app, worker_app_inst_name, prop_name);
  return prop.getValue<T>();
}

template<typename T> void prompt_with_range(T& val, const char* val_c_str,
    const char* error_message, T& min, T& max)
{
  cout << setprecision(15) << "Enter " << val_c_str << " [range("
       << min << " - " << max << ") default = "
       << val << "]" << endl;
  std::string input;
  std::getline(std::cin, input);
  if (!input.empty())
  {
    std::istringstream stream(input);
    stream >> val;
  }
  validate_limits(val, error_message, min, max);
}

template<typename T> void prompt_auto_range(OA::Application& app, T& val, const char* val_c_str,
    const char* error_message, const char* worker, const char* propmin, const char* propmax)
{
  T min = get_worker_prop_val<T>(app, worker, propmin);
  T max = get_worker_prop_val<T>(app, worker, propmax);

  prompt_with_range(val, val_c_str, error_message, min, max);
}

uint16_t get_zero_pad_num_zeros(OCPI::API::Application& app) {
  OA::Property p(app, "zero_pad", "num_zeros");
  uint16_t zero_pad_num_zeros = p.getUShortValue();
  return zero_pad_num_zeros;
}

/*! @return Currently configured (based on worker property values)
 *          CIC interpolation factor.
 ******************************************************************************/
uint16_t get_cic_interpolation_factor(OCPI::API::Application& app) {
  OA::Property p(app, "cic_int", "R");
  uint16_t cic_interpolation_factor = p.getUShortValue();
  return cic_interpolation_factor;
}

/*! @return Currently configured (based on worker property values)
 *          TX interpolation factor (combination of cic_int and zero_pad
 *          workers).
 ******************************************************************************/
double get_tx_interpolation_factor(OCPI::API::Application& app) {
  // x0, x1, ... -> zero_padding --> x0, [0, 0, ...], x1, [0 0 ..]
  // there are num zeros+1 samples out for every sample in
  uint16_t zero_pad_num_zeros = get_zero_pad_num_zeros(app);
  double zp_factor = ((double)zero_pad_num_zeros) + 1.;

  uint16_t cic_interpolation_factor = get_cic_interpolation_factor(app);

  double tx_interpolation_factor = zp_factor * ((double)cic_interpolation_factor);
  return tx_interpolation_factor;
}

/*! @return Currently configured (based on worker property values)
 *          TX sample rate in MHz.
 ******************************************************************************/
double get_tx_sample_rate_MHz(OCPI::API::Application& app) {
  OA::Property p(app, "tx", "sample_rate_MHz");
  double tx_sample_rate_MHz = p.getDoubleValue();
  return tx_sample_rate_MHz;
}

/*! @return Currently configured (based on worker property values)
 *          TX bit rate in bits/sec.
 ******************************************************************************/
double get_tx_bit_rate_bps(OCPI::API::Application& app) {
  double tx_sample_rate_MHz = get_tx_sample_rate_MHz(app);
  double tx_sample_rate_Hz = tx_sample_rate_MHz *1000000.;

  double rate = tx_sample_rate_Hz/get_tx_interpolation_factor(app);
  return rate;
}

int main(int argc, char **argv) {

  //Assign default values
  const char *argv0 = strrchr(argv[0], '/');
  argv0 = argv[0];
  bool overrunFlag = false;
  std::string mode, xml_name, input, value;
  bool debug_mode = false;
  double rx_sample_rate = 39.936;
  double rx_rf_center_freq = 999;
  double rx_rf_bw;
  double rx_rf_gain;
  double rx_bb_bw = 5;
  double rx_bb_gain = 42;
  double rx_if_center_freq = 1;
  double tx_sample_rate = 39.936;
  double tx_rf_center_freq = 1000;
  double tx_rf_gain = 4;
  double tx_bb_bw = 5;
  double tx_bb_gain = -4;
  //double tx_rf_bw = -1;

  double rx_sample_rate_min;
  double rx_rf_center_freq_min;
  double rx_rf_bw_min;
  double rx_rf_gain_min;
  double rx_bb_bw_min;
  double rx_bb_gain_min;
  double rx_if_center_freq_min;
  double tx_sample_rate_min;
  double tx_rf_center_freq_min;
  double tx_rf_gain_min;
  double tx_bb_bw_min;
  double tx_bb_gain_min;
  //double tx_rf_bw_min;

  double rx_sample_rate_max;
  double rx_rf_center_freq_max;
  double rx_rf_bw_max;
  double rx_rf_gain_max;
  double rx_bb_bw_max;
  double rx_bb_gain_max;
  double rx_if_center_freq_max;
  double tx_sample_rate_max;
  double tx_rf_center_freq_max;
  double tx_rf_gain_max;
  double tx_bb_bw_max;
  double tx_bb_gain_max;
  //double tx_rf_bw_max;


  double runtime;
  enum HdlPlatform           {matchstiq_z1, zed, alst4, alst4x, ml605};
  enum HdlPlatformRFFrontend {matchstiq_z1_Frontend, zipperFrontend, FMCOMMS2Frontend, FMCOMMS3Frontend};
  OA::Container *container;
  const HdlPlatform           defaultPlatform = matchstiq_z1;
  const HdlPlatformRFFrontend defaultFrontend = matchstiq_z1_Frontend;
  HdlPlatform           currentPlatform = defaultPlatform;
  HdlPlatformRFFrontend currentFrontend = defaultFrontend;

  //Check what platform we are on
  bool validContainerFound = false;
  for (unsigned n = 0; (container = OA::ContainerManager::get(n)); n++)
  {
    printf("container is %s\n", container->platform().c_str()) ;

    if (container->model() == "hdl" && container->platform() == "matchstiq_z1")
    {
      currentPlatform = matchstiq_z1;
      currentFrontend = matchstiq_z1_Frontend;
      validContainerFound = true;
      printf("FSK App for Matchstiq-Z1\n");
    }
    else if ((container->platform() == "zed" || container->platform() == "zed_ise") && container->model() == "hdl")
    {
      currentPlatform = zed;
      validContainerFound = true;

      if (argc > 1)
      {
        mode = argv[1];
        if ( (mode != "rx") && (mode != "tx") && (mode != "txrx") && (mode != "filerw") && (mode != "bbloopback") )
        {
          usage(argv0,"Error: incorrect test mode.\n");
        }
      }
      else
      {
        usage(argv0,"Error: wrong number of arguments.\n");
      }

      if(mode != "filerw")
      {
        cout << "Zedboard detected, enter RF frontend for Zedboard (zipper or FMCOMMS2 or FMCOMMS3)" << endl;
        std::getline(std::cin, input);
        if (!input.empty())
        {
          std::istringstream stream(input);
          if(stream.str() == "FMCOMMS2")
          {
            currentFrontend = FMCOMMS2Frontend;
          }
          else if(stream.str() == "FMCOMMS3")
          {
            currentFrontend = FMCOMMS3Frontend;
          }
          else if(stream.str() == "zipper")
          {
            currentFrontend = zipperFrontend;
          }
          else
          {
            usage(argv[0],"Error: invalid frontend specified.\n");
          }
        }
        else
        {
          usage(argv[0],"Error: invalid frontend specified.\n");
        }
      }

      printf("FSK App for Zedboard\n");
    }
    else if (container->platform() == "alst4" && container->model() == "hdl")
    {
      currentPlatform = alst4;
      currentFrontend = zipperFrontend;
      validContainerFound = true;
      printf("FSK App for alst4\n");
    }
    else if (container->platform() == "alst4x" && container->model() == "hdl")
    {
      ///@TODO add support
      //currentPlatform = alst4x;
      //currentFrontend = zipperFrontend;
      fprintf(stderr,"Error: not on a valid platform (this app does not currently support alst4x containers\n");
      return 1;
    }
    else if (container->platform() == "ml605" && container->model() == "hdl")
    {
      currentPlatform = ml605;

      if (argc > 1)
      {
        mode = argv[1];
        if ( (mode != "rx") && (mode != "tx") && (mode != "txrx") && (mode != "filerw") && (mode != "bbloopback") )
        {
          usage(argv0,"Error: incorrect test mode.\n");
        }
      }
      else
      {
        usage(argv0,"Error: wrong number of arguments.\n");
      }

      if(mode != "filerw")
      {
        cout << "ML605 detected, enter RF frontend for ML605 (zipper or FMCOMMS2 or FMCOMMS3)" << endl;
        std::getline(std::cin, input);
        if (!input.empty())
        {
          std::istringstream stream(input);
          if(stream.str() == "FMCOMMS2")
          {
            currentFrontend = FMCOMMS2Frontend;
          }
          else if(stream.str() == "FMCOMMS3")
          {
            currentFrontend = FMCOMMS3Frontend;
          }
          else if(stream.str() == "zipper")
          {
            currentFrontend = zipperFrontend;
          }
          else
          {
            usage(argv[0],"Error: invalid frontend specified.\n");
          }
        }
        else
        {
          usage(argv[0],"Error: invalid frontend specified.\n");
        }
      }

      validContainerFound = true;
      printf("FSK App for ml605\n");
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

  printf("model is %s, container is %s\n", container->model().c_str(), container->platform().c_str()) ;

  if (currentFrontend == matchstiq_z1_Frontend)
  {
    xml_name = "lime_adc_test_matchstiq_z1_app.xml";
      rx_rf_bw = 400;
      rx_rf_gain = 10;

      rx_sample_rate_min = 0.1; // TODO / FIXME - set to rx_spec-specified minimum value after app is started
      rx_rf_center_freq_min = 0.2325e3;
      rx_rf_bw_min = 0;
      rx_rf_gain_min = -32.5;
      rx_bb_bw_min = 0;
      rx_bb_gain_min = 5;
      tx_sample_rate_min = 0.1; // TODO / FIXME - read this value from rx_spec after app is started
      tx_rf_center_freq_min = 0.2325e3;
      tx_rf_gain_min = 0;
      tx_bb_bw_min = 0;
      tx_bb_gain_min =-35;
      //tx_rf_bw_min = -1;

      rx_sample_rate_max = 40; // TODO / FIXME - set to rx_spec-specified maximum value after app is started
      rx_rf_center_freq_max = 3.72e3;
      rx_rf_bw_max = 400;
      rx_rf_gain_max = 16;
      rx_bb_bw_max = 14;
      rx_bb_gain_max = 60;
      tx_sample_rate_max = 40; // TODO / FIXME - read this value from rx_spec after app is started
      tx_rf_center_freq_max = 3.72e3;
      tx_rf_gain_max = 25;
      tx_bb_bw_max = 14;
      tx_bb_gain_max = -4;
      //tx_rf_bw_max = -1;
      //printf("FSK App for Matchstiq-Z1\n");
  }
  else if(currentFrontend == zipperFrontend)
  {
    if(currentPlatform == zed)
    {
        // imposing further restriction due to known runtime limitations on
        // zed, setting to
        //      floor(<worst case sample rate that's still reliable> - 10%)
        // i.e. floor(36 Msps                                        * 0.9) = 32
        // 10% is included because the error varies from bitstream to bitstream,
        // so we're accounting for a reasonable amount of uncertainty in the
        // 36 Msps measurement
        rx_sample_rate = 32;
        tx_sample_rate = 32;
        rx_sample_rate_max = 32;
        tx_sample_rate_max = 32;
    }

    if(currentPlatform == alst4)
    {
        // imposing further restriction due to known runtime limitations on
        // ML605, setting to
        //      floor(<worst case sample rate that's still reliable> - 10%)
        // i.e. floor(22 Msps                                        * 0.9) = 19
        // 10% is included because the error varies from bitstream to bitstream,
        // so we're accounting for a reasonable amount of uncertainty in the
        // 22 Msps measurement
        rx_sample_rate = 19;
        tx_sample_rate = 19;
        rx_sample_rate_max = 19;
        tx_sample_rate_max = 19;
    }
    else if(currentPlatform == ml605)
    {
        // imposing further restriction due to known runtime limitations on
        // ML605, setting to
        //      floor(<worst case sample rate that's still reliable> - 10%)
        // i.e. floor(34 Msps                                        * 0.9) = 30
        // 10% is included because the error varies from bitstream to bitstream,
        // so we're accounting for a reasonable amount of uncertainty in the
        // 34 Msps measurement
        rx_sample_rate = 30;
        tx_sample_rate = 30;
        rx_sample_rate_max = 30;
        tx_sample_rate_max = 30;
      }
      rx_rf_bw = -1;
      rx_rf_gain = 3;
      rx_sample_rate_min = 0.5; // TODO / FIXME - read this value from rx_spec after app is started
      rx_rf_center_freq_min = 0.2325e3;
      rx_rf_bw_min = -1;
      rx_rf_gain_min = -6;
      rx_bb_bw_min = 0;
      rx_bb_gain_min = 5;
      tx_sample_rate_min = 0.5; // TODO / FIXME - read this value from rx_spec after app is started
      tx_rf_center_freq_min = 0.2325e3;
      tx_rf_gain_min = 0;
      tx_bb_bw_min = 0;
      tx_bb_gain_min =-35;
      //tx_rf_bw_min = -1;

      rx_rf_center_freq_max = 3.72e3;
      rx_rf_bw_max = -1;
      rx_rf_gain_max = 6;
      rx_bb_bw_max = 14;
      rx_bb_gain_max = 60;
      tx_rf_center_freq_max = 3.72e3;
      tx_rf_gain_max = 25;
      tx_bb_bw_max = 14;
      tx_bb_gain_max = -4;
      //tx_rf_bw_max = -1;
      //printf("FSK App for Zedboard\n");
  }
  // for FMCOMMS frontends, wait until after app initialization and read min/max values from rx/tx workers

  try {
    //Check number of inputs and test mode
    if (argc > 1)
    {
      mode = argv[1];
      if ( (mode != "rx") && (mode != "tx") && (mode != "txrx") && (mode != "filerw") && (mode != "bbloopback") )
      {
        usage(argv0,"Error: incorrect test mode.\n");
      }
    }
    else
    {
      usage(argv0,"Error: wrong number of arguments.\n");
    }
    //Check for debug flag
    if (argc > 2)
    {
      std::string arg_debug_mode = argv[2];
      if (arg_debug_mode == "d")
      {
        debug_mode = true;
      }
    }

    //Assign the appropriate application XML file
    if (currentFrontend == matchstiq_z1_Frontend)
    {
      if (mode == "rx")
      {
        xml_name = "app_fsk_rx_matchstiq_z1.xml";
      }
      else if (mode == "tx")
      {
        xml_name = "app_fsk_tx_matchstiq_z1.xml";
      }
      else if (mode == "txrx" || mode == "bbloopback")
      {
        xml_name = "app_fsk_txrx_matchstiq_z1.xml";
      }
      else if (mode == "filerw")
      {
        xml_name = "app_fsk_filerw.xml";
      }
      else
      {
        usage(argv0,"Error: incorrect test mode.\n");
      }
    }
    else if(currentFrontend == zipperFrontend)
    {
      if (mode == "rx")
      {
        xml_name = "app_fsk_rx_zipper.xml";
      }
      else if (mode == "tx")
      {
        xml_name = "app_fsk_tx_zipper.xml";
      }
      else if (mode == "txrx" || mode == "bbloopback")
      {
        xml_name = "app_fsk_txrx_zipper.xml";
      }
      else if (mode == "filerw")
      {
        xml_name = "app_fsk_filerw.xml";
      }
      else
      {
        usage(argv0,"Error: incorrect test mode.\n");
      }
    }
    else if(currentFrontend == FMCOMMS2Frontend)
    {
      if (mode == "rx")
      {
        xml_name = "app_fsk_rx_fmcomms2.xml";
      }
      else if (mode == "tx")
      {
        xml_name = "app_fsk_tx_fmcomms2.xml";
      }
      else if (mode == "txrx")
      {
        xml_name = "app_fsk_txrx_fmcomms2.xml";
      }
      else if (mode == "bbloopback")
      {
        usage(argv0,"Error: bbloopback mode not supported on FMCOMMS frontends.\n");
        //! @todo TODO/FIXME - support digital AD9361 loopback (which is *not* "B"ase"B"and loopback)
        //xml_name = "app_fsk_txrx_fmcomms2.xml";
      }
      else if (mode == "filerw")
      {
        xml_name = "app_fsk_filerw.xml";
      }
      else
      {
        usage(argv0,"Error: incorrect test mode.\n");
      }
    }
    else if(currentFrontend == FMCOMMS3Frontend)
    {
      if (mode == "rx")
      {
        xml_name = "app_fsk_rx_fmcomms3.xml";
      }
      else if (mode == "tx")
      {
        xml_name = "app_fsk_tx_fmcomms3.xml";
      }
      else if (mode == "txrx")
      {
        xml_name = "app_fsk_txrx_fmcomms3.xml";
      }
      else if (mode == "bbloopback")
      {
        usage(argv0,"Error: bbloopback mode not supported on FMCOMMS frontends.\n");
        //! @todo TODO/FIXME - support digital AD9361 loopback (which is *not* "B"ase"B"and loopback)
        //xml_name = "app_fsk_txrx_fmcomms3.xml";
      }
      else if (mode == "filerw")
      {
        xml_name = "app_fsk_filerw.xml";
      }
      else
      {
        usage(argv0,"Error: incorrect test mode.\n");
      }
    }

    printf("Application properties are found in XML file: %s\n", xml_name.c_str());

    //Prompt the user for rx settings
    if ((mode == "rx" || mode == "txrx") and
        ((currentFrontend == zipperFrontend) or
         (currentFrontend == matchstiq_z1_Frontend)))
    {
      rx_sample_rate_min = mode == "txrx" ? 2 : rx_sample_rate_min;
      cout << setprecision(5) << "Enter RX sample rate in MHz [range("
           << rx_sample_rate_min << " - " << rx_sample_rate_max << ") default = "
           << rx_sample_rate << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_sample_rate;
      }
      if (rx_sample_rate < rx_sample_rate_min || rx_sample_rate > rx_sample_rate_max)
      {
        printLimits("Error: invalid rx_sample_rate.\n", rx_sample_rate, rx_sample_rate_min, rx_sample_rate_max);
      }


      cout << setprecision(5) << "Enter RX rf center frequency in MHz [range("
           << rx_rf_center_freq_min << " - " << rx_rf_center_freq_max << ") default = "
           << rx_rf_center_freq << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_rf_center_freq;
      }

      cout << setprecision(5) << "Enter RX rf bandwidth in MHz [range("
           << rx_rf_bw_min << " - " << rx_rf_bw_max << ") default = "
           << rx_rf_bw << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_rf_bw;
      }

      cout << setprecision(5) << "Enter RX rf gain in dB [range("
           << rx_rf_gain_min << " - " << rx_rf_gain_max << ") default = "
           << rx_rf_gain << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_rf_gain;
      }

      cout << setprecision(5) << "Enter RX baseband bandwidth in MHz [range("
           << rx_bb_bw_min << " - " << rx_bb_bw_max << ") default = "
           << rx_bb_bw << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_bb_bw;
      }

      cout << setprecision(5) << "Enter RX baseband gain in dB [range("
           << rx_bb_gain_min << " - " << rx_bb_gain_max << ") default = "
           << rx_bb_gain << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_bb_gain;
      }

      rx_if_center_freq_min = rx_sample_rate*-0.5 < -15 ? -15 : rx_sample_rate*-0.5;
      rx_if_center_freq_max = rx_sample_rate*32767/65536 > 15 ? 15 : rx_sample_rate*32767/65536;
      cout << setprecision(5) << "RX if center frequency in MHz [range("
           << rx_if_center_freq_min << " - " << rx_if_center_freq_max << ") default = "
           << rx_if_center_freq << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> rx_if_center_freq;
      }
    }

    //Prompt the user for tx settings
    if ((mode == "tx" || mode == "txrx") and
        ((currentFrontend == zipperFrontend) or
         (currentFrontend == matchstiq_z1_Frontend)))
    {
      tx_sample_rate_min = mode == "txrx" ? 2 : tx_sample_rate_min;
      cout << setprecision(5) << "Enter TX sample rate in MHz [range("
           << tx_sample_rate_min << " - " << tx_sample_rate_max << ") default = "
           << tx_sample_rate << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> tx_sample_rate;
      }
      if (tx_sample_rate < tx_sample_rate_min || tx_sample_rate > tx_sample_rate_max)
      {
        printLimits("Error: invalid tx_sample_rate.\n", tx_sample_rate, tx_sample_rate_min, tx_sample_rate_max);
      }

      cout << setprecision(5) << "Enter TX rf center frequency in MHz [range("
           << tx_rf_center_freq_min << " - " << tx_rf_center_freq_max << ") default = "
           << tx_rf_center_freq << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> tx_rf_center_freq;
      }

      cout << setprecision(5) << "Enter TX rf gain in dB [range("
           << tx_rf_gain_min << " - " << tx_rf_gain_max << ") default = "
           << tx_rf_gain << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> tx_rf_gain;
      }

      cout << setprecision(5) << "Enter TX baseband bandwidth in MHz [range("
           << tx_bb_bw_min << " - " << tx_bb_bw_max << ") default = "
           << tx_bb_bw << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> tx_bb_bw;
      }

      cout << setprecision(5) << "Enter TX baseband gain in dB [range("
           << tx_bb_gain_min << " - " << tx_bb_gain_max << ") default = "
           << tx_bb_gain << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> tx_bb_gain;
      }
    }

    //Prompt the user for the amount of time to run for all modes except filerw
    if (mode == "rx" || mode == "tx" || mode == "txrx" || mode == "bbloopback")
    {
      switch (currentFrontend)
      {
        case zipperFrontend:
        case matchstiq_z1_Frontend: runtime = 3;
                                    break;
        case FMCOMMS2Frontend:
        case FMCOMMS3Frontend: runtime = 20;
                               break;
        default: cout << "Invalid frontend choice.";
                 exit(1);
      }

      cout << setprecision(5) << "Enter run time in seconds [default = "
                       << runtime << "]" << endl;
      std::getline(std::cin, input);
      if (!input.empty())
      {
        std::istringstream stream(input);
        stream >> runtime;
      }
    }

    OA::Application app(xml_name.c_str(), NULL);
    app.initialize();
    printf("App initialized.\n");

    if((currentFrontend == FMCOMMS2Frontend) or
       (currentFrontend == FMCOMMS3Frontend))
    {

      if ((mode == "rx") || (mode == "txrx"))
      {
        cout << "Enter FMCOMMS RX SMA channel(RX1A or RX2A)" << endl;
        std::getline(std::cin, input);
        if (!input.empty())
        {
          std::istringstream stream(input);
          if(stream.str() == "RX1A")
          {
            app.setProperty("rx", "config", "reference_clk_rate_Hz 40e6,duplex_mode FDD,are_using_REF_CLK_SMA false,SMA_channel RX1A");
          }
          else if(stream.str() == "RX2A")
          {
            app.setProperty("rx", "config", "reference_clk_rate_Hz 40e6,duplex_mode FDD,are_using_REF_CLK_SMA false,SMA_channel RX2A");
          }
          else
          {
            usage(argv[0],"Error: invalid SMA channel specified.\n");
          }
        }
      }

      if ((mode == "tx") || (mode == "txrx"))
      {
        cout << "Enter FMCOMMS TX SMA channel(TX1A or TX2A)" << endl;
        std::getline(std::cin, input);
        if (!input.empty())
        {
          std::istringstream stream(input);
          if(stream.str() == "TX1A")
          {
            app.setProperty("tx", "config", "reference_clk_rate_Hz 40e6,duplex_mode FDD,are_using_REF_CLK_SMA false,SMA_channel TX1A");
          }
          else if(stream.str() == "TX2A")
          {
            app.setProperty("tx", "config", "reference_clk_rate_Hz 40e6,duplex_mode FDD,are_using_REF_CLK_SMA false,SMA_channel TX2A");
          }
          else
          {
            usage(argv[0],"Error: invalid SMA channel specified.\n");
          }
        }
      }
    }

    //Prompt the user for rx settings
    //Verify rx inputs are within valid ranges
    if ((mode == "rx" || mode == "txrx") and
        ((currentFrontend == FMCOMMS2Frontend) or
         (currentFrontend == FMCOMMS3Frontend)))
    {
      // set known-good defaults for FMCOMMS2/3
      rx_sample_rate = 4.;
      rx_rf_center_freq = 2400.;
      rx_rf_bw = -1.;
      rx_rf_gain = 24.;
      rx_bb_bw = 4.;
      rx_bb_gain = -1.;

      PROMPT(app, rx_sample_rate,    "rx", "sample_rate_min_MHz",         "sample_rate_max_MHz"         );
      app.setPropertyValue<double>(  "rx", "sample_rate_MHz",             rx_sample_rate);

      PROMPT(app, rx_rf_center_freq, "rx", "frequency_min_MHz",           "frequency_max_MHz"           );
      app.setPropertyValue<double>(  "rx", "frequency_MHz",               rx_rf_center_freq);

      PROMPT(app, rx_rf_bw,          "rx", "rf_cutoff_frequency_min_MHz", "rf_cutoff_frequency_max_MHz" );
      app.setPropertyValue<double>(  "rx", "rf_cutoff_frequency_MHz",     rx_rf_bw);

      PROMPT(app, rx_rf_gain,        "rx", "rf_gain_min_dB",              "rf_gain_max_dB"             );
      app.setPropertyValue<double>(  "rx", "rf_gain_dB",                  rx_rf_gain);

      PROMPT(app, rx_bb_bw,          "rx", "bb_cutoff_frequency_min_MHz", "bb_cutoff_frequency_max_MHz");
      app.setPropertyValue<double>(  "rx", "bb_cutoff_frequency_MHz",     rx_bb_bw);

      PROMPT(app, rx_bb_gain,        "rx", "bb_gain_min_dB",              "bb_gain_max_dB"             );
      app.setPropertyValue<double>(  "rx", "bb_gain_dB",                  rx_bb_gain);

      rx_if_center_freq_min = rx_sample_rate*-0.5;
      rx_if_center_freq_max = rx_sample_rate*32767/65536;
      rx_if_center_freq = 0; // set default for prompt

      prompt_with_range(rx_if_center_freq, "rx_if_center_freq", "Error: invalid rx_if_center_freq.\n", rx_if_center_freq_min, rx_if_center_freq_max);

      // It is desired that setting a + IF freq results in mixing *down*.
      // Because complex_mixer's NCO mixes *up* for + freqs (see complex mixer
      // datasheet), IF tune freq must be negated in order to achieve the
      // desired effect.
      double nco_output_freq = -rx_if_center_freq;

      // todo this math might be better off in a small proxy that sits on top of complex_mixer
      // from complex mixer datasheet, nco_output_freq =
      // sample_freq * phs_inc / 2^phs_acc_width, phs_acc_width is fixed at 16
      OA::Short phase_inc = round(nco_output_freq/rx_sample_rate*65536.);

      if(phase_inc == 0) {
        app.setProperty("complex_mixer", "enable", "false");
      }
      else {
        //std::cout << "setting complex mixer phase_inc = " << phase_inc <<"\n";
        app.setPropertyValue<OA::Short>("complex_mixer","phs_inc", phase_inc);
      }
    }

    //Prompt the user for tx settings
    //Verify tx inputs are within valid ranges
    if ((mode == "tx" || mode == "txrx") and
        ((currentFrontend == FMCOMMS2Frontend) or
         (currentFrontend == FMCOMMS3Frontend)))
    {
      // set known-good defaults for FMCOMMS2/3
      tx_sample_rate = 4.;
      tx_rf_center_freq = 2400.;
      double tx_rf_bw = -1.;
      tx_rf_gain = -34.;
      tx_bb_bw = 4.;
      tx_bb_gain = -1.;

      PROMPT(app, tx_sample_rate,    "tx", "sample_rate_min_MHz",         "sample_rate_max_MHz"        );
      app.setPropertyValue<double>(  "tx", "sample_rate_MHz",             tx_sample_rate);

      PROMPT(app, tx_rf_center_freq, "tx", "frequency_min_MHz",           "frequency_max_MHz"          );
      app.setPropertyValue<double>(  "tx", "frequency_MHz",               tx_rf_center_freq);

      PROMPT(app, tx_rf_bw,          "tx", "rf_cutoff_frequency_min_MHz", "rf_cutoff_frequency_max_MHz");
      app.setPropertyValue<double>(  "tx", "rf_cutoff_frequency_MHz",     tx_rf_bw);

      PROMPT(app, tx_rf_gain,         "tx", "rf_gain_min_dB",              "rf_gain_max_dB"             );
      app.setPropertyValue<double>(  "tx", "rf_gain_dB",                  tx_rf_gain);

      PROMPT(app, tx_bb_bw,          "tx", "bb_cutoff_frequency_min_MHz", "bb_cutoff_frequency_max_MHz");
      app.setPropertyValue<double>(  "tx", "bb_cutoff_frequency_MHz",     tx_bb_bw);

      PROMPT(app, tx_bb_gain,        "tx", "bb_gain_min_dB",              "bb_gain_max_dB"             );
      app.setPropertyValue<double>(  "tx", "bb_gain_dB",                  tx_bb_gain);
    }

    //Verify rx inputs are within valid ranges
    if ((mode == "rx" || mode == "txrx") and
        ((currentFrontend == zipperFrontend) or
         (currentFrontend == matchstiq_z1_Frontend)))
    {
      double rx_sample_rate_min_MHz, rx_sample_rate_max_MHz;
      double rx_frequency_min_MHz, rx_frequency_max_MHz;
      double rx_rf_cutoff_frequency_min_MHz, rx_rf_cutoff_frequency_max_MHz;
      double rx_rf_gain_min_dB, rx_rf_gain_max_dB;
      double rx_bb_cutoff_frequency_min_MHz, rx_bb_cutoff_frequency_max_MHz;
      double rx_bb_gain_min_dB, rx_bb_gain_max_dB;

      app.getProperty("rx","sample_rate_min_MHz", value);
      rx_sample_rate_min_MHz = mode == "txrx" ? 2 : atof(value.c_str());
      app.getProperty("rx","sample_rate_max_MHz", value);
      rx_sample_rate_max_MHz = atof(value.c_str());
      if (rx_sample_rate < rx_sample_rate_min_MHz || rx_sample_rate > rx_sample_rate_max_MHz)
      {
        printLimits("Error: invalid rx_sample_rate.\n", rx_sample_rate, rx_sample_rate_min_MHz, rx_sample_rate_max_MHz);
      }

      app.getProperty("rx","frequency_min_MHz", value);
      rx_frequency_min_MHz = atof(value.c_str());
      app.getProperty("rx","frequency_max_MHz", value);
      rx_frequency_max_MHz = atof(value.c_str());
      if (rx_rf_center_freq < rx_frequency_min_MHz || rx_rf_center_freq > rx_frequency_max_MHz)
      {
        printLimits("Error: invalid rx_rf_center_freq.\n", rx_rf_center_freq, rx_frequency_min_MHz, rx_frequency_max_MHz);
      }

      OA::Property prop_rf_cutoff_min(app, "rx", "rf_cutoff_frequency_min_MHz");
      rx_rf_cutoff_frequency_min_MHz = prop_rf_cutoff_min.getValue<double>();
      OA::Property prop_rf_cutoff_max(app, "rx", "rf_cutoff_frequency_max_MHz");
      rx_rf_cutoff_frequency_max_MHz = prop_rf_cutoff_max.getValue<double>();
      if (rx_rf_bw < rx_rf_cutoff_frequency_min_MHz || rx_rf_bw > rx_rf_cutoff_frequency_max_MHz) {
        printLimits("Error: invalid rx_rf_bw.\n", rx_rf_bw, rx_rf_cutoff_frequency_min_MHz, rx_rf_cutoff_frequency_max_MHz);
      }

      app.getProperty("rx","rf_gain_min_dB", value);
      rx_rf_gain_min_dB = atof(value.c_str());
      app.getProperty("rx","rf_gain_max_dB", value);
      rx_rf_gain_max_dB = atof(value.c_str());
      if (rx_rf_gain < rx_rf_gain_min_dB || rx_rf_gain > rx_rf_gain_max_dB)
      {
        printLimits("Error: invalid rx_rf_gain.\n", rx_rf_gain, rx_rf_gain_min_dB, rx_rf_gain_max_dB);
      }

      app.getProperty("rx","bb_cutoff_frequency_min_MHz", value);
      rx_bb_cutoff_frequency_min_MHz = atof(value.c_str());
      app.getProperty("rx","bb_cutoff_frequency_max_MHz", value);
      rx_bb_cutoff_frequency_max_MHz = atof(value.c_str());
      if (rx_bb_bw < rx_bb_cutoff_frequency_min_MHz || rx_bb_bw > rx_bb_cutoff_frequency_max_MHz)
      {
        printLimits("Error: invalid rx_bb_bw.\n", rx_bb_bw, rx_bb_cutoff_frequency_min_MHz, rx_bb_cutoff_frequency_max_MHz);
      }

      app.getProperty("rx","bb_gain_min_dB", value);
      rx_bb_gain_min_dB = atof(value.c_str());
      app.getProperty("rx","bb_gain_max_dB", value);
      rx_bb_gain_max_dB = atof(value.c_str());
      if (rx_bb_gain < rx_bb_gain_min_dB || rx_bb_gain > rx_bb_gain_max_dB)
      {
        printLimits("Error: invalid rx_bb_gain.\n", rx_bb_gain, rx_bb_gain_min_dB, rx_bb_gain_max_dB);
      }

      if (rx_if_center_freq < rx_if_center_freq_min || rx_if_center_freq > rx_if_center_freq_max)
      {
        printLimits("Error: invalid rx_if_center_freq.\n", rx_if_center_freq, rx_if_center_freq_min, rx_if_center_freq_max);
      }
    }

    //Verify tx inputs are within valid ranges
    if ((mode == "tx" || mode == "txrx") and
        ((currentFrontend == zipperFrontend) or
         (currentFrontend == matchstiq_z1_Frontend)))
    {
      double tx_sample_rate_min_MHz, tx_sample_rate_max_MHz;
      double tx_frequency_min_MHz, tx_frequency_max_MHz;
      double tx_rf_gain_min_dB, tx_rf_gain_max_dB;
      double tx_bb_cutoff_frequency_min_MHz, tx_bb_cutoff_frequency_max_MHz;
      double tx_bb_gain_min_dB, tx_bb_gain_max_dB;

      app.getProperty("tx","sample_rate_min_MHz", value);
      tx_sample_rate_min_MHz = mode == "txrx" ? 2 : atof(value.c_str());
      app.getProperty("tx","sample_rate_max_MHz", value);
      tx_sample_rate_max_MHz = atof(value.c_str());
      if (tx_sample_rate < tx_sample_rate_min_MHz || tx_sample_rate > tx_sample_rate_max_MHz)
      {
        printLimits("Error: invalid tx_sample_rate.\n", tx_sample_rate, tx_sample_rate_min_MHz, tx_sample_rate_max_MHz);
      }

      app.getProperty("tx","frequency_min_MHz", value);
      tx_frequency_min_MHz = atof(value.c_str());
      app.getProperty("tx","frequency_max_MHz", value);
      tx_frequency_max_MHz = atof(value.c_str());
      if (tx_rf_center_freq < tx_frequency_min_MHz || tx_rf_center_freq > tx_frequency_max_MHz)
      {
        printLimits("Error: invalid tx_rf_center_freq.\n", tx_rf_center_freq, tx_frequency_min_MHz, tx_frequency_max_MHz);
      }

      app.getProperty("tx","rf_gain_min_dB", value);
      tx_rf_gain_min_dB = atof(value.c_str());
      app.getProperty("tx","rf_gain_max_dB", value);
      tx_rf_gain_max_dB = atof(value.c_str());
      if (tx_rf_gain < tx_rf_gain_min_dB || tx_rf_gain > tx_rf_gain_max_dB)
      {
        printLimits("Error: invalid tx_rf_gain.\n", tx_rf_gain, tx_rf_gain_min_dB, tx_rf_gain_max_dB);
      }

      app.getProperty("tx","bb_cutoff_frequency_min_MHz", value);
      tx_bb_cutoff_frequency_min_MHz = atof(value.c_str());
      app.getProperty("tx","bb_cutoff_frequency_max_MHz", value);
      tx_bb_cutoff_frequency_max_MHz = atof(value.c_str());
      if (tx_bb_bw < tx_bb_cutoff_frequency_min_MHz || tx_bb_bw > tx_bb_cutoff_frequency_max_MHz)
      {
        printLimits("Error: invalid tx_bb_bw.\n", tx_bb_bw, tx_bb_cutoff_frequency_min_MHz, tx_bb_cutoff_frequency_max_MHz);
      }

      app.getProperty("tx","bb_gain_min_dB", value);
      tx_bb_gain_min_dB = atof(value.c_str());
      app.getProperty("tx","bb_gain_max_dB", value);
      tx_bb_gain_max_dB = atof(value.c_str());
      if (tx_bb_gain < tx_bb_gain_min_dB || tx_bb_gain > tx_bb_gain_max_dB)
      {
        printLimits("Error: invalid tx_bb_gain.\n", tx_bb_gain, tx_bb_gain_min_dB, tx_bb_gain_max_dB);
      }
    }

    const bool currentFrontendUsesLimeTransceiver = (currentFrontend==matchstiq_z1_Frontend)
                                      or (currentFrontend==zipperFrontend);
    //Verify Lime-specific TXRX frequency limitation
    if ((mode == "txrx") and
        currentFrontendUsesLimeTransceiver)
    {
      if (fabs(rx_rf_center_freq - tx_rf_center_freq) < 1)
      {
	fprintf(stderr,"Error: RX and TX frequencies must be at least 1 MHz apart. Delta is %f\n",fabs(rx_rf_center_freq - tx_rf_center_freq));
	exit(1);
      }
      if (rx_sample_rate < 2 || tx_sample_rate < 2)
      {
	fprintf(stderr,"Error: In TXRX mode, RX and TX sample rates must be greater than 2 MS/s. Current settings are %f/%f\n",rx_sample_rate,tx_sample_rate);
	exit(1);
      }
      if (rx_if_center_freq < 1)
      {
	fprintf(stderr,"Error: In TXRX mode, RX IF center frequency must be greater than 1 MHz. Current setting is %f\n",rx_if_center_freq);
	exit(1);
      }
    }

    //Verify runtime is within a valid range
    if (mode == "rx" || mode == "tx" || mode == "txrx" || mode == "bbloopback")
    {
      if (runtime < 1 || runtime > 60)
      {
        printLimits("Error: invalid runtime.\n", runtime, 1, 60);
      }
    }

    //Setup rx front end
    if ( ((mode == "rx" || mode == "txrx" || mode == "bbloopback") and
          ((currentFrontend == zipperFrontend) or
           (currentFrontend == matchstiq_z1_Frontend))) or
         ((mode == "bbloopback") and
          ((currentFrontend == FMCOMMS2Frontend) or
           (currentFrontend == FMCOMMS3Frontend))) )
    {
      if( (mode == "bbloopback") and
          ((currentFrontend == FMCOMMS2Frontend) or
           (currentFrontend == FMCOMMS3Frontend)))
      {
        rx_sample_rate = 40.;
        tx_rf_center_freq = 2400.;
        rx_rf_bw = -1.; // must be disabled
        rx_rf_gain = 12.; // chosen to be some reasonable value
        rx_bb_bw = rx_sample_rate;
        rx_bb_gain = -1.; // must be disabled
      }
      app.setPropertyValue<double>("rx","sample_rate_MHz", rx_sample_rate);
      app.setPropertyValue<double>("rx","frequency_MHz", rx_rf_center_freq);
      app.setPropertyValue<double>("rx","rf_cutoff_frequency_MHz", rx_rf_bw);
      app.setPropertyValue<double>("rx","rf_gain_dB", rx_rf_gain);
      app.setPropertyValue<double>("rx","bb_cutoff_frequency_MHz", rx_bb_bw);
      app.setPropertyValue<double>("rx","bb_gain_dB", rx_bb_gain);

      // It is desired that setting a + IF freq results in mixing *down*.
      // Because complex_mixer's NCO mixes *up* for + freqs (see complex mixer
      // datasheet), IF tune freq must be negated in order to achieve the
      // desired effect.
      double nco_output_freq = -rx_if_center_freq;

      // todo this math might be better off in a small proxy that sits on top of complex_mixer
      // from complex mixer datasheet, nco_output_freq =
      // sample_freq * phs_inc / 2^phs_acc_width, phs_acc_width is fixed at 16
      OA::Short phase_inc = round(nco_output_freq/rx_sample_rate*65536.);

      if(phase_inc == 0) {
        app.setProperty("complex_mixer", "enable", "false");
      }
      else {
        //std::cout << "setting complex mixer phase_inc = " << phase_inc <<"\n";
        app.setPropertyValue<OA::Short>("complex_mixer","phs_inc", phase_inc);
      }
    }

    //Setup tx front end
    if ( ((mode == "tx" || mode == "txrx" || mode == "bbloopback") and
          ((currentFrontend == zipperFrontend) or
           (currentFrontend == matchstiq_z1_Frontend))) or
         ((mode == "bbloopback") and
          ((currentFrontend == FMCOMMS2Frontend) or
           (currentFrontend == FMCOMMS3Frontend))) )
    {
      if( (mode == "bbloopback") and
          ((currentFrontend == FMCOMMS2Frontend) or
           (currentFrontend == FMCOMMS3Frontend)))
      {
        tx_sample_rate = 40.;
        tx_rf_center_freq = 2400.;
        app.setProperty("tx","rf_cutoff_frequency_MHz", "-1"); // must be disabled
        tx_rf_gain = -18.; // chosen to be some reasonable value
        tx_bb_bw = tx_sample_rate;
        tx_bb_gain = -1.; // must be disabled
      }
      app.setPropertyValue<double>("tx","sample_rate_MHz", tx_sample_rate);
      app.setPropertyValue<double>("tx","frequency_MHz", tx_rf_center_freq);
      app.setPropertyValue<double>("tx","rf_gain_dB", tx_rf_gain);
      app.setPropertyValue<double>("tx","bb_cutoff_frequency_MHz", tx_bb_bw);
      app.setPropertyValue<double>("tx","bb_gain_dB", tx_bb_gain);
    }

    //Loopback mode for txrx app
    if (mode == "bbloopback")
    {
      //Disable complex mixer, dc offset filter
      app.setProperty("complex_mixer", "enable", "false");
      app.setProperty("dc_offset_filter","bypass", "true");
      //Override filewrite name
      app.setProperty("file_write","fileName", "odata/out_app_fsk_bbloopback.bin");
    }
    else if ((mode == "tx" || mode == "txrx") and currentFrontendUsesLimeTransceiver)
    {
      //Set loopback switches to open
      app.setProperty("rf_tx","loopback", "0x00");
      app.setProperty("rf_tx","tx_pkdbw", "0x00");
      //Enable TX PA
      app.setProperty("rf_tx","tx_pa_en", "0x0B");
    }
    else if ((mode == "rx") and currentFrontendUsesLimeTransceiver)
    {
      //Set loopback switches to open
      app.setProperty("rf_rx","loopback", "0x00");
      app.setProperty("rf_rx","tx_pkdbw", "0x00");
      //Enable TX PA
      app.setProperty("rf_rx","tx_pa_en", "0x0B");
    }

    // dump all inital properties following initalization but before start
    if (debug_mode) {
      std::string name, value;
      bool isParameter, hex = false;
      fprintf(stderr, "Dump of all initial property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value, hex, &isParameter); n++)
      {
        fprintf(stderr, "Property %2u: %s = \"%s\"%s\n", n, name.c_str(), value.c_str(), isParameter ? " (parameter)" : "");
      }
    }

    app.start();
    printf("App started.\n");

    if ((mode == "bbloopback") and currentFrontendUsesLimeTransceiver)
    {
      //Disable TX PA
      app.setProperty("rf_tx","tx_pa_en", "0x03");
      //Disable LNAs
      app.setProperty("rf_rx","rxfe_cbe_lna", "0x00");
      //Set LOOPBBEN to output of TX LPF
      app.setProperty("rf_tx","loopback", "0x40");
      //Set LBEN_LPFIN
      app.setProperty("rf_tx","tx_pkdbw", "0x04");
      //Increase RX VGA2 gain to 30 dB
      app.setProperty("rf_tx","rx_vga2gain", "0x0A");
    }
    //! @todo TODO/FIXME - support digital AD9361 loopback (which is *not* "B"ase"B"and loopback)
    //const bool currentFrontendUsesAD9361Transceiver = (currentFrontend==FMCOMMS2Frontend)
    //                                  or (currentFrontend==FMCOMMS3Frontend);
    //if ((mode == "bbloopback") and currentFrontendUsesAD9361Transceiver)
    //{
    //  app.setProperty("ad9361_config_proxy","bist_loopback", "1");
    //}

    //Mode filerw waits for a zlm to complete. All others use runtime.
    if (mode == "rx" || mode == "tx" || mode == "txrx" || mode == "bbloopback")
    {
      printf("App runs for %f seconds...\n",runtime);
      while (runtime > 0)
      {
        sleep(1);
        runtime --;
        if (mode == "rx" || mode == "txrx" || mode == "bbloopback")
        {
          app.getProperty("qadc","overrun", value);
          if (strcmp("true", value.c_str()) == 0)
          {
            overrunFlag  = true;
          }
        }
      }
    }
    else
    {
      printf("Waiting for done signal from file_write.\n");
      app.wait();
    }

    app.stop();
    printf("App stopped.\n");


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
          fprintf(stderr, "Parameter property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
        }
      }
    }

    //Mode tx is the only one that doesn't write to a file
    if (mode == "rx" || mode == "txrx" || mode == "filerw" || mode == "bbloopback")
    {
      app.getProperty("file_write","bytesWritten", value);
      printf("Bytes to file : %s\n", value.c_str());
      if (overrunFlag)
      {
        printf("WARNING: RX sample rate was high enough that data could not be written to file fast enough, resulting in samples being dropped, try a lower RX sample rate\n");
      }
    }

    //Query tx peak values
    if (mode == "tx" || mode == "txrx" || mode == "bbloopback")
    {
      printf("TX Bit Rate            = %f bps\n",get_tx_bit_rate_bps(app));
    }
    if (mode == "tx" || mode == "txrx" || mode == "filerw" || mode == "bbloopback")
    {
      app.getProperty("tx_fir_real","peak", value);
      printf("TX FIR Real Peak       = %s\n", value.c_str());
      app.getProperty("phase_to_amp_cordic","magnitude", value);
      printf("Phase to Amp Magnitude = %s\n", value.c_str());

    }

    //Query rx peak values
    if (mode == "rx" || mode == "txrx" || mode == "filerw" || mode == "bbloopback")
    {
      if (mode != "filerw")
      {
        app.getProperty("dc_offset_filter","peak", value);
        printf("DC Offset Peak         = %s\n", value.c_str());
        app.getProperty("iq_imbalance_fixer","peak", value);
        printf("IQ Imbalance Peak      = %s\n", value.c_str());
        app.getProperty("complex_mixer","peak", value);
        printf("Complex Mixer Peak     = %s\n", value.c_str());
        app.getProperty("qadc","overrun", value);
        printf("ADC Samples Dropped    = %s\n", value.c_str());
      }
      app.getProperty("rp_cordic","magnitude", value);
      printf("RP Cordic Magnitude    = %s\n", value.c_str());
      app.getProperty("rx_fir_real","peak", value);
      printf("RX FIR Real Peak       = %s\n", value.c_str());
    }

    printf("Application complete\n");

  } catch (std::string &e) {
    fprintf(stderr, "Exception thrown: %s\n", e.c_str());
    return 1;
  }

  return 0;
}
