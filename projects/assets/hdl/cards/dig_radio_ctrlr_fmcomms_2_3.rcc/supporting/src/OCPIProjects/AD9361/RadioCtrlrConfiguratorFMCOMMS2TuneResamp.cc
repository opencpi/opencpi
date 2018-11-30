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

#include "RadioCtrlrConfiguratorFMCOMMS2TuneResamp.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

template<class LC>
ConfiguratorFMCOMMS2TuneResamp<LC>::ConfiguratorFMCOMMS2TuneResamp() :
    ConfiguratorFMCOMMS2<LC>() {

  config_value_t max = std::numeric_limits<config_value_t>::max();
  ConfigValueRanges abs_ranges;
  abs_ranges.add_valid_range(-max, max);

  add_stream_config_RX_tuning_freq_complex_mixer(m_data_stream_RX1            );
  add_stream_config_CIC_dec_decimation_factor(   m_data_stream_RX1, abs_ranges);

  add_stream_config_RX_tuning_freq_complex_mixer(m_data_stream_RX2            );
  add_stream_config_CIC_dec_decimation_factor(   m_data_stream_RX2, abs_ranges);

  add_stream_config_TX_tuning_freq_complex_mixer(m_data_stream_TX1            );
  add_stream_config_CIC_int_interpolation_factor(m_data_stream_TX1, abs_ranges);

  add_stream_config_TX_tuning_freq_complex_mixer(m_data_stream_TX2            );
  add_stream_config_CIC_int_interpolation_factor(m_data_stream_TX2, abs_ranges);
}

template<class LC>
ConfiguratorFMCOMMS2TuneResamp<LC>::ConfiguratorFMCOMMS2TuneResamp(
    ConfigValueRanges CIC_dec_abs_ranges,
    ConfigValueRanges CIC_int_abs_ranges) : 
    ConfiguratorFMCOMMS2<LC>() {

  add_stream_config_RX_tuning_freq_complex_mixer(m_data_stream_RX1                    );
  add_stream_config_CIC_dec_decimation_factor(   m_data_stream_RX1, CIC_dec_abs_ranges);

  add_stream_config_RX_tuning_freq_complex_mixer(m_data_stream_RX2                    );
  add_stream_config_CIC_dec_decimation_factor(   m_data_stream_RX2, CIC_dec_abs_ranges);

  add_stream_config_TX_tuning_freq_complex_mixer(m_data_stream_TX1                    );
  add_stream_config_CIC_int_interpolation_factor(m_data_stream_TX1, CIC_int_abs_ranges);

  add_stream_config_TX_tuning_freq_complex_mixer(m_data_stream_TX2                    );
  add_stream_config_CIC_int_interpolation_factor(m_data_stream_TX2, CIC_int_abs_ranges);
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
add_stream_config_RX_tuning_freq_complex_mixer(data_stream_ID_t data_stream) {

  ConfigValueRanges abs_ranges;
  {
    config_value_t min_tuning_freq_complex_mixer_MHz;
    config_value_t max_tuning_freq_complex_mixer_MHz;
    {
      typedef int16_t complex_mixer_phs_inc_t;
      double max_AD9361_RX_SAMPL_FREQ_MHz;
      {
        double max_AD9361_RX_SAMPL_FREQ_Hz;
        double& x = max_AD9361_RX_SAMPL_FREQ_Hz;
        ///@TODO support REFCLK rates other than 40 MHz
        ///@TODO support all Rx FIR decimation factors
        x = calc_max_AD9361_RX_SAMPL_FREQ_Hz(REFCLK40MHz, RX_FIR_DEC_FACTOR);

        max_AD9361_RX_SAMPL_FREQ_MHz = max_AD9361_RX_SAMPL_FREQ_Hz/1e6;
      }
      double min_complex_mixer_NCO_freq_MHz;
      {
        double& x = min_complex_mixer_NCO_freq_MHz;
        complex_mixer_phs_inc_t phs_inc_min;
        phs_inc_min = std::numeric_limits<complex_mixer_phs_inc_t>::min();
        // see Complex_mixer.pdf formula (4)
        x = max_AD9361_RX_SAMPL_FREQ_MHz*phs_inc_min/65536.;
      }
      double max_complex_mixer_NCO_freq_MHz;
      {
        double& x = max_complex_mixer_NCO_freq_MHz;
        complex_mixer_phs_inc_t phs_inc_max;
        phs_inc_max = std::numeric_limits<complex_mixer_phs_inc_t>::max();
        // see Complex_mixer.pdf formula (4)
        x = max_AD9361_RX_SAMPL_FREQ_MHz*phs_inc_max/65536.;
      }
      // e.g. if tuning freq is 10 MHz, you always mix by the *negative* of
      // 10 MHz to achieve the mix *down* behavior, for complex mixer
      // you mix by the NCO freq
      min_tuning_freq_complex_mixer_MHz = -max_complex_mixer_NCO_freq_MHz;
      max_tuning_freq_complex_mixer_MHz = -min_complex_mixer_NCO_freq_MHz;
    }
    const config_value_t& min = min_tuning_freq_complex_mixer_MHz;
    const config_value_t& max = max_tuning_freq_complex_mixer_MHz;
    abs_ranges.add_valid_range(min, max);
  }
  LockRConstrConfig cfg(abs_ranges);
  typedef std::map<config_key_t, LockRConstrConfig> stream_cfgs_t;
  stream_cfgs_t& cfgs = m_data_streams.at(data_stream).m_configs;
  cfgs.insert(std::make_pair("tuning_freq_complex_mixer_MHz", cfg));
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
add_stream_config_TX_tuning_freq_complex_mixer(data_stream_ID_t data_stream) {

  ConfigValueRanges abs_ranges;
  {
    config_value_t min_tuning_freq_complex_mixer_MHz;
    config_value_t max_tuning_freq_complex_mixer_MHz;
    {
      typedef int16_t complex_mixer_phs_inc_t;
      double max_AD9361_TX_SAMPL_FREQ_MHz;
      {
        double max_AD9361_TX_SAMPL_FREQ_Hz;
        double& x = max_AD9361_TX_SAMPL_FREQ_Hz;
        ///@TODO support REFCLK rates other than 40 MHz
        ///@TODO support all Rx FIR decimation factors
        x = calc_max_AD9361_TX_SAMPL_FREQ_Hz(REFCLK40MHz, TX_FIR_INT_FACTOR);

        max_AD9361_TX_SAMPL_FREQ_MHz = max_AD9361_TX_SAMPL_FREQ_Hz/1e6;
      }
      double min_complex_mixer_NCO_freq_MHz;
      {
        double& x = min_complex_mixer_NCO_freq_MHz;
        complex_mixer_phs_inc_t phs_inc_min;
        phs_inc_min = std::numeric_limits<complex_mixer_phs_inc_t>::min();
        // see Complex_mixer.pdf formula (4)
        x = max_AD9361_TX_SAMPL_FREQ_MHz*phs_inc_min/65536.;
      }
      double max_complex_mixer_NCO_freq_MHz;
      {
        double& x = max_complex_mixer_NCO_freq_MHz;
        complex_mixer_phs_inc_t phs_inc_max;
        phs_inc_max = std::numeric_limits<complex_mixer_phs_inc_t>::max();
        // see Complex_mixer.pdf formula (4)
        x = max_AD9361_TX_SAMPL_FREQ_MHz*phs_inc_max/65536.;
      }
      // e.g. if tuning freq is 10 MHz, you always mix by the *negative* of
      // 10 MHz to achieve the mix *down* behavior, for complex mixer
      // you mix by the NCO freq
      min_tuning_freq_complex_mixer_MHz = -max_complex_mixer_NCO_freq_MHz;
      max_tuning_freq_complex_mixer_MHz = -min_complex_mixer_NCO_freq_MHz;
    }
    const config_value_t& min = min_tuning_freq_complex_mixer_MHz;
    const config_value_t& max = max_tuning_freq_complex_mixer_MHz;
    abs_ranges.add_valid_range(min, max);
  }
  LockRConstrConfig cfg(abs_ranges);
  typedef std::map<config_key_t, LockRConstrConfig> stream_cfgs_t;
  stream_cfgs_t& cfgs = m_data_streams.at(data_stream).m_configs;
  cfgs.insert(std::make_pair("tuning_freq_complex_mixer_MHz", cfg));
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
add_stream_config_CIC_dec_decimation_factor(
    data_stream_ID_t data_stream, ConfigValueRanges CIC_dec_abs_ranges) {

  LockRConstrConfig cfg(CIC_dec_abs_ranges);

  typedef std::map<config_key_t, LockRConstrConfig> stream_cfgs_t;
  stream_cfgs_t& cfgs = m_data_streams.at(data_stream).m_configs;
  cfgs.insert(std::make_pair("CIC_dec_decimation_factor", cfg));
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
add_stream_config_CIC_int_interpolation_factor(
    data_stream_ID_t data_stream, ConfigValueRanges CIC_int_abs_ranges) {

  LockRConstrConfig cfg(CIC_int_abs_ranges);

  typedef std::map<config_key_t, LockRConstrConfig> stream_cfgs_t;
  stream_cfgs_t& cfgs = m_data_streams.at(data_stream).m_configs;
  cfgs.insert(std::make_pair("CIC_int_interpolation_factor", cfg));
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(
    data_stream_ID_t data_stream,
    config_key_t     samp_rate) {

  constrain_FE_samp_rate_to_func_of_DS_complex_mixer_freq(data_stream,samp_rate); // (1/2)
  constrain_DS_complex_mixer_freq_to_func_of_FE_samp_rate(data_stream,samp_rate); // (2/2)
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_FE_samp_rate_to_func_of_DS_complex_mixer_freq(
    data_stream_ID_t data_stream,
    config_key_t     samp_rate) {

  LockRConstrConfig& cfg_X = get_config(samp_rate);

  const LockRConstrConfig& cfg_Y = get_config(data_stream, "tuning_freq_complex_mixer_MHz");

  // newly constrained ValidRanges that will be applied to X
  ConfigValueRanges new_constrained_ranges_X;

  ConfigValueRanges possible_ranges_Y = cfg_Y.get_ranges_possible();

  auto it_Y = possible_ranges_Y.m_ranges.begin();
  for(; it_Y != possible_ranges_Y.m_ranges.end(); it_Y++) {

    typedef int16_t phs_inc_t;
    double min_samp_rate;

    // tuning freq is negative of complex mixer NCO freq,
    // e.g. if tuning freq is 10 MHz, you always mix by the *negative* of
    // 10 MHz to achieve the mix *down* behavior, for complex mixer
    // you mix by the NCO freq
    double min_complex_mixer_NCO_freq_MHz = -it_Y->get_max();
    double max_complex_mixer_NCO_freq_MHz = -it_Y->get_min();

    if(min_complex_mixer_NCO_freq_MHz >= 0.) {
      double min_NCO_freq_magnitude = min_complex_mixer_NCO_freq_MHz;
      phs_inc_t phs_inc_upper_limit = std::numeric_limits<phs_inc_t>::max();
      // nco_output_freq = sample_freq*phs_inc/(2^phs_acc_width)
      min_samp_rate = min_NCO_freq_magnitude/(phs_inc_upper_limit/65536.);
    }
    else { // (min_complex_mixer_NCO_freq_MHz < 0.)
      double min_NCO_freq_magnitude;
      if(max_complex_mixer_NCO_freq_MHz >= 0.) {
        min_NCO_freq_magnitude = 0.;
      }
      else {
        min_NCO_freq_magnitude = -max_complex_mixer_NCO_freq_MHz;
      }
      phs_inc_t phs_inc_lower_limit = std::numeric_limits<phs_inc_t>::min();
      // nco_output_freq = sample_freq*phs_inc/(2^phs_acc_width)
      min_samp_rate = min_NCO_freq_magnitude/(-phs_inc_lower_limit/65536.);
    }
    config_value_t min = min_samp_rate;
    config_value_t max = std::numeric_limits<config_value_t>::max();

    new_constrained_ranges_X.add_valid_range(min, max);
  }

  if(not cfg_X.is_locked()) {
    cfg_X.overlap_constrained(new_constrained_ranges_X);
  }
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_DS_complex_mixer_freq_to_func_of_FE_samp_rate(
    data_stream_ID_t data_stream, config_key_t samp_rate) {

  LockRConstrConfig& cfg_X = get_config(data_stream, "tuning_freq_complex_mixer_MHz");

  const LockRConstrConfig& cfg_Y = get_config(samp_rate);

  // newly constrained ValidRanges that will be applied to X
  ConfigValueRanges new_constrained_ranges_X;

  ConfigValueRanges possible_ranges_Y = cfg_Y.get_ranges_possible();

  auto it_Y = possible_ranges_Y.m_ranges.begin();
  for(; it_Y != possible_ranges_Y.m_ranges.end(); it_Y++) {

    double min_tuning_freq_complex_mixer_MHz;
    double max_tuning_freq_complex_mixer_MHz;
    {
      // assuming Y (sampling rate) is positive
      double min_complex_mixer_NCO_freq_MHz = it_Y->get_max()*-32768./65536.;
      double max_complex_mixer_NCO_freq_MHz = it_Y->get_max()*32767./65536.;
      // tuning freq is negative of complex mixer NCO freq,
      // e.g. if tuning freq is 10 MHz, you always mix by the *negative* of
      // 10 MHz to achieve the mix *down* behavior, for complex mixer
      // you mix by the NCO freq
      min_tuning_freq_complex_mixer_MHz = -max_complex_mixer_NCO_freq_MHz;
      max_tuning_freq_complex_mixer_MHz = -min_complex_mixer_NCO_freq_MHz;
    }
    const config_value_t& min = min_tuning_freq_complex_mixer_MHz;
    const config_value_t& max = max_tuning_freq_complex_mixer_MHz;

    new_constrained_ranges_X.add_valid_range(min, max);
  }

  if(not cfg_X.is_locked()) {
    cfg_X.overlap_constrained(new_constrained_ranges_X);
  }
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_tuning_freq_equals_Rx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(
    data_stream_ID_t data_stream) {

  LockRConstrConfig& cfg_X = get_config(data_stream, config_key_tuning_freq_MHz);
  LockRConstrConfig& cfg_A = get_config("Rx_RFPLL_LO_freq");
  LockRConstrConfig& cfg_B = get_config(data_stream, "tuning_freq_complex_mixer_MHz");

  constrain_all_XAB_such_that_X_equals_A_plus_B(cfg_X, cfg_A, cfg_B);
  //constrain_RFPLL_freq_by_FMCOMMS2_balun_limits_and_min_DS_samp_rate(m_data_stream_RX1, "Rx_RFPLL_LO_freq");
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_tuning_freq_equals_Tx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(
    data_stream_ID_t data_stream) {

  LockRConstrConfig& cfg_X = get_config(data_stream, config_key_tuning_freq_MHz);
  LockRConstrConfig& cfg_A = get_config("Tx_RFPLL_LO_freq");
  LockRConstrConfig& cfg_B = get_config(data_stream, "tuning_freq_complex_mixer_MHz");

  constrain_all_XAB_such_that_X_equals_A_plus_B(cfg_X, cfg_A, cfg_B);
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec(
    data_stream_ID_t data_stream,
    config_key_t     frontend_bandwidth) {

  bool cfg_X_changed, cfg_A_changed, cfg_B_changed;

  LockRConstrConfig& cfg_X = get_config(data_stream, config_key_bandwidth_3dB_MHz);
  LockRConstrConfig& cfg_A = get_config(frontend_bandwidth);
  LockRConstrConfig& cfg_B = get_config(data_stream, "CIC_dec_decimation_factor");

  constrain_all_XAB_such_that_X_equals_A_divided_by_B(cfg_X, cfg_A, cfg_B);
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int(
    data_stream_ID_t data_stream,
    config_key_t    frontend_bandwidth) {

  LockRConstrConfig& cfg_X = get_config(data_stream, config_key_bandwidth_3dB_MHz);
  LockRConstrConfig& cfg_A = get_config(frontend_bandwidth);
  LockRConstrConfig& cfg_B = get_config(data_stream, "CIC_int_interpolation_factor");

  constrain_all_XAB_such_that_X_equals_A_divided_by_B(cfg_X, cfg_A, cfg_B);
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec(
    data_stream_ID_t data_stream,
    config_key_t    frontend_samp_rate) {

  bool cfg_X_changed, cfg_A_changed, cfg_B_changed;

  LockRConstrConfig& cfg_X = get_config(data_stream, config_key_sampling_rate_Msps);
  LockRConstrConfig& cfg_A = get_config(frontend_samp_rate);
  LockRConstrConfig& cfg_B = get_config(data_stream, "CIC_dec_decimation_factor");

  constrain_all_XAB_such_that_X_equals_A_divided_by_B(cfg_X, cfg_A, cfg_B);
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::
constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int(
    data_stream_ID_t data_stream,
    config_key_t    frontend_samp_rate) {

  LockRConstrConfig& cfg_X = get_config(data_stream, config_key_sampling_rate_Msps);
  LockRConstrConfig& cfg_A = get_config(frontend_samp_rate);
  LockRConstrConfig& cfg_B = get_config(data_stream, "CIC_int_interpolation_factor");

  constrain_all_XAB_such_that_X_equals_A_divided_by_B(cfg_X, cfg_A, cfg_B);
}

template<class LC>
void ConfiguratorFMCOMMS2TuneResamp<LC>::impose_constraints_single_pass() {

  // unfortunately, order in which these are called matters...

  constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_0_samp_rate();
  constrain_Rx_RFPLL_LO_freq_le_f_data_stream_0_samp_rate();
  constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_1_samp_rate();
  constrain_Rx_RFPLL_LO_freq_le_f_data_stream_1_samp_rate();
  constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_2_samp_rate();
  constrain_Tx_RFPLL_LO_freq_le_f_data_stream_2_samp_rate();
  constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_3_samp_rate();
  constrain_Tx_RFPLL_LO_freq_le_f_data_stream_3_samp_rate();
  constrain_data_stream_0_samp_rate_le_2500_minus_2400();
  constrain_data_stream_1_samp_rate_le_2500_minus_2400();
  constrain_data_stream_2_samp_rate_le_2500_minus_2400();
  constrain_data_stream_3_samp_rate_le_2500_minus_2400();

  constrain_gain_mode_data_stream_0_equals_0_or_1();
  constrain_gain_mode_data_stream_1_equals_0_or_1();
  constrain_gain_mode_data_stream_2_equals_1();
  constrain_gain_mode_data_stream_3_equals_1();
  constrain_gain_dB_data_stream_2_is_in_range_neg_89p75_to_0();
  constrain_gain_dB_data_stream_3_is_in_range_neg_89p75_to_0();

  constrain_tuning_freq_equals_Rx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(m_data_stream_RX1); // (all/3)
  constrain_tuning_freq_equals_Rx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(m_data_stream_RX2); // (all/3)
  constrain_tuning_freq_equals_Tx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(m_data_stream_TX1); // (all/3)
  constrain_tuning_freq_equals_Tx_RFPLL_LO_freq_plus_complex_mixer_NCO_freq(m_data_stream_TX2); // (all/3)

  constrain_gain_dB_data_stream_0_equals_func_of_Rx_RFPLL_LO_freq();
  constrain_gain_dB_data_stream_1_equals_func_of_Rx_RFPLL_LO_freq();

  constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec(m_data_stream_RX1, "rx_rf_bandwidth"); // (all/3)
  constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_dec(m_data_stream_RX2, "rx_rf_bandwidth"); // (all/3)
  constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int(m_data_stream_TX1, "tx_rf_bandwidth"); // (all/3)
  constrain_DS_bandwidth_equals_FE_bandwidth_divided_by_CIC_int(m_data_stream_TX2, "tx_rf_bandwidth"); // (all/3)

  constrain_RX_SAMPL_FREQ_MHz_equals_TX_SAMPL_FREQ_MHz_times_DAC_Clk_divider();

  constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec(m_data_stream_RX1, "RX_SAMPL_FREQ_MHz"); // (all/3)
  constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   m_data_stream_RX1, "RX_SAMPL_FREQ_MHz"); // (all/2)

  constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_dec(m_data_stream_RX2, "RX_SAMPL_FREQ_MHz"); // (all/3)
  constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   m_data_stream_RX2, "RX_SAMPL_FREQ_MHz"); // (all/2)

  constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int(m_data_stream_TX1, "TX_SAMPL_FREQ_MHz"); // (all/3)
  constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   m_data_stream_TX1, "TX_SAMPL_FREQ_MHz"); // (all/2)

  constrain_sampling_rate_equals_FE_samp_rate_divided_by_CIC_int(m_data_stream_TX2, "TX_SAMPL_FREQ_MHz"); // (all/3)
  constrain_FE_samp_rate_equals_func_of_DS_complex_mixer_freq(   m_data_stream_TX2, "TX_SAMPL_FREQ_MHz"); // (all/2)
  constrain_samples_are_complex_data_stream_0_equals_1();
  constrain_samples_are_complex_data_stream_1_equals_1();
  constrain_samples_are_complex_data_stream_2_equals_1();
  constrain_samples_are_complex_data_stream_3_equals_1();

  // Nyquist criterion constraints
  constrain_rx_rf_bandwidth_less_than_or_equal_to_RX_SAMPL_FREQ_MHz();
  constrain_tx_rf_bandwidth_less_than_or_equal_to_TX_SAMPL_FREQ_MHz();
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
