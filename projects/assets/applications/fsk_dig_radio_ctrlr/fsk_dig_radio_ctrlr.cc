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

#include <iostream>
#include <unistd.h>
#include <cstdio>
#include <cassert>
#include <string>
#include <cstdlib> // EXIT_SUCCESS, EXIT_FAILURE, strtod(), atoi()
#include <sstream> // std::ostringstream
#include <cerrno> // errno
#include <stdio.h> // perror()
#include <string.h> // perror(), strerror()
#include <cmath> // round()
#include "OcpiApi.hh"
#include "OcpiOsDebugApi.h" // OCPI::OS::logPrint(), OCPI_LOG_BAD

namespace OA = OCPI::API;

// tweak only if needed
#define APP_EXTRA_RUNTIME_FACTOR 1.1 // run app 10% longer than expected TX/RX
                                     // time, ...
#define APP_EXTRA_RUNTIME_SEC 1.     // ... plus an additional 1 second
#define FS_TOLERANCE_PERCENT 0.5 // as percent of fs (workaround for not implementing --tolerance_*x_baud_rate)
#define BW_TOLERANCE_PERCENT 0.5 // as percent of bw (workaround for not implementing --tolerance_*x_baud_rate)
#define OS_JPEG_SIZE_BYTES 9194

#define APP "fsk_dig_radio_ctrlr"

#define PARSE_ARG(arg) \
  if(ii+1 >= argc) { \
    usage(); \
    std::cerr << "ERROR: --" #arg " must be followed by a value\n"; \
    return EXIT_FAILURE; \
  } \
  app_main_args.arg = argv[ii+1]; \
  args_processed.arg = 1;

#define PARSE_ARG_STR(arg) \
  if(ii+1 >= argc) { \
    usage(); \
    std::cerr << "ERROR: --" #arg " must be followed by a value\n"; \
    return EXIT_FAILURE; \
  } \
  app_main_args.arg.assign(argv[ii+1]); \
  args_processed.arg = 1;

#define PARSE_ARG_DOUBLE(arg) \
  if(ii+1 >= argc) { \
    usage(); \
    std::cerr << "ERROR: --" #arg " must be followed by a value\n"; \
    return EXIT_FAILURE; \
  } \
  app_main_args.arg = strtod(argv[ii+1], NULL); \
  if(errno == ERANGE) { \
    perror(strerror(errno)); \
    return EXIT_FAILURE; \
  } \
  args_processed.arg = 1;

#define ERR_IF_MISSING_ARG_D(arg) \
  if(args_processed.arg == 0) { \
    usage(); \
    std::cerr << "ERROR: missing argument: -" #arg "\n"; \
    return EXIT_FAILURE; \
  }

#define ERR_IF_MISSING_ARG_DD(arg, var) \
  if(args_processed.var == 0) { \
    usage(); \
    std::cerr << "ERROR: missing argument: --" #arg "\n"; \
    return EXIT_FAILURE; \
  }

struct app_main_args_t {
  std::string  rx_data_stream_ID;
  double       rx_baud_rate;
  std::string  rx_tuning_freq_MHz;
  std::string  rx_gain_mode;
  std::string  rx_gain_dB;

  // tolerance
  //double       tol_rx_baud_rate;
  std::string  tol_rx_tuning_freq_MHz;
  std::string  tol_rx_gain_dB;

  std::string  tx_data_stream_ID;
  double       tx_baud_rate;
  std::string  tx_tuning_freq_MHz;
  std::string  tx_gain_mode;
  std::string  tx_gain_dB;

  // tolerance
  //double       tol_tx_baud_rate;
  std::string  tol_tx_tuning_freq_MHz;
  std::string  tol_tx_gain_dB;

  bool d; // dump properties
  unsigned int l; // log level
  bool U; // uncached
  std::string app_xml;
};

struct app_main_args_processed_t {
  int rx_data_stream_ID;
  int rx_baud_rate;
  int rx_tuning_freq_MHz;
  int rx_gain_mode;
  int rx_gain_dB;

  // tolerance
  //int tol_rx_baud_rate;
  int tol_rx_tuning_freq_MHz;
  int tol_rx_gain_dB;

  int tx_data_stream_ID;
  int tx_baud_rate;
  int tx_tuning_freq_MHz;
  int tx_gain_mode;
  int tx_gain_dB;

  // tolerance
  //int tol_tx_baud_rate; // tolerance
  int tol_tx_tuning_freq_MHz;
  int tol_tx_gain_dB;

  int d;
  int l;
  int t;
  int U;
  int app_xml;
};

namespace OA = OCPI::API;

app_main_args_t app_main_args;
OA::Application* p_app;
static bool overrun = false;
static bool underrun = false;

void version() {
  std::cout << APP << " develop\n";
}

void usage() {
  version();
  std::cout << "usage: " << APP << " [--version] [--help]\n";
  std::cout << "                           [--rx-data-stream-ID <arg>]";
  std::cout << " [--rx-baud-rate <arg>]\n";
  std::cout << "                           [--rx-tuning-freq-MHz <arg>]";
  std::cout << " [--rx-gain-mode <arg>]\n";
  std::cout << "                           [--rx-gain-dB <arg>]";
/*  std::cout << "              [--tolerance-rx-baud-rate <arg>]";
  std::cout << " [--tolerance-rx-tuning-freq-MHz <arg>]\n";
  std::cout << "              [--tolerance-rx-gain-dB <arg>]";
  std::cout << " [--tx-data-stream-ID <arg>]\n";
  std::cout << "              [--tx-baud-rate <arg>]";
  std::cout << " [--tx-tuning-freq-MHz <arg>]\n";
  std::cout << "              [--tx-gain-mode <arg>]";
  std::cout << " [--tx-gain-dB <arg>]\n";
  std::cout << "              [--tx-tolerance-baud-rate <arg>]";
  std::cout << " [--tx-tolerance-tuning-freq-MHz <arg>]\n";
  std::cout << "              [--tx-tolerance-gain-dB <arg>]";*/
  std::cout << " [--tolerance-rx-tuning-freq-MHz <arg>]\n";
  std::cout << "                           [--tolerance-rx-gain-dB <arg>]";
  std::cout << " [--tx-data-stream-ID <arg>]\n";
  std::cout << "                           [--tx-baud-rate <arg>]";
  std::cout << " [--tx-tuning-freq-MHz <arg>]\n";
  std::cout << "                           [--tx-gain-mode <arg>]";
  std::cout << " [--tx-gain-dB <arg>]\n";
  std::cout << "                           [--tx-tolerance-tuning-freq-MHz <arg>]";
  std::cout << " [--tx-tolerance-gain-dB <arg>]\n";
  std::cout << "                           [-d]";
  std::cout << " [-U]";
  std::cout << " <app-xml>\n\n";
  std::cout << " --rx-data-stream-ID <arg>      Set desired RX data stream ID\n";
  std::cout << "                                (required if using rx or txrx mode)\n";
  std::cout << " --rx-baud-rate <arg>           (required if using rx or txrx mode)\n";
  std::cout << " --rx-tuning-freq-MHz <arg>     (required if using rx or txrx mode)\n";
  std::cout << " --rx-gain-mode <arg>           Standard values are auto or manual, \n";
  std::cout << "                                hardware-specific values may also be \n";
  std::cout << "                                permissible\n";
  std::cout << "                                (required if using rx or txrx mode)\n";
  std::cout << " --rx-gain-dB <arg>             will be ignored if --gain-mode is anything \n";
  std::cout << "                                other than manual\n";
  std::cout << "                                (required if using rx or txrx mode)\n";
  //std::cout << " --tolerance-rx-baud-rate       +/- this tolerance in Msps will determine\n";
  //std::cout << "                                radio configuration success\n";
  //std::cout << "                                (required if using rx or txrx mode)\n";
  std::cout << " --tolerance-rx-tuning-freq-MHz <arg> +/- this tolerance in MHz will determine\n";
  std::cout << "                                radio configuration success\n";
  std::cout << "                                (required if using rx or txrx mode).\n";
  std::cout << " --tolerance-rx-gain-dB <arg>   +/- this tolerance in dB will determine\n";
  std::cout << "                                radio configuration success\n";
  std::cout << "                                (will be ignored if --gain-mode is anything \n";
  std::cout << "                                other than manual)\n";
  std::cout << "                                (required if using rx or txrx mode)\n";
  std::cout << " --tx-data-stream-ID <arg>      Set desired RX data stream ID\n";
  std::cout << "                                (required if using tx or txrx mode)\n";
  std::cout << " --tx-baud-rate <arg>           (required if using tx or txrx mode)\n";
  std::cout << " --tx-tuning-freq-MHz <arg>     (required if using tx or txrx mode)\n";
  std::cout << " --tx-gain-mode <arg>           Standard values are auto or manual, \n";
  std::cout << "                                hardware-specific values may also be \n";
  std::cout << "                                permissible (typically set to manual)\n";
  std::cout << "                                (required if using tx or txrx mode)\n";
  std::cout << " --tx-gain-dB <arg>             will be ignored if --gain-mode is anything \n";
  std::cout << "                                other than manual\n";
  std::cout << "                                (required if using tx or txrx mode)\n";
  //std::cout << " --tolerance-tx-baud-rate       +/- this tolerance in Msps will determine\n";
  //std::cout << "                                radio configuration success\n";
  //std::cout << "                                (required if using tx or txrx mode)\n";
  std::cout << " --tolerance-tx-tuning-freq-MHz <arg> +/- this tolerance in MHz will determine\n";
  std::cout << "                                radio configuration success\n";
  std::cout << "                                (required if using tx or txrx mode).\n";
  std::cout << " --tolerance-tx-gain-dB <arg>   +/- this tolerance in dB will determine\n";
  std::cout << "                                radio configuration success\n";
  std::cout << "                                (will be ignored if --gain-mode is anything \n";
  std::cout << "                                other than manual)\n";
  std::cout << "                                (required if using tx or txrx mode)\n";
  std::cout << " -d                             dump properties\n";
  std::cout << " -U                             dump cached properties uncached, ignoring cache\n";
  std::cout << " <app-xml>                      (always required)\n";
}

bool string_ends_in(const std::string& testee, std::string ending) {
  if(testee.length() >= ending.length()) {
    auto elen = ending.length();
    return (0 == testee.compare(testee.length()-elen, elen, ending));
  }
  else {
    return false;
  }
}

bool mode_is_rx() {
  return string_ends_in(app_main_args.app_xml, "_rx.xml");
}

bool mode_is_tx() {
  return string_ends_in(app_main_args.app_xml, "_tx.xml");
}

bool mode_is_txrx() {
  return string_ends_in(app_main_args.app_xml, "_txrx.xml");
}

bool mode_is_filerw() {
  return string_ends_in(app_main_args.app_xml, "_filerw.xml");
}

bool mode_requires_rx() {
  return mode_is_rx() or mode_is_txrx();
}

bool mode_requires_tx() {
  return mode_is_tx() or mode_is_txrx();
}

bool app_xml_is_valid() {
  return mode_is_rx() or mode_is_tx() or mode_is_txrx() or mode_is_filerw();
}

/// @brief "enough", which is not necessarily "all possible"
int ensure_enough_args_are_processed(
    const app_main_args_processed_t& args_processed) {

  if(mode_requires_rx()) {
    ERR_IF_MISSING_ARG_DD(rx-data-stream-ID, rx_data_stream_ID)
    ERR_IF_MISSING_ARG_DD(rx-baud-rate, rx_baud_rate)
    ERR_IF_MISSING_ARG_DD(rx-tuning-freq-MHz, rx_tuning_freq_MHz)
    ERR_IF_MISSING_ARG_DD(rx-gain-mode, rx_gain_mode)
    if(app_main_args.rx_gain_mode.compare("manual") == 0) {
      if(args_processed.rx_gain_dB == 0) {
        std::ostringstream oss;
        usage();
        oss << "ERROR: because manual rx gain mode was requested, --rx-gain-dB argument ";
        oss << "must be included\n";
        std::cerr << oss.str().c_str();
        return EXIT_FAILURE;
      }
    }
    else {
      // dig_radio_ctrlr.rcc should ignore this value, initialize it to a
      // reasonable value (0) just in case
      app_main_args.rx_gain_dB.assign("0");
      if(args_processed.rx_gain_dB != 0) {
        std::ostringstream oss;
        oss << "WARN : included --gain-dB argument which will be ignored ";
        oss << "because --gain-mode was auto\n";
        std::cout << oss.str().c_str();
      }
    }
    ERR_IF_MISSING_ARG_DD(tolerance-rx-tuning-freq-MHz, tol_rx_tuning_freq_MHz)
    if(app_main_args.rx_gain_mode.compare("manual") == 0) {
      if(args_processed.tol_rx_gain_dB == 0) {
        std::ostringstream oss;
        usage();
        oss << "ERROR: because manual mode was requested, --rx-tolerance-gain-dB ";
        oss << "argument must be included\n";
        std::cerr << oss.str().c_str();
        return EXIT_FAILURE;
      }
    }
    else {
      // dig_radio_ctrlr.rcc should ignore this value, initialize it to a
      // reasonable value (1) just in case
      app_main_args.tol_rx_gain_dB.assign("0");
      if(args_processed.tol_rx_gain_dB != 0) {
        std::ostringstream oss;
        oss << "WARN : included --rx-tolerance-gain-dB argument which will be ";
        oss << "ignored because --rx-gain-mode was auto\n";
        std::cout << oss.str().c_str();
      }
    }
  }
  if(mode_requires_tx()) {
    ERR_IF_MISSING_ARG_DD(tx-data-stream-ID, tx_data_stream_ID)
    ERR_IF_MISSING_ARG_DD(tx-baud-rate, tx_baud_rate)
    ERR_IF_MISSING_ARG_DD(tx-tuning-freq-MHz, tx_tuning_freq_MHz)
    ERR_IF_MISSING_ARG_DD(tx-gain-mode, tx_gain_mode)
    if(app_main_args.tx_gain_mode.compare("manual") == 0) {
      if(args_processed.tx_gain_dB == 0) {
        std::ostringstream oss;
        usage();
        oss << "ERROR: because manual tx gain mode was requested, --tx-gain-dB argument ";
        oss << "must be included\n";
        std::cerr << oss.str().c_str();
        return EXIT_FAILURE;
      }
    }
    else {
      // dig_radio_ctrlr.rcc should ignore this value, initialize it to a
      // reasonable value (0) just in case
      app_main_args.tx_gain_dB.assign("0");
      if(args_processed.tx_gain_dB != 0) {
        std::ostringstream oss;
        oss << "WARN : included --gain-dB argument which will be ignored ";
        oss << "because --gain-mode was auto\n";
        std::cout << oss.str().c_str();
      }
    }
    ERR_IF_MISSING_ARG_DD(tolerance-tx-tuning-freq-MHz, tol_tx_tuning_freq_MHz)
    if(app_main_args.tx_gain_mode.compare("manual") == 0) {
      if(args_processed.tol_tx_gain_dB == 0) {
        std::ostringstream oss;
        usage();
        oss << "ERROR: because manual mode was requested, --tx-tolerance-gain-dB ";
        oss << "argument must be included\n";
        std::cerr << oss.str().c_str();
        return EXIT_FAILURE;
      }
    }
    else {
      // dig_radio_ctrlr.rcc should ignore this value, initialize it to a
      // reasonable value (1) just in case
      app_main_args.tol_rx_gain_dB.assign("1");
      if(args_processed.tol_rx_gain_dB != 0) {
        std::ostringstream oss;
        oss << "WARN : included --rx-tolerance-gain-dB argument which will be ";
        oss << "ignored because --rx-gain-mode was auto\n";
        std::cout << oss.str().c_str();
      }
    }
  }
  if(args_processed.app_xml == 0) {
    usage();
    std::cerr << "ERROR: missing app xml as last argument\n";
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int parse_args(int argc, char **argv) {
  app_main_args.d = false; // set default to no dump of properties

  //app_main_args_processed_t args_processed = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
  app_main_args_processed_t args_processed = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

  int ii=1; // start parsing at arg 1 (ignore arg 0)
  while(ii < argc) {
    int num_args_processed = 2;
    if(strcmp(argv[ii],"--rx-data-stream-ID") == 0) {
      PARSE_ARG_STR(rx_data_stream_ID)
    }
    else if(strcmp(argv[ii],"--rx-baud-rate") == 0) {
      PARSE_ARG_DOUBLE(rx_baud_rate)
    }
    else if(strcmp(argv[ii],"--rx-tuning-freq-MHz") == 0) {
      PARSE_ARG_STR(rx_tuning_freq_MHz)
    }
    else if(strcmp(argv[ii],"--rx-gain-mode") == 0) {
      if(ii+1 >= argc) {
        usage();
        std::cerr << "ERROR: --rx-gain-mode must be followed by a value\n";
        return EXIT_FAILURE;
      }
      app_main_args.rx_gain_mode.assign(argv[ii+1]);
      args_processed.rx_gain_mode = 1;
    }
    else if(strcmp(argv[ii],"--rx-gain-dB") == 0) {
      PARSE_ARG_STR(rx_gain_dB)
    }
    //else if(strcmp(argv[ii],"--tolerance-rx-baud-rate") == 0) {
    //  PARSE_ARG_DOUBLE(tol_rx_baud_rate)
    //}
    else if(strcmp(argv[ii],"--tolerance-rx-tuning-freq-MHz") == 0) {
      PARSE_ARG_STR(tol_rx_tuning_freq_MHz)
    }
    else if(strcmp(argv[ii],"--tolerance-rx-gain-dB") == 0) {
      PARSE_ARG_STR(tol_rx_gain_dB)
    }
    else if(strcmp(argv[ii],"--tx-data-stream-ID") == 0) {
      PARSE_ARG_STR(tx_data_stream_ID)
    }
    else if(strcmp(argv[ii],"--tx-baud-rate") == 0) {
      PARSE_ARG_DOUBLE(tx_baud_rate)
    }
    else if(strcmp(argv[ii],"--tx-tuning-freq-MHz") == 0) {
      PARSE_ARG_STR(tx_tuning_freq_MHz)
    }
    else if(strcmp(argv[ii],"--tx-gain-mode") == 0) {
      if(ii+1 >= argc) {
        usage();
        std::cerr << "ERROR: --tx-gain-mode must be followed by a value\n";
        return EXIT_FAILURE;
      }
      app_main_args.tx_gain_mode.assign(argv[ii+1]);
      args_processed.tx_gain_mode = 1;
    }
    else if(strcmp(argv[ii],"--tx-gain-dB") == 0) {
      PARSE_ARG_STR(tx_gain_dB)
    }
    //else if(strcmp(argv[ii],"--tolerance-tx-baud-rate") == 0) {
    //  PARSE_ARG_DOUBLE(tol_tx_baud_rate)
    //}
    else if(strcmp(argv[ii],"--tolerance-tx-tuning-freq-MHz") == 0) {
      PARSE_ARG_STR(tol_tx_tuning_freq_MHz)
    }
    else if(strcmp(argv[ii],"--tolerance-tx-gain-dB") == 0) {
      PARSE_ARG_STR(tol_tx_gain_dB)
    }
    else if(strcmp(argv[ii],"-d") == 0) {
      app_main_args.d = true;
      args_processed.d = 1;
      num_args_processed = 1;
    }
    else if((strcmp(argv[ii],"-l") == 0) or (strcmp(argv[ii],"--log-level") == 0)) {
      if(ii+1 >= argc) {
        usage();
        std::cerr << "ERROR: " << argv[ii] << "must be followed by a value\n";
        return EXIT_FAILURE;
      }
      /// @todo / FIXME - this arg doesn't appear to be used anywhere yet
      app_main_args.l = atoi(argv[ii+1]);
      args_processed.l = 1;
    }
    else if(strcmp(argv[ii],"-U") == 0) {
      app_main_args.U = true;
      args_processed.U = 1;
      num_args_processed = 1;
    }
    else if(strcmp(argv[ii],"--version") == 0) {
      version();
      exit(EXIT_SUCCESS);
    }
    else if(strcmp(argv[ii],"--help") == 0) {
      usage();
      exit(EXIT_SUCCESS);
    }
    else if(ii == argc-1) {
      app_main_args.app_xml.assign(argv[ii]);
      if(not app_xml_is_valid()) {
        std::ostringstream ostr;
        ostr << "ERROR: " << app_main_args.app_xml << " is not supported by ";
        ostr << "this app (app-xml parameter must end in _rx.xml, _tx.xml, or ";
        ostr << "_filerw.xml in order to specify the mode to be used by the ";
        ostr << "ACI)\n";
        std::cerr << ostr.str();
        exit(EXIT_FAILURE);
      }
      args_processed.app_xml = 1;
    }
    else {
      usage();
      std::cerr << "ERROR: invalid argument specified: " << argv[ii] << "\n";
      return EXIT_FAILURE;
    }
    ii += num_args_processed;
  }

  int ret = ensure_enough_args_are_processed(args_processed);
  
  return ret;
}

/*void print_RX_data_stream_config() {
}*/

double get_rx_bits_per_sym() {
  return 1; // 1 bits/sym (2 possible logic levels) for binary FSK
}

double get_tx_bits_per_sym() {
  return 1; // 1 bits/sym (2 possible logic levels) for binary FSK
}

double get_rx_syms_per_baud() {
  return 1;
}

double get_tx_syms_per_baud() {
  return 1;
}

double get_rx_samps_per_sym() {
  // note that SPB is assumed to have been set in the app XML
  assert(p_app != 0);
  return (double) p_app->getPropertyValue<OA::UShort>("baudTracking", "SPB");
}

double get_tx_samps_per_sym() {
  const char* inst = "zero_pad";
  // num_zeros is assumed to have been set in the app XML
  assert(p_app != 0);
  return ((double)p_app->getPropertyValue<OA::UShort>(inst, "num_zeros")) + 1.;
}

/*// this function does not work correctly (bug AV-4885)
/// @todo / FIXME - loop over sequence length instead of fixed value of 1024
size_t get_idx_of_sampling_rate_Msps_data_stream_ID(const std::string& ds_ID) {

  const char* inst = "dig_radio_ctrlr"; // worker inst name
  const char* prop = "sampling_rate_Msps"; // prop name

  for(size_t ds_idx = 0; ds_idx < 2; ds_idx++) { // 1024 arbitrarily chosen

    // data stream ID for current iteration
    const char* prop_mem = "data_stream_ID"; // struct prop member
    assert(p_app != 0);

    // exemplifies bug AV-4885
    std::string s = p_app->getPropertyValue<std::string>(inst,prop,{ds_idx,prop_mem});

    std::cout << "ds_idx=" << ds_idx << "\n";
    std::cout << "s=" << s << "\n";
    if(ds_ID.compare(s) == 0) { // if desired data stream ID was found
      return ds_idx;
    }
  }

  std::string str("could not get sampling rate for dig_radio_ctrlr ");
  str += "data stream ID ";
  str += ds_ID;
  str += " because data stream ID could not be found";
  throw str;
}*/

///@brief Get prop value by data stream ID.
double get_dig_radio_ctrlr_prop_by_data_stream_ID(std::string ds_ID,
    const char* pr) {
  std::string value;
  const char* inst = "dig_radio_ctrlr"; // worker inst name
  
  /*size_t ds_idx = get_idx_of_sampling_rate_Msps_data_stream_ID(ds_ID);
  std::cout << "ds_ID=" << ds_ID << "\n";
  std::cout << "ds_idx=" << ds_idx << "\n";
  assert(p_app != 0);
  return p_app->getPropertyValue<double>(inst, pr, {ds_idx, pr});*/

  assert(p_app != 0);
  p_app->getProperty(inst, pr, value);
  // e.g. {data_stream_ID SMA_RX,sampling_rate_Msps 0.55},{data_stream_ID SMA_TX,sampling_rate_Msps 0.55}
  bool grab_value = false;
  std::string ds_str, val_str;
  int state = 0;
  for(auto it = value.begin(); it != value.end(); ++it) {
    switch(state) {
      case 0:
        // {data_stream_ID SMA_RX,sampling_rate_Msps 0.55},{data_stream_ID 
        // ^
        if(*it != '{') {
          std::ostringstream oss;
          oss << "error parsing " << inst << "worker's " << pr;
          oss << "property value of \" "<< value << "\n";
          throw oss.str();
        }
        state++;
        break;
      case 1:
        ds_str.clear();
        if(*it == ' ') {
          // {data_stream_ID SMA_RX,sampling_rate_Msps 0.55},{data_stream_ID 
          //                ^
          state++;
        }
        break;
      case 2:
        if(*it == ',') {
          // {data_stream_ID SMA_RX,sampling_rate_Msps 0.55},{data_stream_ID 
          //                       ^
          grab_value = (ds_str.compare(ds_ID) == 0);
          state++;
        }
        else {
          ds_str += *it;
        }
        break;
      case 3:
        if(*it == ' ') {
          // {data_stream_ID SMA_RX,sampling_rate_Msps 0.55},{data_stream_ID 
          //                                          ^
          state++;
        }
        break;
      case 4:
        if(*it == '}') {
          // {data_stream_ID "SMA_RX",sampling_rate_Msps 0.55},{data_stream_ID 
          //                                                 ^
          if(grab_value) {
            return strtod(val_str.c_str(), NULL);
          }                              
          else {
            val_str.clear();
          }                              
          state++;
        }
        else {
          val_str += *it;
        }
        break;
      case 5:
        if(*it == ',') {
          state = 0;
        }
        else {
          state++;
        }
        break;
      default:
        std::ostringstream oss;
        oss << "error parsing " << inst << "worker's " << pr;
        oss << "property value of \" "<< value << "\n";
        throw oss.str();
        break;
    }
  }

  std::string str("could not get sampling rate for dig_radio_ctrlr ");
  str += "data stream ID ";
  str += ds_ID;
  str += " because data stream ID could not be found";
  return 0;
}

/*! @brief Intended to be used after dig_radio_ctrlr config lock already
 *         occurred, but will still probably work even before the lock.
 ******************************************************************************/
double get_rx_baud_rate() {
  assert(mode_requires_rx());
  const std::string& ID = app_main_args.rx_data_stream_ID;
  const char* p = "sampling_rate_Msps";
  double samps_per_sec = get_dig_radio_ctrlr_prop_by_data_stream_ID(ID,p) * 1e6;

  return samps_per_sec / get_rx_samps_per_sym() / get_rx_syms_per_baud();
}

/// @brief Get rate at which bits are expected to be received.
double get_rx_bit_rate() {
  return get_rx_baud_rate() * get_rx_syms_per_baud() * get_rx_bits_per_sym();
}

double get_tx_baud_rate() {
  assert(mode_requires_tx());
  const std::string& ID = app_main_args.tx_data_stream_ID;
  const char* p = "sampling_rate_Msps";
  double samps_per_sec = get_dig_radio_ctrlr_prop_by_data_stream_ID(ID,p) * 1e6;

  return samps_per_sec / get_tx_samps_per_sym() / get_tx_syms_per_baud();
}

/// @brief Get bit rate at which bits are transmitted.
double get_tx_bit_rate() {
  return get_tx_baud_rate() * get_tx_syms_per_baud() * get_tx_bits_per_sym();
}

double get_desired_rx_samp_rate_Msps() {
  assert(mode_requires_rx());
  double tmp = app_main_args.rx_baud_rate * get_rx_syms_per_baud();
  tmp *= get_rx_samps_per_sym(); // in sps
  return tmp / 1e6; // in Msps
}

double get_desired_rx_bw_3dB_MHz() {
  double tmp = get_desired_rx_samp_rate_Msps();
  tmp *= (1. - (FS_TOLERANCE_PERCENT/100.)); // tmp is max desired bandwidth
  return tmp / (1. + (BW_TOLERANCE_PERCENT/100.));
}

/// @brief Get tolerance.
double get_tol_rx_samp_rate_Msps() {
  return get_desired_rx_samp_rate_Msps() * FS_TOLERANCE_PERCENT / 100.;
}

/// @brief Get tolerance.
double get_tol_rx_bw_3dB_MHz() {
  return get_desired_rx_bw_3dB_MHz() * BW_TOLERANCE_PERCENT / 100.;
}

double get_desired_tx_samp_rate_Msps() {
  assert(mode_requires_tx());
  double tmp = app_main_args.tx_baud_rate * get_tx_syms_per_baud();
  tmp *= get_tx_samps_per_sym(); // in sps
  return tmp / 1e6; // in Msps
}

double get_desired_tx_bw_3dB_MHz() {
  double tmp = get_desired_tx_samp_rate_Msps();
  tmp *= (1. - (FS_TOLERANCE_PERCENT/100.)); // tmp is max bandwidth
  return tmp / (1. + (BW_TOLERANCE_PERCENT/100.));
}

/// @brief Get tolerance.
double get_tol_tx_samp_rate_Msps() {
  return get_desired_tx_samp_rate_Msps() * (FS_TOLERANCE_PERCENT/100.);
}

/// @brief Get tolerance.
double get_tol_tx_bw_3dB_MHz() {
  return get_desired_tx_bw_3dB_MHz() * (BW_TOLERANCE_PERCENT/100.);
}

/*! @brief Use dig_radio_ctrlr worker to request config lock for one RX
 *         and one TX I/Q data stream.
 *  @return Either EXIT_SUCCESS or EXIT_FAILURE macro values.
 ******************************************************************************/
void dig_radio_ctrlr_request_config_lock(bool& do_exit_failure) {
  assert(p_app != 0);
  OA::Application& app = *p_app;

  // unlock all existing configs in order to allow them to change (not necessary
  // for first config lock request, but including it to exemplify best
  // practices)
  app.setPropertyValue<bool>("dig_radio_ctrlr", "unlock_all", true);

  std::ostringstream cfg_lock_req_ostr;

  // config_lock_ID value was arbitrarily chosen, this app only ever has a
  // single config lock
  cfg_lock_req_ostr << "config_lock_ID LOCK_FSK_DIG_RADIO_CTRLR_APP,";

  cfg_lock_req_ostr << "data_streams {";
  unsigned num_data_streams = 0;

  if(mode_requires_rx()) {

    std::ostringstream req; // requested data stream
    req << "{"; // beginning of data stream

    //      bitstream             digital radio         __
    //  +-----------------+     +-------------------+   \/
    //  |   "RX0" workers |     |                   |   |
    //  |     +---+ +--+  |     |                   |   |
    //  |  <--|   |<|  |<-|<----|<----data stream ID|<--+
    //  |     +---+ +--+  |     |                   |
    //  +-----------------+     +-------------------+
    req << "data_stream_type RX,";
    req << "data_stream_ID "      << app_main_args.rx_data_stream_ID << ",";
    req << "routing_ID RX0,"; // application uses RX0 routing ID (see OAS)

    req << "tuning_freq_MHz "     << app_main_args.rx_tuning_freq_MHz << ",";
    req << "bandwidth_3dB_MHz "   << get_desired_rx_bw_3dB_MHz() << ",";
    req << "sampling_rate_Msps "  << get_desired_rx_samp_rate_Msps() << ",";

    // it is desired that the samples coming from digital radio and through the
    // "RXO" workers are complex
    req << "samples_are_complex " << "true,";

    req << "gain_mode "           << app_main_args.rx_gain_mode << ",";
    req << "gain_dB "             << app_main_args.rx_gain_dB   << ",";
    req << "tolerance_tuning_freq_MHz "
                                  <<app_main_args.tol_rx_tuning_freq_MHz << ",";
    req << "tolerance_bandwidth_3dB_MHz " << get_tol_rx_bw_3dB_MHz()     << ",";
    req << "tolerance_sampling_rate_Msps "<< get_tol_rx_samp_rate_Msps() << ",";
    req << "tolerance_gain_dB "           << app_main_args.tol_rx_gain_dB;
    req << "}"; // end of data stream

    cfg_lock_req_ostr << req.str();
    num_data_streams++;
  }

  if(mode_requires_tx()) {

    std::ostringstream req; // requested data stream
    if(num_data_streams > 0) { // only append comma if rx stream was requested
      req << ",";
    }
    req << "{"; // beginning of data stream

    //      bitstream             digital radio         __
    //  +-----------------+     +-------------------+   \/
    //  |   "TX0" workers |     |                   |   |
    //  |     +---+ +--+  |     |                   |   |
    //  |  -->|   |>|  |->|---->|---->data stream ID|->-+
    //  |     +---+ +--+  |     |                   |
    //  +-----------------+     +-------------------+
    req << "data_stream_type TX,";
    req << "data_stream_ID "      << app_main_args.tx_data_stream_ID << ",";
    req << "routing_ID TX0,"; // application uses TX0 routing ID (see OAS)

    req << "tuning_freq_MHz "     << app_main_args.tx_tuning_freq_MHz << ",";
    req << "bandwidth_3dB_MHz "   << get_desired_tx_bw_3dB_MHz()      << ",";
    req << "sampling_rate_Msps "  << get_desired_tx_samp_rate_Msps()  << ",";

    // it is desired that the samples coming from the "TX0" workers and to the
    // digital radio are complex
    req << "samples_are_complex " << "true,";

    req << "gain_mode "           << app_main_args.tx_gain_mode << ",";
    req << "gain_dB "             << app_main_args.tx_gain_dB   << ",";
    req << "tolerance_tuning_freq_MHz "
                                  <<app_main_args.tol_tx_tuning_freq_MHz << ",";
    req << "tolerance_bandwidth_3dB_MHz " << get_tol_tx_bw_3dB_MHz()     << ",";
    req << "tolerance_sampling_rate_Msps "<< get_tol_tx_samp_rate_Msps() << ",";
    req << "tolerance_gain_dB "           << app_main_args.tol_tx_gain_dB;
    req << "}"; // end of data stream

    cfg_lock_req_ostr << req.str();
  }

  cfg_lock_req_ostr << "}"; // end of all data streams

  // perform request, exception will be thrown if request fails
  std::string lock_req_str = cfg_lock_req_ostr.str();
  app.setProperty("dig_radio_ctrlr","request_config_lock",lock_req_str.c_str());

  std::cout << "dig_radio_ctrlr config lock request was SUCCESSFUL\n";

  //print_RX_data_stream_config(app, inst, do_exit_failure);
  do_exit_failure = false;
}

double get_expected_filesize_bits() {
  return OS_JPEG_SIZE_BYTES * 8;
}

double get_expected_rx_filesize_bits() {
  return get_expected_filesize_bits();
}

double get_expected_tx_filesize_bits() {
  return get_expected_filesize_bits();
}

double get_rx_runtime_duration_sec(bool do_log = false) {
  if(do_log) {
    std::cout << "rx bit rate         = " << get_rx_bit_rate() << " bps\n";
  }
  double tmp = get_expected_rx_filesize_bits() / get_rx_bit_rate();
  if(do_log) {
    std::cout << "rx runtime duration = " << tmp << " sec\n";
  }
  return tmp;
}

double get_tx_runtime_duration_sec(bool do_log = false) {
  if(do_log) {
    std::cout << "tx bit rate         = " << get_tx_bit_rate() << " bps\n";
  }
  double tmp = get_expected_tx_filesize_bits() / get_tx_bit_rate();
  if(do_log) {
    std::cout << "tx runtime duration = " << tmp << " sec\n";
  }
  return tmp;
}

bool ADC_overrun_occurred() {

  std::string i_str; // instance string name of worker in application

  assert(p_app != 0);
  assert(mode_requires_rx());
  p_app->getProperty("dig_radio_ctrlr","app_inst_name_RX0_qadc", i_str);
  return p_app->getPropertyValue<bool>(i_str.c_str(), "overrun");
}

bool DAC_underrun_occurred() {

  std::string i_str; // instance string name of worker in application

  assert(p_app != 0);
  assert(mode_requires_tx());
  p_app->getProperty("dig_radio_ctrlr","app_inst_name_TX0_qdac", i_str);
  return p_app->getPropertyValue<bool>(i_str.c_str(), "underrun");
}

void do_sleep(bool& do_exit_failure) {
  assert(p_app != 0);

  if(mode_is_filerw()) {
    std::cout << "waiting indefinitely for application done condition...\n";
    p_app->wait();       // wait until app is "done"
  }
  else {
    double max_rx_or_tx_duration_sec = 0.;

    std::cout << "file size           = ";
    std::cout << get_expected_rx_filesize_bits() << " bits\n";
    if(mode_requires_rx()) {
      max_rx_or_tx_duration_sec = get_rx_runtime_duration_sec(true);
      if(mode_requires_tx()) {
        double tx_runtime_duration = get_tx_runtime_duration_sec(true);
        if(tx_runtime_duration > max_rx_or_tx_duration_sec) {
          max_rx_or_tx_duration_sec = tx_runtime_duration;
        }
      }
    }
    else if(mode_requires_tx()) {
      max_rx_or_tx_duration_sec = get_tx_runtime_duration_sec(true);
    }

    double tmp = max_rx_or_tx_duration_sec * APP_EXTRA_RUNTIME_FACTOR;
    tmp += APP_EXTRA_RUNTIME_SEC;
    unsigned int runtime_sec = (unsigned int) ceil(tmp);
    std::cout << "running app for " << runtime_sec << " sec\n";

    for(unsigned int n = runtime_sec; n > 0; n--) {

      if(mode_requires_rx()) {
        if(ADC_overrun_occurred()) {
          if(not overrun) { // only print warning once
            std::ostringstream oss;
            oss << "WARN:  ADC I/Q data was dropped due to limited ";
            oss << "FPGA->CPU bandwidth\n";
            std::cout << oss.str().c_str();
          }
          overrun = true;
        }

        if(app_main_args.rx_gain_mode.compare("auto") == 0) {
          double gain;
          const std::string& ID = app_main_args.rx_data_stream_ID;
          try {
            gain = get_dig_radio_ctrlr_prop_by_data_stream_ID(ID, "gain_dB");
            std::cout << "current " << ID << " AGC gain is " << gain << " dB\n";
          }
          catch(...) { // don't want the app to fail if we can't read this value
          }
        }
      }

      if(mode_requires_tx()) {

        // DAC is expected to underrun as soon as file_read-produced "EOF"
        // reaches the qdac worker, so we monitor qdac's underrun *only* up
        // until the expected TX runtime duration, 0.1 provides a fudge factor so
        // that we don't accidentally produce a warning close to the expected
        // undderun moment
        double max_mon_time = std::max(get_tx_runtime_duration_sec()-0.1,0.);
        if(((double)(runtime_sec-n)) < max_mon_time) {
          OCPI::OS::logPrint(OCPI_LOG_INFO, "monitoring DAC underrun");

          if(DAC_underrun_occurred()) {
            if(not underrun) { // only print warning once
              std::ostringstream ostr;
              ostr << "WARN:  breaks occurred in I/Q data transmission (DAC CDC ";
              ostr << "underrun occurred)\n";
              std::cout << ostr.str();
            }
            underrun = true;
          }
        }
        else {
          OCPI::OS::logPrint(OCPI_LOG_INFO, "NOT monitoring DAC underrun");
        }
      }

      sleep(1);
    }

    p_app->stop();
  }
  do_exit_failure = false;
}

bool output_file_expected() {
  return mode_requires_rx() or mode_is_filerw();
}

std::string get_output_filename() {
  assert(output_file_expected());
  assert(p_app != 0);
  return p_app->getPropertyValue<std::string>("file_write", "fileName");
}

void app_post_stop() {

  if(app_main_args.d) {
    bool print_parameters = true;
    bool print_cached = not app_main_args.U;
    assert(p_app != 0);
    p_app->dumpProperties(print_parameters, print_cached, NULL);
  }

  if(mode_requires_rx()) {
    if(ADC_overrun_occurred()) {
      if(not overrun) {
        std::ostringstream oss;
        oss << "WARN:  ADC I/Q data was dropped due to limited HDL->RCC ";
        oss << "bandwidth\n";
        std::cout << oss.str().c_str();
      }
      overrun = true;
    }
    assert(p_app != 0);
    if(!p_app->getPropertyValue<bool>("real_digitizer", "sync_criteria_met")) {
      std::cerr << "ERR : real_digitizer sync pattern 0xFACE never found\n";
      exit(EXIT_FAILURE);
    }
  }

  // DAC is expected to underrun as soon as file_read-produced "EOF"
  // reaches the qdac worker, so we don't check for it here

  if(output_file_expected()) {
    std::cout << "output file is located at " << get_output_filename();
  }
  std::cout << "\n";
}

/*! @todo / FIXME - implement --tolerance-*x_baud_rate parameters?
 *  @todo / FIXME - warn if multiple e.g. --rx-baud --rx-baud
 *  @return Return value indicates success (0 indicates success, 1
 *          indicates failure).
 ******************************************************************************/
int main(int argc, char **argv) {

  p_app = 0;

  // Reference OpenCPI_Application_Development document for an explanation of the ACI

  int ret = parse_args(argc, argv);
  if(ret != EXIT_SUCCESS) {
    return ret;
  }

  bool do_exit_failure = false;

  try {
    OA::Application app(app_main_args.app_xml);
    p_app = &app;

    // ensure odata directory exists, mkdir -p will still exit 0 if dir already
    // exists
    if(system("mkdir -p odata") != 0) {
      std::cerr << "ERROR: mkdir -p odata returned non-zero value\n";
      exit(EXIT_FAILURE);
    }

    app.initialize(); // all resources have been allocated

    if(app_main_args.d) {
      bool print_parameters = true;
      bool print_cached = not app_main_args.U;
      app.dumpProperties(print_parameters, print_cached, NULL);
    }

    if(output_file_expected()) {
      std::string cmd_str("rm -rf ");
      cmd_str += get_output_filename();

      std::cout << "calling " << cmd_str << "\n";

      if(system(cmd_str.c_str()) != 0) {
        std::cerr << "ERROR: " << cmd_str << " returned non-zero value\n";
        exit(EXIT_FAILURE);
      }
    }

    if(not mode_is_filerw()) {
      // digital radio is configured before the application is started, i.e.
      // before data starts flowing
      dig_radio_ctrlr_request_config_lock(do_exit_failure);
      if(do_exit_failure) {
        return EXIT_FAILURE;
      }
    }

    app.start();      // execution is started

    do_sleep(do_exit_failure);

    if(do_exit_failure) {
      return EXIT_FAILURE;
    }

    app_post_stop();
  } catch (std::string &e) {
    std::cerr << "ERROR: exception caught: " << e << std::endl;
    return EXIT_FAILURE;
  } catch (const char* e) {
    std::cerr << "ERROR: exception caught: " << e << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
