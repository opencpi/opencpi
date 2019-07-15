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

/*! @file
 *  @brief This contents of this file are intended to include any and all
 *         radio-specific functionality, and as little as possible
 *         radio-agnostic functionality.
 ******************************************************************************/

#include <iostream>
#include <unistd.h> // sleep()
#include <vector> // std::vector
#include "OcpiApi.hh"
#include "FSKApp.hh"

class MatchstiqZ1FSKApp : public FSKRFApp {

public:

  /*! @param[in] file OAS filename.
   *  @param[in] args Arguments to executable.
   ****************************************************************************/
  MatchstiqZ1FSKApp(const char* file, const args_t& args) :
      FSKRFApp(file, args), m_min_diff_rx_tx_center_freq_MHz(1.) {
  }

  tx_ocs_config_t get_default_tx_config() {

    tx_ocs_config_t ret;

    // intelligently set the prompt-suggested default TX settings to values that
    // are expected to work well with current RX settings
    ret.sample_rate_MHz = 39.936;
    if(get_mode_requires_rx() && m_val_has_been_set["rx_sample_rate_MHz"]) {
      double tmp = m_app.getPropertyValue<double>("rx", "sample_rate_MHz");
      ret.sample_rate_MHz = tmp;
    }
    ret.rf_center_freq_MHz = 1000.;
    if(get_mode_requires_rx() && m_val_has_been_set["rx_frequency_MHz"]) {
      double rx_rf_fc = m_app.getPropertyValue<double>("rx", "frequency_MHz");
      double diff = ret.rf_center_freq_MHz - rx_rf_fc; 

      if(!(abs(diff) > m_min_diff_rx_tx_center_freq_MHz)) {
        ret.rf_center_freq_MHz = rx_rf_fc + m_min_diff_rx_tx_center_freq_MHz;
      }
    }

    ret.rf_bw_MHz = -1.;
    ret.bb_bw_MHz = 5.;
    ret.bb_gain_dB = -4.;
    ret.rf_gain_dB = 4.;

    return ret;
  }

  rx_ocs_config_t get_default_rx_config() {

    rx_ocs_config_t ret;

    // intelligently set the prompt-suggested default RX settings to values that
    // are expected to work well with current TX settings
    ret.sample_rate_MHz = 39.936;
    if(m_val_has_been_set["tx_sample_rate_MHz"]) {
      double tmp = m_app.getPropertyValue<double>("tx", "sample_rate_MHz");
      ret.sample_rate_MHz = tmp;
    }
    ret.rf_center_freq_MHz = 1000.;
    if(m_val_has_been_set["tx_frequency_MHz"]) {
      double tx_rf_fc = m_app.getPropertyValue<double>("tx", "frequency_MHz");
      double diff = ret.rf_center_freq_MHz - tx_rf_fc; 
      if(!(abs(diff) > m_min_diff_rx_tx_center_freq_MHz)) {
        ret.rf_center_freq_MHz = tx_rf_fc - m_min_diff_rx_tx_center_freq_MHz;
      }
    }

    ret.rf_bw_MHz = 400.;
    ret.bb_bw_MHz = 5.;
    ret.bb_gain_dB = 42.;
    ret.rf_gain_dB = 10.;
  
    return ret;
  }

  void prompt_and_configure_rx() {

    if(get_mode_requires_rx()) {

      rx_ocs_config_t default_rx_config = get_default_rx_config();

      double rx_if_fc = m_min_diff_rx_tx_center_freq_MHz;

      FSKApp::prompt_and_configure_rx(default_rx_config, rx_if_fc);
    }
  }

protected:

  const double m_min_diff_rx_tx_center_freq_MHz;

}; // class MatchstiqZ1FSKApp

class FMCOMMS3FSKApp : public FSKRFApp {

public:

  /*! @param[in] file OAS filename.
   *  @param[in] args Arguments to executable.
   ****************************************************************************/
  FMCOMMS3FSKApp(const char* file, const args_t& args) : FSKRFApp(file, args) {
  }

  void initialize() {

    FSKRFApp::initialize();
    prompt_and_select_rx_antenna();
    prompt_and_select_tx_antenna();
  }

  std::vector<std::string> get_supported_rx_chans() {

    return {"RX1A", "RX2A"};
  }

  std::vector<std::string> get_supported_tx_chans() {

    return {"TX1A", "TX2A"};
  }

  /*! @param[in] description   Description of antenna/channel for prompt.
   *  @param[in] supported_chs Vector of supported Channel name strings.
   *  @param[in] inst          Application instance name of the
   *                           fmcomms_2_3_rx.rcc or fmcomms_2_3_tx.rcc worker
   *                           whose config property is to be set.
   ****************************************************************************/
  void prompt_and_select_ch(const char* description,
      const std::vector<std::string>& supported_chs, const char* inst) {

    std::cout << "Enter " << get_frontend_c_str() << " " << description;
    std::cout << " channel (" << supported_chs << ")\n";
    std::string input;
    std::getline(std::cin, input);
    std::string msg(get_frontend_c_str());
    msg += " channel";
    throw_if_val_unsupported(msg.c_str(), input, supported_chs);

    std::string val_start_str("reference_clk_rate_Hz 40e6,duplex_mode FDD,");
    val_start_str += "are_using_REF_CLK_SMA false,SMA_channel ";
    m_app.setProperty(inst, "config", (val_start_str + input).c_str());
  }

  void prompt_and_select_tx_antenna() {

    if(get_mode_requires_tx()) {
      prompt_and_select_ch("TX SMA", get_supported_tx_chans(), "tx");
    }
  }

  void prompt_and_select_rx_antenna() {

    if(get_mode_requires_rx()) {
      prompt_and_select_ch("RX SMA", get_supported_rx_chans(), "rx");
    }
  }

  tx_ocs_config_t get_default_tx_config() {

    tx_ocs_config_t ret;

    // intelligently set the prompt-suggested default TX settings to values that
    // are expected to work well with current RX settings
    ret.sample_rate_MHz = 4.;
    if(get_mode_requires_rx() && m_val_has_been_set["rx_sample_rate_MHz"]) {
      double tmp = m_app.getPropertyValue<double>("rx", "sample_rate_MHz");
      ret.sample_rate_MHz = tmp;
    }
    ret.rf_center_freq_MHz = 2400.;
    if(get_mode_requires_rx() && m_val_has_been_set["rx_frequency_MHz"]) {
      double tmp = m_app.getPropertyValue<double>("rx", "frequency_MHz");
      ret.rf_center_freq_MHz = tmp;
    }

    ret.rf_bw_MHz = -1.;
    ret.bb_bw_MHz = 4.;
    ret.bb_gain_dB = -1.;
    ret.rf_gain_dB = -34.;

    return ret;
  }

  rx_ocs_config_t get_default_rx_config() {

    rx_ocs_config_t ret;

    // intelligently set the prompt-suggested default RX settings to values that
    // are expected to work well with current TX settings
    ret.sample_rate_MHz = 4.;
    if(get_mode_requires_tx() && m_val_has_been_set["tx_sample_rate_MHz"]) {
      double tmp = m_app.getPropertyValue<double>("tx", "sample_rate_MHz");
      ret.sample_rate_MHz = tmp;
    }
    ret.rf_center_freq_MHz = 2400.;
    if(get_mode_requires_tx() && m_val_has_been_set["tx_frequency_MHz"]) {
      double tmp = m_app.getPropertyValue<double>("tx", "frequency_MHz");
      ret.rf_center_freq_MHz = tmp;
    }

    ret.rf_bw_MHz = -1.;
    ret.bb_bw_MHz = 4.;
    ret.bb_gain_dB = -1.;
    ret.rf_gain_dB = 24.;

    return ret;
  }

protected:

  virtual const char* get_frontend_c_str(bool upper = false) {

    return upper ? "FMCOMMS3" : "fmcomms3";
  }

}; // class FMCOMMS3FSKApp

class FMCOMMS2FSKApp : public FMCOMMS3FSKApp {

public:

  FMCOMMS2FSKApp(const char* file, const args_t& args) :
      FMCOMMS3FSKApp(file, args) {
  }

protected:

  virtual const char* get_frontend_c_str(bool upper = false) {

    return upper ? "FMCOMMS2" : "fmcomms2";
  }

}; // class FMCOMMS2FSKApp

/*! @return Vector of HDL platforms which this application is intended to
 *          support.
 ******************************************************************************/
std::vector<std::string> get_supported_hdl_pfs() {
  return {"matchstiq_z1", "zed", "zed_ise", "ml605"};
}

std::vector<std::string> get_zed_supported_frontends() {

  return {"fmcomms2", "fmcomms3"};
}

std::vector<std::string> get_ml605_supported_frontends() {

  return {"fmcomms2", "fmcomms3"};
}

/*! @brief  Retrieve RF frontend, prompting if necessary.
 *  @return string representing RF frontend
 ******************************************************************************/
std::string get_frontend(const std::string& hdl_pf) {

  std::string ret;

  bool do_prompt;

  std::vector<std::string> supported_fes;
  if(hdl_pf == "zed") {
    supported_fes = get_zed_supported_frontends();
    do_prompt = true;
  }
  else if(hdl_pf == "ml605") {
    supported_fes = get_ml605_supported_frontends();
    do_prompt = true;
  }
  else if(hdl_pf == "matchstiq_z1") {
    ret.assign(hdl_pf);
    do_prompt = false;
  }

  if(do_prompt) {
    std::cout << "Enter RF frontend for " << hdl_pf; 
    std::cout << " (supported values are ";
    bool first = true;
    for(auto it = supported_fes.begin(); it != supported_fes.end(); ++it) {
      std::cout << (first ? "" : ", ") << "\'" << *it << "\'";
      first = false;
    }
    std::cout << ")\n";
    std::getline(std::cin, ret);

    throw_if_val_unsupported("RF frontend", ret, supported_fes);
  }

  return ret;
}

void run(FSKApp& app) {
  // Reference https://opencpi.github.io/OpenCPI_Application_Development.pdf for
  // an explanation of the ACI.

  bool adc_overrun_flag = false;

  app.initialize(); // all resources have been allocated
  std::cout << "App initialized.\n";

  app.print_properties(true); // dump initial props

  app.start();      // execution is started
  std::cout << "App started.\n";

  // Must use either wait()/finish() or stop(). The finish() method must
  // always be called after wait(). The start() method can be called
  // again after stop().
  // The filerw mode waits for a ZLM for app to finish.
  std::cout << "Waiting for done signal from file_write.\n";
  app.wait();       // wait until app is "done"
  app.finish();     // do end-of-run processing like dump properties
  std::cout << "App stopped/finished.\n";

  app.print_properties(false); // dump final props
  app.print_bytes_to_file(adc_overrun_flag);
  app.print_tx_peak_values();
  app.print_rx_peak_values();
  app.move_out_file();

  std::cout << "Application complete\n";
}

void run_rf(FSKRFApp& app) {
  // Reference https://opencpi.github.io/OpenCPI_Application_Development.pdf for
  // an explanation of the ACI.

  bool adc_overrun_flag = false;

  app.initialize(); // all resources have been allocated
  std::cout << "App initialized.\n";

  app.prompt_and_configure_tx();
  app.prompt_and_configure_rx();
  app.print_properties(true); // dump initial props

  app.start();      // execution is started
  std::cout << "App started.\n";

  // Must use either wait()/finish() or stop(). The finish() method must
  // always be called after wait(). The start() method can be called
  // again after stop().
  // All none-filerw modes use runtime to determine when to stop.
  unsigned timer = app.get_runtime_sec();
  std::cout << "App runs for " << timer << " seconds...\n";
  while(timer-- > 0) {
    sleep(1);
    adc_overrun_flag = app.get_adc_overrun_flag();
  }
  app.stop();
  std::cout << "App stopped/finished.\n";

  app.print_properties(false); // dump final props
  app.print_bytes_to_file(adc_overrun_flag);
  app.print_tx_peak_values();
  app.print_rx_peak_values();
  app.move_out_file();

  std::cout << "Application complete\n";
}

int main(int argc, char **argv) {

  int ret = 0;

  try {
    args_t args(parse_and_validate_args(argc, argv));

    if(args.mode == "filerw") {
      FSKApp app(get_and_print_oas_filename(args, "").c_str(), args);
      run(app);
    }
    else {
      std::string pf = get_and_validate_hdl_platform(get_supported_hdl_pfs());

      // because some HDL platforms supported by this app will support multiple
      // RF frontends, prompt to get intended frontend
      std::string fe(get_frontend(pf));
      if(pf == "matchstiq_z1") {
        MatchstiqZ1FSKApp app(get_and_print_oas_filename(args, fe).c_str(), args);
        run_rf(app);
      }
      else if(fe == "fmcomms2") {
        FMCOMMS2FSKApp app(get_and_print_oas_filename(args, fe).c_str(), args);
        run_rf(app);
      }
      else if(fe == "fmcomms3") {
        FMCOMMS3FSKApp app(get_and_print_oas_filename(args, fe).c_str(), args);
        run_rf(app);
      }
    }
  } catch (std::string &e) {
    std::cerr << "ERROR: " << e << "\n";
    ret = 1;
  }

  return ret;
}
