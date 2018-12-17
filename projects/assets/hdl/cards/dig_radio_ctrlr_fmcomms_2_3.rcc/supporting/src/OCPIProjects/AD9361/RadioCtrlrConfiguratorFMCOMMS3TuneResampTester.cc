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

#include <iostream> // std::cout, std::cerr
#include <vector>   // std::vector
#include <string>   // std::string
#include "RadioCtrlr.hh"                                     // config_value_t
#include "RadioCtrlrConfiguratorTester.hh"                   // ConfiguratorTester
#include "RadioCtrlrConfiguratorFMCOMMS3TuneResampTester.hh" // ConfiguratorFMCOMMS3TuneResampTester

namespace OCPIProjects {

namespace RadioCtrlr {

ConfiguratorFMCOMMS3TuneResampTester::ConfiguratorFMCOMMS3TuneResampTester(
    ConfiguratorFMCOMMS3TuneResamp<>& UUT) : ConfiguratorTester(UUT) {
}

bool ConfiguratorFMCOMMS3TuneResampTester::run_tests() {
  try{
    //m_UUT.set_forwarding_callback_log_info(vprintf); // to debug issues
    //m_UUT.set_forwarding_callback_log_debug(vprintf); // to debug issues
    //m_UUT.log_all_possible_config_values();
    test_constrain_RX_gain_mode_abs_range();
    test_constrain_TX_gain_mode_abs_range();
    test_constrain_RX_gain_abs_range();
    test_constrain_TX_gain_abs_range();

    test_constrain_tuning_freq_equals_RFPLL_LO_freq_plus_complex_mixer_NCO_freq("SMA_RX1A","Rx_RFPLL_LO_freq"); // (all/3)
    test_constrain_tuning_freq_equals_RFPLL_LO_freq_plus_complex_mixer_NCO_freq("SMA_RX2A","Rx_RFPLL_LO_freq"); // (all/3)
    test_constrain_tuning_freq_equals_RFPLL_LO_freq_plus_complex_mixer_NCO_freq("SMA_TX1A","Tx_RFPLL_LO_freq"); // (all/3)
    test_constrain_tuning_freq_equals_RFPLL_LO_freq_plus_complex_mixer_NCO_freq("SMA_TX2A","Tx_RFPLL_LO_freq"); // (all/3)

    test_constrain_RX_gain_limits();

    test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec("SMA_RX1A", "rx_rf_bandwidth"); // (all/3)
    test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec("SMA_RX2A", "rx_rf_bandwidth"); // (all/3)
    test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int("SMA_TX1A", "tx_rf_bandwidth"); // (all/3)
    test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int("SMA_TX2A", "tx_rf_bandwidth"); // (all/3)

    test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider(); // (1/3)
    test_constrain_TX_SAMPL_FREQ_to_RX_SAMPL_FREQ_divided_by_DAC_Clk_divider();    // (2/3)
    test_constrain_DAC_Clk_divider_to_RX_SAMPL_FREQ_divided_by_TX_SAMPL_FREQ();    // (3/3)


    config_value_t expected_min_NCO_freq = -30.72; // 61.44*-32768./65536.;
    config_value_t expected_max_NCO_freq = 61.44*32767./65536.;
    config_value_t& min = expected_min_NCO_freq;
    config_value_t& max = expected_max_NCO_freq;

    test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec("SMA_RX1A", "RX_SAMPL_FREQ_MHz"); // (all/3)
    test_constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   "SMA_RX1A", "RX_SAMPL_FREQ_MHz",min,max,2.08333334,61.44); // (all/2)

    test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec("SMA_RX2A", "RX_SAMPL_FREQ_MHz"); // (all/3)
    test_constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   "SMA_RX2A", "RX_SAMPL_FREQ_MHz",min,max,2.08333334,61.44); // (all/2)

    test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int("SMA_TX1A", "TX_SAMPL_FREQ_MHz"); // (all/3)
    test_constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   "SMA_TX1A", "TX_SAMPL_FREQ_MHz",min,max,2.08333334,61.44); // (all/2)

    test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int("SMA_TX2A", "TX_SAMPL_FREQ_MHz"); // (all/3)
    test_constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   "SMA_TX2A", "TX_SAMPL_FREQ_MHz",min,max,2.08333334,61.44); // (all/2)

    config_value_t bw_min_MHz     = 1.25;
    config_value_t bw_middle_MHz  = 10.;
    config_value_t bw_max_MHz     = 40.;
    config_value_t fs_min_Msps    = 2.083334;
    config_value_t fs_middle_Msps = 15.;
    config_value_t fs_max_Msps    = 61.44;
    test_constrain_Nyquist_criteria(bw_min_MHz,bw_middle_MHz,bw_max_MHz,fs_min_Msps,fs_middle_Msps,fs_max_Msps);
  }
  catch(const char* err) {
    std::cerr << "caught exception: " << err << "\n";
    return false;
  }
  catch(std::string& err) {
    std::cerr << "caught exception: " << err << "\n";
    return false;
  }

  return true;
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_RX_gain_mode_abs_range() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  const config_value_t _auto = 0;
  const config_value_t manual= 1;
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, -1.,                            false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, manual, true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, _auto,  true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, 2.,                             false);
  }
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_TX_gain_mode_abs_range() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  const config_value_t _auto = 0;
  const config_value_t manual= 1;
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, -1.,                            false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, manual, true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, _auto,  false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, 2.,                             false);
  }
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_RX_gain_abs_range() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, -11.,  false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, -10.,  true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, 77.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, 78.,   false);
  }
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_TX_gain_abs_range() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, -90.,  false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, -89.75,true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, 0.,    true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB, 1.,    false);
  }
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_tuning_freq_equals_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(data_stream_ID_t data_stream_ID, config_key_t RFPLL_LO_freq) {
  config_value_t expected_min_NCO_freq = -30.72; // 61.44*-32768./65536.;
  config_value_t expected_max_NCO_freq = 61.44*32767./65536.;
  config_value_t& min = expected_min_NCO_freq;
  config_value_t& max = expected_max_NCO_freq;
  test_constrain_data_stream_tuning_freq_by_RFPLL_LO_freq_plus_data_stream_complex_mixer_NCO_freq(data_stream_ID,RFPLL_LO_freq,min,max);   // (1/3)
  test_constrain_RFPLL_LO_freq_by_data_stream_tuning_freq_minus_data_stream_complex_mixer_NCO_freq(data_stream_ID,RFPLL_LO_freq,min,max);  // (2/3)
  test_constrain_data_stream_complex_mixer_NCO_freq_by_RFPLL_LO_freq_minus_data_stream_tuning_freq(data_stream_ID,RFPLL_LO_freq,min,max); // (3/3)
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_data_stream_tuning_freq_by_RFPLL_LO_freq_plus_data_stream_complex_mixer_NCO_freq(
      data_stream_ID_t data_stream_ID,
      config_key_t RFPLL_LO_freq,
      config_value_t expected_min_NCO_freq,
    config_value_t expected_max_NCO_freq) {
  config_value_t min = -expected_max_NCO_freq;
  config_value_t max = -expected_min_NCO_freq;
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          70.,   true );
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          2400., true );
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          6000., true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min,   true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max,   true ,0.0000001);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, true , 0.0000001);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     true , 0.0000001);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, true , 0.0000001);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          70.,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   true );
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          70.,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       true );
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          70.,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          70.,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          70.,   true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                      2400.,     true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                      2400.,     true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                      2400.,     true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     true );
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                      2400.,     true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                      2400.,     true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          6000., true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.+min,   false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          6000., true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 70.,       false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          6000., true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 2400.,     false);
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          6000., true , 0.0000001);
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.,     true );
  m_UUT.unlock_all();
  test_config_lock(RFPLL_LO_freq,                          6000., true , 0.0000001);
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz, 6000.+max, true , 0.0000002);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_RFPLL_LO_freq_by_data_stream_tuning_freq_minus_data_stream_complex_mixer_NCO_freq(
      data_stream_ID_t data_stream_ID,
      config_key_t RFPLL_LO_freq,
      config_value_t expected_min_NCO_freq,
    config_value_t expected_max_NCO_freq) {
  config_value_t min = -expected_max_NCO_freq;
  config_value_t max = -expected_min_NCO_freq;
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 70.      , true );
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 70.-min  , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 2400.    , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 2400.-min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 6000.+min, true , 0.0000001);
  test_config_lock(RFPLL_LO_freq                                  , 70.      , false);
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , true , 0.0000002);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 70.      , true );
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 70.-0.   , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 2400.    , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 2400.-0. , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 6000.    , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , false);
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.-0. , true , 0.0000001);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 70.+max  , true );
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 70.      , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 2400.    , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 2400.-max, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , true );
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 6000.    , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , false);
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , false);
  test_config_lock(RFPLL_LO_freq                                  , 6000.-max, true , 0.0000001);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_data_stream_complex_mixer_NCO_freq_by_RFPLL_LO_freq_minus_data_stream_tuning_freq(
      data_stream_ID_t data_stream_ID,
      config_key_t RFPLL_LO_freq,
      config_value_t expected_min_NCO_freq,
    config_value_t expected_max_NCO_freq) {
  config_value_t min = -expected_max_NCO_freq;
  config_value_t max = -expected_min_NCO_freq;
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 70.  +min, true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 2400.+min, true );
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , false);
  config_value_t tolerance_MHz = 0.0000001;
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , true, tolerance_MHz);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 6000.+min, true );
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , true , 0.0000001);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , true , 0.0000001);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 70.  +0. , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 2400.+0. , true );
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 6000.+0. , true );
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 70.+max  , true );
  test_config_lock(RFPLL_LO_freq                                  , 70.      , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 2400.+max, true );
  test_config_lock(RFPLL_LO_freq                                  , 2400.    , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , true ,0.0000001);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_tuning_freq_MHz    , 6000.+max, true ,0.0000001);
  test_config_lock(RFPLL_LO_freq                                  , 6000.    , true );
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", min      , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.       , false);
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", max      , true ,0.0000001);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_RX_gain_limits() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB,         -10.,  true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB,         -3.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB,         1.,    true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB,         62.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB,         71.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_dB,         77.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               1300., true );
    test_config_lock(*it, config_key_gain_dB,         -10.,  false);
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               1300., true );
    test_config_lock(*it, config_key_gain_dB,         -3.,   false);
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               1300., true );
    test_config_lock(*it, config_key_gain_dB,         1.,    true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               1300., true );
    test_config_lock(*it, config_key_gain_dB,         62.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               1300., true );
    test_config_lock(*it, config_key_gain_dB,         71.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               1300., true );
    test_config_lock(*it, config_key_gain_dB,         77.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4000., true );
    test_config_lock(*it, config_key_gain_dB,         -10.,  false);
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4000., true );
    test_config_lock(*it, config_key_gain_dB,         -3.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4000., true );
    test_config_lock(*it, config_key_gain_dB,         1.,    true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4000., true );
    test_config_lock(*it, config_key_gain_dB,         62.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4000., true );
    test_config_lock(*it, config_key_gain_dB,         71.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4000., true );
    test_config_lock(*it, config_key_gain_dB,         77.,   false);
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4001., true );
    test_config_lock(*it, config_key_gain_dB,         -10.,  true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4001., true );
    test_config_lock(*it, config_key_gain_dB,         -3.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4001., true );
    test_config_lock(*it, config_key_gain_dB,         1.,    true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4001., true );
    test_config_lock(*it, config_key_gain_dB,         62.,   true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4001., true );
    test_config_lock(*it, config_key_gain_dB,         71.,   false);
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               4001., true );
    test_config_lock(*it, config_key_gain_dB,         77.,   false);
    m_UUT.unlock_all();
  }
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec(
      data_stream_ID_t data_stream_ID,
    config_key_t frontend_bandwidth) {
  config_value_t min = 4; // assumed min CIC_dec decimation factor
  //config_value_t max = 8192; // assumed max CIC_dec decimation factor
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor", min, true);
  //m_UUT.unlock_all();
  //test_config_lock(data_stream_ID, "CIC_dec_decimation_factor", max, true);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 0.4/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 10./min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 56./min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",   min,     true);
  test_config_lock(frontend_bandwidth,                            10.,     true );
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 0.4/min, false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",   min,     true);
  test_config_lock(frontend_bandwidth,                            10.,     true );
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 10./min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",   min,     true);
  test_config_lock(frontend_bandwidth,                            10.,     true );
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 56./min, false);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int(
      data_stream_ID_t data_stream_ID,
    config_key_t frontend_bandwidth) {
  config_value_t min = 4; // assumed min CIC_int interpolation factor
  //config_value_t max = 8192; // assumed max CIC_int interpolation factor
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min, true);
  //m_UUT.unlock_all();
  //test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", max, true);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 1.25/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 10./min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz, 40./min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,     true);
  test_config_lock(frontend_bandwidth,                             10.,     true );
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz,  1.25/min, false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,     true);
  test_config_lock(frontend_bandwidth,                             10.,     true );
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz,  10./min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,     true);
  test_config_lock(frontend_bandwidth,                             10.,     true );
  test_config_lock(data_stream_ID, config_key_bandwidth_3dB_MHz,  40./min, false);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider() {
  m_UUT.unlock_all(); // start of test
  //m_UUT.set_log_ostream(std::cout);
  //m_UUT.log_all_possible_config_values();
  //m_UUT.disable_log();
  test_config_lock("TX_SAMPL_FREQ_MHz",       10.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       5.000,  false );
  test_config_lock("RX_SAMPL_FREQ_MHz",       10.000, true  );
  m_UUT.unlock_all();
  test_config_lock("TX_SAMPL_FREQ_MHz",       10.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       5.000,  false );
  test_config_lock("RX_SAMPL_FREQ_MHz",       20.000, true  );
  m_UUT.unlock_all();
  test_config_lock("TX_SAMPL_FREQ_MHz",       40.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       40.000, true  );
  m_UUT.unlock_all(); // start of test w/ divider=1
  test_config_lock("DAC_Clk_divider",         1.,     true  );
  test_config_lock("TX_SAMPL_FREQ_MHz",       10.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       5.000,  false );
  test_config_lock("RX_SAMPL_FREQ_MHz",       10.000, true  );
  m_UUT.unlock_all();
  test_config_lock("DAC_Clk_divider",         1.,     true  );
  test_config_lock("TX_SAMPL_FREQ_MHz",       10.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       5.000,  false );
  test_config_lock("RX_SAMPL_FREQ_MHz",       20.000, false );
  m_UUT.unlock_all();
  test_config_lock("DAC_Clk_divider",         1.,     true  );
  test_config_lock("TX_SAMPL_FREQ_MHz",       40.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       40.000, true  );
  m_UUT.unlock_all(); // start of test w/ divider=2
  test_config_lock("DAC_Clk_divider",         2.,     true  );
  test_config_lock("TX_SAMPL_FREQ_MHz",       10.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       5.000,  false );
  test_config_lock("RX_SAMPL_FREQ_MHz",       10.000, false );
  m_UUT.unlock_all();
  test_config_lock("DAC_Clk_divider",         2.,     true  );
  test_config_lock("TX_SAMPL_FREQ_MHz",       10.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       5.000,  false );
  test_config_lock("RX_SAMPL_FREQ_MHz",       20.000, true  );
  m_UUT.unlock_all();
  test_config_lock("DAC_Clk_divider",         2.,     true  );
  test_config_lock("TX_SAMPL_FREQ_MHz",       30.000, true  );
  test_config_lock("RX_SAMPL_FREQ_MHz",       30.000, false );
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_TX_SAMPL_FREQ_to_RX_SAMPL_FREQ_divided_by_DAC_Clk_divider() {
  test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider();
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_DAC_Clk_divider_to_RX_SAMPL_FREQ_divided_by_TX_SAMPL_FREQ() {
  m_UUT.unlock_all();
  test_config_lock("DAC_Clk_divider", 1., true );;
  m_UUT.unlock_all();
  test_config_lock("DAC_Clk_divider", 2., true );;

  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    config_value_t min;
    min = m_UUT.get_config_min_valid_value(*it, config_key_sampling_rate_Msps);
    config_value_t max;
    max = m_UUT.get_config_max_valid_value(*it, config_key_sampling_rate_Msps);
    test_config_lock(*it, config_key_sampling_rate_Msps, min,   true );
    test_config_lock("DAC_Clk_divider",                   1.,    false);;
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, min,   true );
    test_config_lock("DAC_Clk_divider",                   2.,    true );;
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, max,   true );
    test_config_lock("DAC_Clk_divider",                   1.,    true );;
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, max,   true );
    test_config_lock("DAC_Clk_divider",                   2.,    false);;
  }
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec(
      data_stream_ID_t data_stream_ID,
    config_key_t frontend_samp_rate) {
  config_value_t min = 4; // assumed min CIC_dec decimation factor
  config_value_t max = 8192; // assumed max CIC_dec decimation factor
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",    min,       true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",    max,       true );
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             2.084,     true );
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             61.44,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",    min,       true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",    max,       true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, false);
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             61.44,     true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             2.084,     true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",    min,       true );
  test_config_lock(frontend_samp_rate,                             61.44,     true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_dec_decimation_factor",    min,       true );
  test_config_lock(frontend_samp_rate,                             61.,       true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, false);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int(
      data_stream_ID_t data_stream_ID,
    config_key_t frontend_samp_rate) {
  config_value_t min = 4; // assumed min CIC_int interpolation factor
  config_value_t max = 8192; // assumed max CIC_int interpolation factor
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,       true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", max,       true );
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             2.084,     true );
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             61.44,     true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,       true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", max,       true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, false);
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             61.44,     true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(frontend_samp_rate,                             2.084,     true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, false);
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,       true );
  test_config_lock(frontend_samp_rate,                             61.44,     true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "CIC_int_interpolation_factor", min,       true );
  test_config_lock(frontend_samp_rate,                             61.,       true );
  test_config_lock(data_stream_ID, config_key_sampling_rate_Msps, 61.44/min, false);
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_FE_samp_rate_to_func_of_DS_complex_mixer_freq(
      data_stream_ID_t data_stream_ID,
      config_key_t frontend_samp_rate,
      config_value_t expected_min_NCO_freq,
      config_value_t expected_max_NCO_freq,
      config_value_t expected_min_sampling_rate_Msps,
    config_value_t expected_max_sampling_rate_Msps) {

  const config_value_t minif = -expected_max_NCO_freq;
  const config_value_t maxif = -expected_min_NCO_freq;
  const config_value_t& minfs = expected_min_sampling_rate_Msps;
  const config_value_t& maxfs = expected_max_sampling_rate_Msps;

  m_UUT.unlock_all();
  test_config_lock(                frontend_samp_rate,              minfs, true );
  m_UUT.unlock_all();
  test_config_lock(                frontend_samp_rate,              maxfs, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", minif, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", maxif, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", minif, true );
  test_config_lock(                frontend_samp_rate,              minfs, false);
  test_config_lock(                frontend_samp_rate,              maxfs, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", maxif, true );
  test_config_lock(                frontend_samp_rate,              minfs, false);
  test_config_lock(                frontend_samp_rate,              maxfs, true );
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_DS_complex_mixer_freq_to_func_of_FE_samp_rate(
      data_stream_ID_t data_stream_ID,
      config_key_t   frontend_samp_rate,
      config_value_t expected_min_NCO_freq,
      config_value_t expected_max_NCO_freq,
      config_value_t expected_min_sampling_rate_Msps,
    config_value_t expected_max_sampling_rate_Msps) {

  const config_value_t minif = -expected_max_NCO_freq;
  const config_value_t maxif = -expected_min_NCO_freq;
  const config_value_t& minfs = expected_min_sampling_rate_Msps;
  const config_value_t& maxfs = expected_max_sampling_rate_Msps;

  m_UUT.unlock_all();
  test_config_lock(                frontend_samp_rate,              minfs, true );
  m_UUT.unlock_all();
  test_config_lock(                frontend_samp_rate,              maxfs, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", minif, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", 0.,    true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", maxif, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", minif, true );
  test_config_lock(                frontend_samp_rate,              minfs, false);
  test_config_lock(                frontend_samp_rate,              maxfs, true );
  m_UUT.unlock_all();
  test_config_lock(data_stream_ID, "tuning_freq_complex_mixer_MHz", maxif, true );
  test_config_lock(                frontend_samp_rate,              minfs, false);
  test_config_lock(                frontend_samp_rate,              maxfs, true );
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(
    data_stream_ID_t data_stream_ID,
    config_key_t     frontend_samp_rate,
    config_value_t   expected_min_NCO_freq,
    config_value_t   expected_max_NCO_freq,
    config_value_t   expected_min_sampling_rate_Msps,
    config_value_t   expected_max_sampling_rate_Msps) {

  test_constrain_FE_samp_rate_to_func_of_DS_complex_mixer_freq(data_stream_ID,frontend_samp_rate,expected_min_NCO_freq,expected_max_NCO_freq,expected_min_sampling_rate_Msps,expected_max_sampling_rate_Msps); // (1/2)
  test_constrain_DS_complex_mixer_freq_to_func_of_FE_samp_rate(data_stream_ID,frontend_samp_rate,expected_min_NCO_freq,expected_max_NCO_freq,expected_min_sampling_rate_Msps,expected_max_sampling_rate_Msps); // (2/2)
}

void ConfiguratorFMCOMMS3TuneResampTester::test_constrain_Nyquist_criteria(
    const config_value_t bw_min_MHz,
    const config_value_t bw_middle_MHz,
    const config_value_t bw_max_MHz,
    const config_value_t fs_min_Msps,
    const config_value_t fs_middle_Msps,
    const config_value_t fs_max_Msps) {

  // test parameters passed in to ensure test is setup properly
  m_UUT.unlock_all();
  test_config_lock("rx_rf_bandwidth"  , bw_min_MHz,     true);
  m_UUT.unlock_all();
  test_config_lock("rx_rf_bandwidth"  , bw_middle_MHz,  true);
  m_UUT.unlock_all();
  test_config_lock("rx_rf_bandwidth"  , bw_max_MHz,     true);
  m_UUT.unlock_all();
  test_config_lock("RX_SAMPL_FREQ_MHz", fs_min_Msps,    true);
  m_UUT.unlock_all();
  test_config_lock("RX_SAMPL_FREQ_MHz", fs_middle_Msps, true);
  m_UUT.unlock_all();
  test_config_lock("RX_SAMPL_FREQ_MHz", fs_max_Msps,    true);

  // test Nyquist criteria constraint

  m_UUT.unlock_all();
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps   , true );
  test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz    , false);
  test_config_lock("rx_rf_bandwidth"                  , bw_middle_MHz , false);
  test_config_lock("rx_rf_bandwidth"                  , bw_min_MHz    , true );

  m_UUT.unlock_all();
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_middle_Msps, true );
  test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz    , false);
  test_config_lock("rx_rf_bandwidth"                  , bw_middle_MHz , true );

  m_UUT.unlock_all();
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_max_Msps   , true );
  test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz    , true );

  m_UUT.unlock_all();
  test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz    , true );
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps   , false);
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_middle_Msps, false);
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_max_Msps   , true );

  m_UUT.unlock_all();
  test_config_lock("rx_rf_bandwidth"                  , bw_middle_MHz , true );
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps   , false);
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_middle_Msps, true );

  m_UUT.unlock_all();
  test_config_lock("rx_rf_bandwidth"                  , bw_min_MHz    , true );
  test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps   , true );

  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");

  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {

    for(config_value_t dec=1.; dec <= 4.01; dec += 3.) {
      // test parameters passed in to ensure test is setup properly
      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",   dec,                true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz/dec,     true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz/dec,  true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz/dec,     true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps/dec,    true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/dec, true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps/dec,    true);

      // test Nyquist criteria constraint

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",    dec               , true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /dec, true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /dec, false);
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz /dec, false);
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz    /dec, true );
      test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz        , false);
      test_config_lock("rx_rf_bandwidth"                  , bw_middle_MHz     , false);
      test_config_lock("rx_rf_bandwidth"                  , bw_min_MHz        , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",    dec               , true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/dec, true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /dec, false);
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz /dec, true );
      test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz        , false);
      test_config_lock("rx_rf_bandwidth"                  , bw_middle_MHz     , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",    dec               , true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps   /dec, true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /dec, true );
      test_config_lock("rx_rf_bandwidth"                  , bw_max_MHz        , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",    dec               , true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /dec, true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /dec, false);
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/dec, false);
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps   /dec, true );
      test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps       , false);
      test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_middle_Msps,     false);
      test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_max_Msps       , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",    dec               , true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz /dec, true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /dec, false);
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/dec, true );
      test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps       , false);
      test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_middle_Msps,     true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_dec_decimation_factor",    dec               , true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz    /dec, true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /dec, true );
      test_config_lock("RX_SAMPL_FREQ_MHz"                , fs_min_Msps       , true );
    }
  }

  data_stream_keys.clear();
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");

  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {

    for(config_value_t fac=1.; fac <= 4.01; fac += 3.) {
      // test parameters passed in to ensure test is setup properly
      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac,                true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,   bw_min_MHz/fac,     true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,   bw_middle_MHz/fac,  true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,   bw_max_MHz/fac,     true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_sampling_rate_Msps,  fs_min_Msps/fac,    true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_sampling_rate_Msps,  fs_middle_Msps/fac, true);
      m_UUT.unlock_all();
      test_config_lock(*it, config_key_sampling_rate_Msps,  fs_max_Msps/fac,    true);

      // test Nyquist criteria constraint

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac               , true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /fac, true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /fac, false);
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz /fac, false);
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz    /fac, true );
      test_config_lock("tx_rf_bandwidth"                  , bw_max_MHz        , false);
      test_config_lock("tx_rf_bandwidth"                  , bw_middle_MHz     , false);
      test_config_lock("tx_rf_bandwidth"                  , bw_min_MHz        , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac               , true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/fac, true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /fac, false);
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz /fac, true );
      test_config_lock("tx_rf_bandwidth"                  , bw_max_MHz        , false);
      test_config_lock("tx_rf_bandwidth"                  , bw_middle_MHz     , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac               , true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps   /fac, true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /fac, true );
      test_config_lock("tx_rf_bandwidth"                  , bw_max_MHz        , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac               , true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    /fac, true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /fac, false);
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/fac, false);
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps   /fac, true );
      test_config_lock("TX_SAMPL_FREQ_MHz"                , fs_min_Msps       , false);
      test_config_lock("TX_SAMPL_FREQ_MHz"                , fs_middle_Msps,     false);
      test_config_lock("TX_SAMPL_FREQ_MHz"                , fs_max_Msps       , true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac               , true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz /fac, true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /fac, false);
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps/fac, true );
      test_config_lock("TX_SAMPL_FREQ_MHz"                , fs_min_Msps       , false);
      test_config_lock("TX_SAMPL_FREQ_MHz"                , fs_middle_Msps,     true );

      m_UUT.unlock_all();
      test_config_lock(*it, "CIC_int_interpolation_factor", fac               , true );
      test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz    /fac, true );
      test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   /fac, true );
      test_config_lock("TX_SAMPL_FREQ_MHz"                , fs_min_Msps       , true );
    }
  }
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
