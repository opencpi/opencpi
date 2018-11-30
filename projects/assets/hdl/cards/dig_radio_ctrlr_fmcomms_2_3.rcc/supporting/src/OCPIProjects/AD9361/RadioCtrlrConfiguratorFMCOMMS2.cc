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

#include <algorithm> // std::min
#include "RadioCtrlrConfiguratorFMCOMMS2.hh"
#include "RadioCtrlrConfiguratorAD9361.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

/*! @brief data stream 0 -maps-to-> FMCOMMS2 AD9361 RX1A SMA port,
 *         data stream 1 -maps-to-> FMCOMMS2 AD9361 RX2A SMA port,
 *         data stream 2 -maps-to-> FMCOMMS2 AD9361 TX2A SMA port,
 *         data stream 3 -maps-to-> FMCOMMS2 AD9361 TX2A SMA port.
 *         A configurator is a software-only representation of hardware
 *         capabilities.
 ******************************************************************************/
template<class LC>
ConfiguratorFMCOMMS2<LC>::ConfiguratorFMCOMMS2() :
    ConfiguratorAD9361<LC>("SMA_RX1A","SMA_RX2A","SMA_TX1A","SMA_TX2A") {
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_by_FMCOMMS2_balun_limits_and_min_DS_samp_rate(
    data_stream_ID_t data_stream_ID, config_key_t key) {

  LockRConstrConfig& cfg_Y = get_config(key);

  // constraint will be applied to config Y, think Y=f(X)
  if(cfg_Y.is_locked()) {
    return;
  }

  // constraint is a function of config X, think Y=f(X)
  auto config_key = config_key_sampling_rate_Msps;
  const LockRConstrConfig& cfg_X = get_config(data_stream_ID, config_key);

  // these values apply to all 4 baluns (i.e. all 4 data streams)
  const double FMCOMMS2_balun_low_3dB_cutoff_freq = 2400.;
  const double FMCOMMS2_balun_upp_3dB_cutoff_freq = 2500.;

  // note this is complex samp rate (because *it_X is always complex samp
  // rate and it is assigned to *it_X below)
  config_value_t min_samp_rate_Msps;
  min_samp_rate_Msps = std::numeric_limits<config_value_t>::max();

  bool at_least_one_samp_rate = false;

  ConfigValueRanges possible_ranges_X = cfg_X.get_ranges_possible();

  auto it_X = possible_ranges_X.m_ranges.begin();
  for(; it_X != possible_ranges_X.m_ranges.end(); it_X++) {

    at_least_one_samp_rate = true;
    min_samp_rate_Msps = std::min(min_samp_rate_Msps, it_X->get_min());
  }

  if(not at_least_one_samp_rate) {
    // should never happen, but writing safe code
    min_samp_rate_Msps = 0;
  }

  bool limited_by_more_than_balun = false;

  ConfigValueRanges new_constrained_ranges_Y;
  {
    config_value_t min, max;
    // remember *_samp_rate_Msps are the complex samp rates
    min = FMCOMMS2_balun_low_3dB_cutoff_freq + (min_samp_rate_Msps/2.);
    max = FMCOMMS2_balun_upp_3dB_cutoff_freq - (min_samp_rate_Msps/2.);
    if(min_samp_rate_Msps > 0.) {
      limited_by_more_than_balun = true;
    }

    new_constrained_ranges_Y.add_valid_range(min, max);
  }

  cfg_Y.overlap_constrained(new_constrained_ranges_Y);
}

template<class LC>
bool ConfiguratorFMCOMMS2<LC>::
constrain_data_stream_samp_rate_by_f_config(
    data_stream_ID_t data_stream_ID, config_key_t config_key,
    bool t_upp_f_low) {

  const LockRConstrConfig& cfg_X = get_config(config_key);
  auto fskey = config_key_sampling_rate_Msps;
  auto cfg = get_config(data_stream_ID, fskey);
  bool possible_ranges_changed;
  config_value_t min;
  bool limited_by_more_than_balun;
  if(t_upp_f_low) { // do upper
    min = 2. * (2500. - cfg_X.get_largest_max_possible());
    limited_by_more_than_balun = cfg_X.get_largest_max_possible() > 0.;
  }
  else { // do lower
    min = 2. * (cfg_X.get_smallest_min_possible() - 2400.);
    limited_by_more_than_balun = cfg_X.get_smallest_min_possible() > 2400.;
  }
  cfg.impose_min_for_all_ranges_constrained(min, &possible_ranges_changed);
  /*if(possible_ranges_changed) {
    std::ostringstream ostr;
    ostr << "configurator: valid ranges updated for data stream ";
    ostr << data_stream_ID << ", config " << fskey << " to ";
    ostr << cfg.get_ranges_possible();
    ostr << " due to FMCOMMS2 balun limitations";
    if(limited_by_more_than_balun) {
      ostr << " as well as changes in valid ranges of " << config_key;
    }

    this->log_debug("%s", ostr.str().c_str());
  }*/

  return possible_ranges_changed;
}

template<class LC>
bool ConfiguratorFMCOMMS2<LC>::limit_Y_to_ge_X_times_A_plus_B(
    LockRConstrConfig& cfg_Y, const LockRConstrConfig& cfg_X,
    const config_value_t A, const config_value_t B) const {

  if(cfg_Y.is_locked()) {
    return false;
  }

  ConfigValueRanges possible_ranges_Y = cfg_Y.get_ranges_possible();
  ConfigValueRanges possible_ranges_X = cfg_X.get_ranges_possible();

  // newly constrained ConfigValidRanges that will be applied to Y
  ConfigValueRanges new_constrained_ranges_Y;

  config_value_t min;
  if(A > 0) {
    min = possible_ranges_X.get_smallest_min() * A + B;
  }
  else {
    min = possible_ranges_X.get_largest_max() * A + B;
  }
  config_value_t max = possible_ranges_Y.get_largest_max();

  if(max >= min) {
    new_constrained_ranges_Y.add_valid_range(min, max);
  }

  return cfg_Y.overlap_constrained(new_constrained_ranges_Y);
}

template<class LC>
bool ConfiguratorFMCOMMS2<LC>::limit_Y_to_le_X_times_A_plus_B(
    LockRConstrConfig& cfg_Y, const LockRConstrConfig& cfg_X,
    const config_value_t A, const config_value_t B) const {

  if(cfg_Y.is_locked()) {
    return false;
  }

  ConfigValueRanges possible_ranges_Y = cfg_Y.get_ranges_possible();
  ConfigValueRanges possible_ranges_X = cfg_X.get_ranges_possible();

  // newly constrained ConfigValidRanges that will be applied to Y
  ConfigValueRanges new_constrained_ranges_Y;

  config_value_t min = possible_ranges_Y.get_smallest_min();
  config_value_t max;
  if(A > 0) {
    max = possible_ranges_X.get_largest_max() * A + B;
  }
  else {
    max = possible_ranges_X.get_smallest_min() * A + B;
  }

  if(max >= min) {
    new_constrained_ranges_Y.add_valid_range(min, max);
  }

  return cfg_Y.overlap_constrained(new_constrained_ranges_Y);
}

template<class LC>
bool ConfiguratorFMCOMMS2<LC>::constrain_Y_ge_X_times_A_plus_B(
    LockRConstrConfig& cfg_Y, LockRConstrConfig& cfg_X,
    const config_value_t A, const config_value_t B) const {

  bool changed = false;

  // if Y >= X*A + B, ...
  changed |= limit_Y_to_ge_X_times_A_plus_B(cfg_Y, cfg_X, A, B);

  if(A > 0) { // ... then X <= Y/A - B/A
    changed |= limit_Y_to_le_X_times_A_plus_B(cfg_X, cfg_Y, 1/A, -B/A);
  }
  else {
    // whenver you multiply or divide an inequality by a negative number, you
    // must flip the inequality sign
    changed |= limit_Y_to_ge_X_times_A_plus_B(cfg_X, cfg_Y, 1/A, -B/A);
  }

  return changed;
}

template<class LC>
bool ConfiguratorFMCOMMS2<LC>::constrain_Y_le_X_times_A_plus_B(
    LockRConstrConfig& cfg_Y, LockRConstrConfig& cfg_X,
    const config_value_t A, const config_value_t B) const {

  bool changed = false;

  // if Y <= X*A + B, ...
  changed |= limit_Y_to_le_X_times_A_plus_B(cfg_Y, cfg_X, A, B);

  if(A > 0) { // ... then X >= Y/A - B/A
    changed |= limit_Y_to_ge_X_times_A_plus_B(cfg_X, cfg_Y, 1/A, -B/A);
  }
  else {
    // whenver you multiply or divide an inequality by a negative number, you
    // must flip the inequality sign
    changed |= limit_Y_to_le_X_times_A_plus_B(cfg_X, cfg_Y, 1/A, -B/A);
  }

  return changed;
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_config_f_data_stream_samp_rate(config_key_t config_key,
    const std::string& ds, bool t_greaterequal_f_lessequal) {

  LockRConstrConfig& cfg_Y = get_config(config_key);

  LockRConstrConfig& cfg_X = get_config(ds, config_key_sampling_rate_Msps);

  if(t_greaterequal_f_lessequal) {
    constrain_Y_ge_X_times_A_plus_B(cfg_Y, cfg_X, 0.5, 2400);
  }
  else {
    constrain_Y_le_X_times_A_plus_B(cfg_Y, cfg_X, -0.5, 2500);
  }
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_data_stream_samp_rate_le_2500_minus_2400(
    const std::string& ds) {

  const double FMCOMMS2_balun_low_3dB_cutoff_freq = 2400.;
  const double FMCOMMS2_balun_upp_3dB_cutoff_freq = 2500.;

  double max_samp_rate_Msps = FMCOMMS2_balun_upp_3dB_cutoff_freq;
  max_samp_rate_Msps -= FMCOMMS2_balun_low_3dB_cutoff_freq;

  auto& cfg = get_config(ds, config_key_sampling_rate_Msps);
  if(not cfg.is_locked()) {
    cfg.impose_max_for_all_ranges_constrained(max_samp_rate_Msps);
  }
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_0_samp_rate() {

  auto& ds = m_data_stream_RX1;
  constrain_config_f_data_stream_samp_rate("Rx_RFPLL_LO_freq", ds, true);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Rx_RFPLL_LO_freq_le_f_data_stream_0_samp_rate() {

  auto& ds = m_data_stream_RX1;
  constrain_config_f_data_stream_samp_rate("Rx_RFPLL_LO_freq", ds, false);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Rx_RFPLL_LO_freq_ge_f_data_stream_1_samp_rate() {

  auto& ds = m_data_stream_RX2;
  constrain_config_f_data_stream_samp_rate("Rx_RFPLL_LO_freq", ds, true);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Rx_RFPLL_LO_freq_le_f_data_stream_1_samp_rate() {

  auto& ds = m_data_stream_RX2;
  constrain_config_f_data_stream_samp_rate("Rx_RFPLL_LO_freq", ds, false);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_2_samp_rate() {

  auto& ds = m_data_stream_TX1;
  constrain_config_f_data_stream_samp_rate("Tx_RFPLL_LO_freq", ds, true);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Tx_RFPLL_LO_freq_le_f_data_stream_2_samp_rate() {

  auto& ds = m_data_stream_TX1;
  constrain_config_f_data_stream_samp_rate("Tx_RFPLL_LO_freq", ds, false);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Tx_RFPLL_LO_freq_ge_f_data_stream_3_samp_rate() {

  auto& ds = m_data_stream_TX2;
  constrain_config_f_data_stream_samp_rate("Tx_RFPLL_LO_freq", ds, true);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_Tx_RFPLL_LO_freq_le_f_data_stream_3_samp_rate() {

  auto& ds = m_data_stream_TX2;
  constrain_config_f_data_stream_samp_rate("Tx_RFPLL_LO_freq", ds, false);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_data_stream_0_samp_rate_le_2500_minus_2400() {

  constrain_data_stream_samp_rate_le_2500_minus_2400(m_data_stream_RX1);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_data_stream_1_samp_rate_le_2500_minus_2400() {

  constrain_data_stream_samp_rate_le_2500_minus_2400(m_data_stream_RX2);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_data_stream_2_samp_rate_le_2500_minus_2400() {

  constrain_data_stream_samp_rate_le_2500_minus_2400(m_data_stream_TX1);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::
constrain_data_stream_3_samp_rate_le_2500_minus_2400() {

  constrain_data_stream_samp_rate_le_2500_minus_2400(m_data_stream_TX2);

  this->throw_if_any_possible_ranges_are_empty(__func__);
}

template<class LC>
void ConfiguratorFMCOMMS2<LC>::impose_constraints_single_pass() {

  // FMCOMMS2 has the following constraints...
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

  // ... in addition to the AD9361 constraints
  ConfiguratorAD9361<LC>::impose_constraints_single_pass();
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
