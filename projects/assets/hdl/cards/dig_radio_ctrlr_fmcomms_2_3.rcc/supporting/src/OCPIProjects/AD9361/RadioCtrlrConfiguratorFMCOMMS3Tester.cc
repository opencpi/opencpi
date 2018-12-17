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
#include "RadioCtrlr.hh" // config_value_t
#include "RadioCtrlrConfiguratorTester.hh" // ConfiguratorTester
#include "RadioCtrlrConfiguratorFMCOMMS3.hh" // ConfiguratorFMCOMMS3

namespace OCPIProjects {

namespace RadioCtrlr {

ConfiguratorFMCOMMS3Tester::ConfiguratorFMCOMMS3Tester(ConfiguratorFMCOMMS3<>& UUT) :
    ConfiguratorTester(UUT) {
}

bool ConfiguratorFMCOMMS3Tester::run_tests() {
  try {
    //m_UUT.set_forwarding_callback_log_info(vprintf); // to debug issues
    //m_UUT.set_forwarding_callback_log_debug(vprintf); // to debug issues
    //m_UUT.log_all_possible_config_values();
    test_constraint_RX_tuning_freqs_equal_Rx_RFPLL_LO_freq();
    test_constraint_TX_tuning_freqs_equal_Tx_RFPLL_LO_freq();

    test_constraint_RX_bandwidths_equal_rx_rf_bandwidth();
    test_constraint_TX_bandwidths_equal_tx_rf_bandwidth();

    test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider();
    test_constrain_TX_SAMPL_FREQ_to_RX_SAMPL_FREQ_divided_by_DAC_Clk_divider();
    test_constraint_RX_sampling_rates_equal_RX_SAMPL_FREQ();
    test_constraint_TX_sampling_rates_equal_TX_SAMPL_FREQ();
    test_constrain_DAC_Clk_divider_to_RX_SAMPL_FREQ_divided_by_TX_SAMPL_FREQ();

    test_constraint_RX_gain_mode_abs_range();
    test_constraint_TX_gain_mode_abs_range();

    test_constraint_RX_gain_abs_range();
    test_constrain_RX_gain_limits();
    test_constraint_TX_gain_abs_range();

    config_value_t bw_min_MHz     = 1.25;
    config_value_t bw_middle_MHz  = 10.;
    config_value_t bw_max_MHz     = 40.;
    config_value_t fs_min_Msps    = 2.083334;
    config_value_t fs_middle_Msps = 15.;
    config_value_t fs_max_Msps    = 61.44;
    test_constrain_Nyquist_criteria(bw_min_MHz,bw_middle_MHz,bw_max_MHz,fs_min_Msps,fs_middle_Msps,fs_max_Msps);
  }
  catch(std::string& err) {
    std::cerr << err << "\n";
    return false;
  }
  catch(const char* err) {
    std::cerr << err << "\n";
    return false;
  }
  catch(...) {
    std::cerr << "caught unknown exception\n";
    return false;
  }
  return true;
}

void ConfiguratorFMCOMMS3Tester::test_constraint_RX_tuning_freqs_equal_Rx_RFPLL_LO_freq() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 70.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 6000., true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               2400., true );
    test_config_lock(*it, config_key_tuning_freq_MHz, 70.,   false);
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               2400., true );
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    m_UUT.unlock_all();
    test_config_lock("Rx_RFPLL_LO_freq",               2400., true );
    test_config_lock(*it, config_key_tuning_freq_MHz, 6000., false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    test_config_lock("Rx_RFPLL_LO_freq",               70.,   false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    test_config_lock("Rx_RFPLL_LO_freq",               2400., true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    test_config_lock("Rx_RFPLL_LO_freq",               6000., false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constraint_TX_tuning_freqs_equal_Tx_RFPLL_LO_freq() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 70.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 6000., true );
    m_UUT.unlock_all();
    test_config_lock("Tx_RFPLL_LO_freq",               2400., true );
    test_config_lock(*it, config_key_tuning_freq_MHz, 70.,   false);
    m_UUT.unlock_all();
    test_config_lock("Tx_RFPLL_LO_freq",               2400., true );
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    m_UUT.unlock_all();
    test_config_lock("Tx_RFPLL_LO_freq",               2400., true );
    test_config_lock(*it, config_key_tuning_freq_MHz, 6000., false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    test_config_lock("Tx_RFPLL_LO_freq",               70.,   false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    test_config_lock("Tx_RFPLL_LO_freq",               2400., true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_tuning_freq_MHz, 2400., true );
    test_config_lock("Tx_RFPLL_LO_freq",               6000., false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constraint_RX_bandwidths_equal_rx_rf_bandwidth() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 0.4,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 10.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 56.,   true );
    m_UUT.unlock_all();
    test_config_lock("rx_rf_bandwidth",                  10.,   true );
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 0.4,   false);
    m_UUT.unlock_all();
    test_config_lock("rx_rf_bandwidth",                  10.,   true );
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 10.,   true );
    m_UUT.unlock_all();
    test_config_lock("rx_rf_bandwidth",                  10.,   true );
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 56.,   false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 10.,   true );
    test_config_lock("rx_rf_bandwidth",                  0.4,   false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 10.,   true );
    test_config_lock("rx_rf_bandwidth",                  10.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 10.,   true );
    test_config_lock("rx_rf_bandwidth",                  56.,   false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constraint_TX_bandwidths_equal_tx_rf_bandwidth() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 1.249,  false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 1.25,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 40.,    true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz, 40.001, false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constraint_RX_sampling_rates_equal_RX_SAMPL_FREQ() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 2.084, true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 61.44, true );
    m_UUT.unlock_all();
    test_config_lock("RX_SAMPL_FREQ_MHz",                 10.,   true );
    test_config_lock(*it, config_key_sampling_rate_Msps, 2.084, false);
    m_UUT.unlock_all();
    test_config_lock("RX_SAMPL_FREQ_MHz",                 10.,   true );
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    m_UUT.unlock_all();
    test_config_lock("RX_SAMPL_FREQ_MHz",                 10.,   true );
    test_config_lock(*it, config_key_sampling_rate_Msps, 61.44, false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    test_config_lock("RX_SAMPL_FREQ_MHz",                 2.084, false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    test_config_lock("RX_SAMPL_FREQ_MHz",                 10.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    test_config_lock("RX_SAMPL_FREQ_MHz",                 61.44, false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constraint_TX_sampling_rates_equal_TX_SAMPL_FREQ() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 2.084, true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 61.44, true );
    m_UUT.unlock_all();
    test_config_lock("TX_SAMPL_FREQ_MHz",                 10.,   true );
    test_config_lock(*it, config_key_sampling_rate_Msps, 2.084, false);
    m_UUT.unlock_all();
    test_config_lock("TX_SAMPL_FREQ_MHz",                 10.,   true );
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    m_UUT.unlock_all();
    test_config_lock("TX_SAMPL_FREQ_MHz",                 10.,   true );
    test_config_lock(*it, config_key_sampling_rate_Msps, 61.44, false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    test_config_lock("TX_SAMPL_FREQ_MHz",                 2.084, false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    test_config_lock("TX_SAMPL_FREQ_MHz",                 10.,   true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, 10.,   true );
    test_config_lock("TX_SAMPL_FREQ_MHz",                 61.44, false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider() {
  m_UUT.unlock_all(); // start of test
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

void ConfiguratorFMCOMMS3Tester::test_constrain_TX_SAMPL_FREQ_to_RX_SAMPL_FREQ_divided_by_DAC_Clk_divider() {
  test_constrain_RX_SAMPL_FREQ_to_TX_SAMPL_FREQ_multiplied_by_DAC_Clk_divider();
}

void ConfiguratorFMCOMMS3Tester::test_constrain_DAC_Clk_divider_to_RX_SAMPL_FREQ_divided_by_TX_SAMPL_FREQ() {
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

void ConfiguratorFMCOMMS3Tester::test_constraint_RX_gain_abs_range() {
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

void ConfiguratorFMCOMMS3Tester::test_constrain_RX_gain_limits() {
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

void ConfiguratorFMCOMMS3Tester::test_constraint_RX_gain_mode_abs_range() {
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

void ConfiguratorFMCOMMS3Tester::test_constraint_TX_gain_abs_range() {
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

void ConfiguratorFMCOMMS3Tester::test_constraint_TX_gain_mode_abs_range() {
  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");
  const config_value_t _auto = 0;
  const config_value_t manual= 1;
  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, -1.,                       false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, manual, true );
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, _auto,  false);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_gain_mode, 2.,                        false);
  }
}

void ConfiguratorFMCOMMS3Tester::test_constrain_Nyquist_criteria(
      const config_value_t bw_min_MHz,
      const config_value_t bw_middle_MHz,
      const config_value_t bw_max_MHz,
      const config_value_t fs_min_Msps,
      const config_value_t fs_middle_Msps,
    config_value_t fs_max_Msps) {

  std::vector<data_stream_ID_t> data_stream_keys;
  data_stream_keys.push_back("SMA_RX1A");
  data_stream_keys.push_back("SMA_RX2A");
  data_stream_keys.push_back("SMA_TX1A");
  data_stream_keys.push_back("SMA_TX2A");

  for(auto it=data_stream_keys.begin(); it!=data_stream_keys.end(); it++) {

    // test parameters passed in to ensure test is setup properly
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz,     true);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz,  true);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz,     true);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps,    true);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps, true);
    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps,    true);

    // test Nyquist criteria constraint

    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   , true );
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    , false);
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz , false);
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz    , true );

    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps, true );
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    , false);
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz , true );

    m_UUT.unlock_all();
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps   , true );
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    , true );

    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_max_MHz    , true );
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   , false);
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps, false);
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_max_Msps   , true );

    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_middle_MHz , true );
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   , false);
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_middle_Msps, true );

    m_UUT.unlock_all();
    test_config_lock(*it, config_key_bandwidth_3dB_MHz,  bw_min_MHz    , true );
    test_config_lock(*it, config_key_sampling_rate_Msps, fs_min_Msps   , true );
  }
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
