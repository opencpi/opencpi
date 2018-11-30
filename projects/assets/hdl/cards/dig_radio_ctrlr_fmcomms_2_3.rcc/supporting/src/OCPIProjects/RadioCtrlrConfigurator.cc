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

#include <ostream>               // std::ostream
#include <sstream>               // std::ostringstream
#include <string>                // std::string
#include <map>                   // std::map
#include <vector>                // std::vector
#include <limits>                // std::numeric_limits
#include "LogForwarder.hh"       // LogForwarder
#include "UtilValidRanges.hh"    // Util::ValidRanges
#include "RadioCtrlrConfigurator.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

RadioConfiguratorDataStreamBase::RadioConfiguratorDataStreamBase(
    const data_stream_type_t type) : m_type(type) {
}

data_stream_type_t RadioConfiguratorDataStreamBase::get_type() const {
  return m_type;
}

bool RadioConfiguratorDataStreamBase::operator==(
    const RadioConfiguratorDataStreamBase& rhs) const {

  // check part of rhs that contains *this has equal content
  for(auto it = m_configs.begin(); it != m_configs.end(); it++) {
    const config_key_t&      config_key   = it->first;
    const LockRConstrConfig& config_value = it->second;

    auto itrhs = rhs.m_configs.find(config_key);
    if(itrhs == rhs.m_configs.end()) {
      return false;
    }
    if(not(config_value == (itrhs->second))) {
      return false;
    }
  }

  // check part of *this that contains rhs has equal content
  auto itrhs = rhs.m_configs.begin();
  for(; itrhs != rhs.m_configs.end(); itrhs++) {
    const config_key_t&      rhs_config_key   = itrhs->first;
    const LockRConstrConfig& rhs_config_value = itrhs->second;

    auto it = m_configs.find(rhs_config_key);
    if(it == m_configs.end()) {
      return false;
    }
    if(not(rhs_config_value == (it->second))) {
      return false;
    }
  }

  return true;
}

AnaRadioConfiguratorDataStream::AnaRadioConfiguratorDataStream(
    const data_stream_type_t type) : RadioConfiguratorDataStreamBase(type) {

  config_value_t max = std::numeric_limits<config_value_t>::max();
  {
    // set absolute ranges to all allowable values for
    // config_value_t
    ConfigValueRanges abs_ranges;
    abs_ranges.add_valid_range(-max, max);
    LockRConstrConfig config(abs_ranges);

    m_configs.insert(std::make_pair(config_key_tuning_freq_MHz, config));
  }
  {
    ConfigValueRanges abs_ranges;
    abs_ranges.add_valid_range(0., max); // bandwidth can never be negative
    LockRConstrConfig config(abs_ranges);

    m_configs.insert(std::make_pair(config_key_bandwidth_3dB_MHz, config));
  }
}

DigRadioConfiguratorDataStream::DigRadioConfiguratorDataStream(
    const data_stream_type_t type) :
    AnaRadioConfiguratorDataStream(type) {

  config_value_t max = std::numeric_limits<config_value_t>::max();
  {
    ConfigValueRanges abs_ranges;
    abs_ranges.add_valid_range(0., max); // sampling rate can never be negative
    LockRConstrConfig config(abs_ranges);

    m_configs.insert(std::make_pair(config_key_sampling_rate_Msps, config));
  }

  {
    ConfigValueRanges abs_ranges;
    abs_ranges.add_valid_range(0, 0); // map false to (config_value_t) 0
    abs_ranges.add_valid_range(1, 1); // map true to (config_value_t) 1
    LockRConstrConfig config(abs_ranges);

    m_configs.insert(std::make_pair(config_key_samples_are_complex, config));
  }
}

DigRadioConfiguratorDataStreamWithGain::DigRadioConfiguratorDataStreamWithGain(
    const data_stream_type_t type) : DigRadioConfiguratorDataStream(type) {
  {
    ConfigValueRanges abs_ranges;
    config_value_t a = 0; // auto
    abs_ranges.add_valid_range(a, a);
    config_value_t m = 1; // manual
    abs_ranges.add_valid_range(m, m);

    LockRConstrConfig config(abs_ranges);
    m_configs.insert(std::make_pair(config_key_gain_mode, config));
  }
  {
    // set absolute ranges to all allowable values for
    // config_value_t
    config_value_t max = std::numeric_limits<config_value_t>::max();
    ConfigValueRanges abs_ranges;
    abs_ranges.add_valid_range(-max, max);
    LockRConstrConfig config(abs_ranges);
    m_configs.insert(std::make_pair(config_key_gain_dB, config));
  }
}

template<class DS, class LC>
Configurator<DS, LC>::Configurator(
    const DS               data_stream_0,
    const data_stream_ID_t data_stream_0_key) :
    m_impose_constraints_first_run_did_occur(false) {

  // radio contains at a minimum a single data stream
  m_data_streams.insert(std::make_pair(data_stream_0_key, data_stream_0));

  // we really want to call impose_constraints() here, but since
  // impose_constraints() is virtual, we instead call
  // ensure_impose_constraints_first_run_did_occur() first thing
  // in every method whose behavior could have been affected by
  // the impose_constraints() call we wanted to be in the
  // constructor
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
bool Configurator<DS, LC>::lock_config(
    const data_stream_ID_t data_stream_key,
    const config_key_t     config_key,
    const config_value_t   config_val,
    const config_value_t   config_val_tolerance) {
  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  LockRConstrConfig& cfg = get_config(data_stream_key, config_key);

  const char* ds = data_stream_key.c_str();
  const char* cfgk = config_key.c_str();
  log_debug("configurator: calling lock_config() for %s %s", ds, cfgk);
  bool lock_was_successful = lock_config(cfg,config_val,config_val_tolerance);
  if(lock_was_successful) {
    std::ostringstream oss;
    oss << "configurator: lock succeeded";
    oss << " for config " << config_key;
    oss << " for data stream " << data_stream_key;
    oss << " for requested value of ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val << " with tolerance of +/- ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val_tolerance;
    oss << " (actual locked value was ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << cfg.get_smallest_min_possible(); // strong guarantee
    oss << ")";
    log_debug(oss.str().c_str());
  }
  else {
    std::ostringstream oss;
    oss << "configurator: lock failed";
    oss << " for config " << config_key;
    oss << " for data stream " << data_stream_key;
    oss << " for attempted lock value of ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val << " with tolerance of +/- ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val_tolerance;
    oss << " (possible ranges are ";
    oss << get_config(data_stream_key, config_key).get_ranges_possible() << ")";
    log_debug(oss.str().c_str());
    //log_all_possible_config_values();
  }

  return lock_was_successful;
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
bool Configurator<DS, LC>::lock_config(
    const config_key_t   config_key,
    const config_value_t config_val,
    const config_value_t config_val_tolerance) {
  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  LockRConstrConfig& cfg = get_config(config_key);

  log_debug("configurator: calling lock_config() for %s", config_key.c_str());
  bool lock_was_successful = lock_config(cfg,config_val,config_val_tolerance);
  if(lock_was_successful) {
    std::ostringstream oss;
    oss << "configurator: lock succeeded";
    oss << " for config " << config_key;
    oss << " for requested value of ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val << " with tolerance of +/- ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val_tolerance;
    oss << " (actual locked value was ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << cfg.get_smallest_min_possible(); // strong guarantee
    oss << ")";
    log_info(oss.str().c_str());
  }
  else {
    std::ostringstream oss;
    oss << "configurator: lock failed";
    oss << " for config " << config_key;
    oss << " for attempted lock value of ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val << " with tolerance of +/- ";
    oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    oss << config_val_tolerance;
    //oss << " because attempted lock value of ";
    //oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    //oss << config_val << " with tolerance of +/- ";
    //oss << std::setprecision(std::numeric_limits<config_value_t>::digits10+1);
    //oss << config_val_tolerance;
    //oss << " was out of valid range(s): " << cfg.get_ranges_possible();
    log_info(oss.str().c_str());
  }

  return lock_was_successful;
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
void Configurator<DS, LC>::unlock_config(
    const data_stream_ID_t data_stream_key,
    const config_key_t     config_key) {

  LockRConstrConfig& config = get_config(data_stream_key, config_key);

  config.unlock();
  log_debug("configurator: unlocked config (for data stream %s): %s", data_stream_key.c_str(), config_key.c_str());

  impose_constraints();

  // impose_constraints should call this function many times, but it's
  // called again here for robustness / just in case
  throw_if_any_possible_ranges_are_empty();
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
void Configurator<DS, LC>::unlock_config(
    const config_key_t config_key) {

  LockRConstrConfig& config = get_config(config_key);

  config.unlock();
  log_debug("configurator: unlocked config: %s", config_key);

  impose_constraints();

  // impose_constraints should call this function many times, but it's
  // called again here for robustness / just in case
  throw_if_any_possible_ranges_are_empty();
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
void Configurator<DS, LC>::unlock_all() {

  for(auto it=m_configs.begin(); it!= m_configs.end(); it++) {
    LockRConstrConfig& config = it->second;

    config.unlock();

    log_debug("configurator: unlocked config: %s", it->first.c_str());
  }

  auto itds = m_data_streams.begin();
  for(; itds!= m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      LockRConstrConfig& config = itc->second;

      config.unlock();

      if(not(m_callback_log_debug == 0)) {
        auto cfg = itc->first.c_str();
        auto stream = itds->first.c_str();
        log_debug("configurator: unlocked config (for data stream %s): %s", stream, cfg);
      }
    }
  }
  impose_constraints();

  // impose_constraints should call this function many times, but it's
  // called again here for robustness / just in case
  throw_if_any_possible_ranges_are_empty();
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
config_value_t Configurator<DS, LC>::get_config_min_valid_value(
    const data_stream_ID_t data_stream_key,
    const config_key_t     config_key) {

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return get_config_min_valid_value(get_config(data_stream_key, config_key));
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
config_value_t Configurator<DS, LC>::get_config_max_valid_value(
    const data_stream_ID_t data_stream_key,
    const config_key_t     config_key) {

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return get_config_max_valid_value(get_config(data_stream_key, config_key));
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
bool Configurator<DS, LC>::get_config_is_locked(
    const data_stream_ID_t data_stream_key,
    const config_key_t     config_key) {

  return get_config(data_stream_key, config_key).is_locked();
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
bool Configurator<DS, LC>::get_config_is_locked(
    const config_key_t config_key) {

  return get_config(config_key).is_locked();
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
config_value_t Configurator<DS, LC>::get_config_min_valid_value(
    const config_key_t config_key) {

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return get_config_min_valid_value(get_config(config_key));
}

/// @todo / FIXME - Implement strong guarantee of exception safety.
template<class DS, class LC>
config_value_t Configurator<DS, LC>::get_config_max_valid_value(
    const config_key_t config_key) {

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return get_config_max_valid_value(get_config(config_key));
}

template<class DS, class LC>
void Configurator<DS, LC>::log_all_possible_config_values(
    bool do_info, bool do_debug) {

  bool any = false;
  any |= (do_info  and (m_callback_log_info  != 0));
  any |= (do_debug and (m_callback_log_debug != 0));
  if(not any) {
    return;
  }

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  std::ostringstream oss;
  auto itds = m_data_streams.begin();
  for(; itds!= m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      oss << "configurator: possible values: data_stream[";
      oss << itds->first << "]";
      oss << ".config[" << itc->first << "]=";

      // avoiding overflow and handling and addition operator's promotion
      // to signed integer
      const int8_t l1 = (int8_t)(itds->first.size() & 0x3f);
      const int8_t l2 = (int8_t)(itc->first.size()  & 0x3f);
      const uint8_t len = (uint8_t)(l1 + l2);

      for(uint16_t ii=0; ii<(len < 40 ? 40-len : 0); ii++) {
        oss << " "; // manual column alignment
      }
      if(itc->second.is_locked()) {
        oss << "(locked) ";
      }
      else {
        oss << "         ";
      }
      oss << itc->second.get_ranges_possible() << "\n";
    }
  }
  for(auto it = m_configs.begin(); it != m_configs.end(); it++){
    oss << "configurator: possible values: config[" << it->first << "]=";
    const size_t len = it->first.size();
    for(size_t ii=0; ii<(len < 54 ? 54-len : 0); ii++) {
      oss << " "; // manual column alignment
    }
    if(it->second.is_locked()) {
      oss << "(locked) ";
    }
    else {
      oss << "         ";
    }
    oss << it->second.get_ranges_possible() << "\n";
  }
  if(do_info) {
    log_info(oss.str().c_str());
  }
  if(do_debug) {
    log_debug(oss.str().c_str());
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::find_data_streams_of_type(
    const data_stream_type_t       type,
    std::vector<data_stream_ID_t>& data_streams) const {

  auto it=m_data_streams.begin();
  for(; it != m_data_streams.end(); it++) {
    if(it->second.get_type() == type) {
      data_streams.push_back(it->first);
    }
  }
}

template<class DS, class LC>
const ConfigValueRanges& Configurator<DS, LC>::get_ranges_possible(
    const data_stream_ID_t data_stream_key,
    const config_key_t     config_key) {
  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return get_config(data_stream_key, config_key).get_ranges_possible();
}

template<class DS, class LC>
const ConfigValueRanges& Configurator<DS, LC>::get_ranges_possible(
    const config_key_t config_key) {
  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return get_config(config_key).get_ranges_possible();
}

/*template<class DS, class LC>
void Configurator<DS, LC>::set_forwarding_callback_log_info(
    const LC callback_func_ptr) {

  LogForwarder<LC>::set_forwarding_callback_log_info(callback_func_ptr);

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.set_forwarding_callback_log_info(callback_func_ptr);
  }

  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.set_forwarding_callback_log_info(callback_func_ptr);
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::set_forwarding_callback_log_debug(
    const LC callback_func_ptr) {

  LogForwarder<LC>::set_forwarding_callback_log_debug(callback_func_ptr);

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.set_forwarding_callback_log_debug(callback_func_ptr);
  }

  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.set_forwarding_callback_log_debug(callback_func_ptr);
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::set_forwarding_callback_log_trace(
    const LC callback_func_ptr) {

  LogForwarder<LC>::set_forwarding_callback_log_trace(callback_func_ptr);

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.set_forwarding_callback_log_trace(callback_func_ptr);
  }

  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.set_forwarding_callback_log_trace(callback_func_ptr);
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::set_forwarding_callback_log_warn(
    const LC callback_func_ptr) {

  LogForwarder<LC>::set_forwarding_callback_log_warn(callback_func_ptr);

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.set_forwarding_callback_log_warn(callback_func_ptr);
  }

  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.set_forwarding_callback_log_warn(callback_func_ptr);
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::set_forwarding_callback_log_error(
    const LC callback_func_ptr) {

  LogForwarder<LC>::set_forwarding_callback_log_error(callback_func_ptr);

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.set_forwarding_callback_log_error(callback_func_ptr);
  }

  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.set_forwarding_callback_log_error(callback_func_ptr);
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::clear_forwarding_callback_log_info() {

  LogForwarder<LC>::clear_forwarding_callback_log_info();

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.clear_forwarding_callback_log_info();
  }
  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.clear_forwarding_callback_log_info();
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::clear_forwarding_callback_log_debug() {

  LogForwarder<LC>::clear_forwarding_callback_log_debug();

  for(auto it=m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.clear_forwarding_callback_log_debug();
  }
  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.clear_forwarding_callback_log_debug();
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::clear_forwarding_callback_log_trace() {

  LogForwarder<LC>::clear_forwarding_callback_log_trace();

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.clear_forwarding_callback_log_trace();
  }
  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.clear_forwarding_callback_log_trace();
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::clear_forwarding_callback_log_warn() {

  LogForwarder<LC>::clear_forwarding_callback_log_warn();

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.clear_forwarding_callback_log_warn();
  }
  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.clear_forwarding_callback_log_warn();
    }
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::clear_forwarding_callback_log_error() {

  LogForwarder<LC>::clear_forwarding_callback_log_error();

  for(auto it = m_configs.begin(); it!= m_configs.end(); it++) {
    it->second.clear_forwarding_callback_log_error();
  }
  auto itds = m_data_streams.begin();
  for(; itds != m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      itc->second.clear_forwarding_callback_log_error();
    }
  }
}*/

template<class DS, class LC>
void Configurator<DS, LC>::ensure_impose_constraints_first_run_did_occur() {

  if(not m_impose_constraints_first_run_did_occur) {
    /// @todo / FIXME - remove this hack(fixes ranges just after construction)
    unlock_all(); // (which calls impose_constraints())

    //impose_constraints();
    // throw_if_any_possible_ranges_are_empty();

    m_impose_constraints_first_run_did_occur = true;
  }
}

template<class DS, class LC>
void Configurator<DS, LC>::throw_if_any_possible_ranges_are_empty(
    const char* calling_func_c_str) const {

  std::string err_str("empty set detected for possible ranges ");

  for(auto it=m_configs.begin(); it!= m_configs.end(); it++) {
    if(it->second.get_ranges_possible().size() == 0) {
      err_str += "for config ";
      err_str += it->first.c_str();
      if(calling_func_c_str != 0) {
        err_str += " in ";
        err_str += calling_func_c_str;
        err_str += "()";
      }
      throw err_str;
    }
  }
  auto itds = m_data_streams.begin();
  for(; itds!= m_data_streams.end(); itds++) {
    auto itc = itds->second.m_configs.begin();
    for(; itc != itds->second.m_configs.end(); itc++) {
      if(itc->second.get_ranges_possible().size() == 0) {
        err_str += "for data stream ";
        err_str += itds->first.c_str();
        err_str += " for config ";
        err_str += itc->first.c_str();
        if(calling_func_c_str != 0) {
          err_str += " in ";
          err_str += calling_func_c_str;
          err_str += "()";
        }
        throw err_str;
      }
    }
  }
}

/*! @param[in] config      Reference to config object.
 *  @param[in] cfg_val     Config value.
 *  @param[in] cfg_val_tol Config value tolerance.
 ******************************************************************************/
template<class DS, class LC>
bool Configurator<DS, LC>::lock_config(
    LockRConstrConfig& config,
    const config_value_t cfg_val,
    const config_value_t cfg_val_tol) {
  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  bool was_originally_locked = config.is_locked();
  bool lock_was_successful = false;

  for(unsigned att = 0; att < 1; att++) { /// @todo / FIXME remove this uneccessary loop

    std::string msg; // for logging

    // first attempt to lock exact requested value...
    lock_was_successful = config.lock(cfg_val, 0.);

    // ... and if that fails, find all ranges which we know will lock, and if
    // there are any that will lock, the value exactly in the middle of the range
    // with the largest window is what will be locked (here in the configurator),
    // the value in the middle is chosen so that, once the configurator-locked
    // value is attempted to be locked by the ctrlr, the ctrlr will have the
    // highest probably of success because the configurator-locked value will be
    // farthest away from the tolerance edges
    if(not lock_was_successful) {
      ConfigValueRanges valid_ranges;
      valid_ranges.add_valid_range(cfg_val-cfg_val_tol, cfg_val+cfg_val_tol);

      // this will contain all ranges which we know will lock
      ConfigValueRanges possible_valid_ranges = config.get_ranges_possible();
      possible_valid_ranges.overlap(valid_ranges);

      auto it = possible_valid_ranges.m_ranges.begin();
      auto it_largest_window = possible_valid_ranges.m_ranges.end();
      config_value_t max_window_size = -1.;
      for(; it != possible_valid_ranges.m_ranges.end(); ++it) {
        config_value_t window_size = it->get_max() - it->get_min();
        if(window_size > max_window_size) {
          max_window_size = window_size;
          it_largest_window = it;
        }
      }

      if(it_largest_window != possible_valid_ranges.m_ranges.end()) {
        config_value_t val_in_middle = it_largest_window->get_min();
        val_in_middle += it_largest_window->get_max();
        val_in_middle /= 2.;

        // this is expected to succeed
        lock_was_successful = config.lock(val_in_middle, 0.);
      }
    }

    if(lock_was_successful) {

      bool do_unroll = false;
      try {
        impose_constraints();

        // impose_constraints should call this function many times, but it's
        // called again here for robustness / just in case
        throw_if_any_possible_ranges_are_empty();
      }
      catch(std::string& err) {
        do_unroll = true;
        msg.assign("configurator: caugh exception during constraints ");
        msg += "imposition, WHICH LIKELY MEANS REQUESTED CONFIG LOCK VALUE IS ";
        msg += "NOT ACCEPTABLE, performing lock unroll now";
        log_debug("%s (%s)", msg.c_str(), err.c_str());
      }
      catch(...) {
        do_unroll = true;
        msg.assign("configurator: attempting to mitigate UNEXPECTED bad ");
        msg += "state caused by unknown problem after last ";
        msg += "lock, unrolling last lock now";
        log_debug(msg.c_str());
      }

      if(do_unroll) { // undo lock and rollback possible ranges
        log_debug("configurator: unrolling lock");
        lock_was_successful = false;
        //log_all_possible_config_values(false, true);
        if(not was_originally_locked) {
          log_debug("configurator: unlocking config");
          config.unlock();
          std::ostringstream ostr;
          ostr << "configurator: config possible ranges are now: ";
          ostr << config.get_ranges_possible();
          log_debug(ostr.str().c_str());
          std::ostringstream ostr2;
          ostr2 << "configurator: config constrained ranges are now: ";
          ostr2 << config.get_ranges_constrained();
          log_debug(ostr2.str().c_str());
        }
        for(auto it=m_configs.begin(); it!= m_configs.end(); it++) {
          ConfigValueRanges vrs = it->second.get_ranges_possible();
          it->second.set_constrained_to_absolute();
          if(it->second.is_locked()) {
            // this is hackish and only necessary because
            // set_constrained_to_absolute() allows us to do bad things
            it->second.set_ranges_possible(vrs);
          }
        }
        auto itds = m_data_streams.begin();
        for(; itds!= m_data_streams.end(); itds++) {
          auto itc = itds->second.m_configs.begin();
          for(; itc != itds->second.m_configs.end(); itc++) {
            ConfigValueRanges vrs = itc->second.get_ranges_possible();
            itc->second.set_constrained_to_absolute();
            if(itc->second.is_locked()) {
              // this is hackish and only necessary because
              // set_constrained_to_absolute() allows us to do bad things
              itc->second.set_ranges_possible(vrs);
            }
          }
        }
        {
          impose_constraints();

          // impose_constraints should call this function many times, but it's
          // called again here for robustness / just in case
          throw_if_any_possible_ranges_are_empty();
        }

        //log_all_possible_config_values(false, true);
      }
      else {
        // configurator successful, ranges in good state (i.e. none are empty)
        return true;
      }
    }
    else {
      if(att == 1) {
        // if configurator not succesful for offset=0,tolerance=tolerance, it
        // will never succeed, so go ahead and break
        break;
      }
    }
  }
  return false;
}

template<class DS, class LC>
void Configurator<DS, LC>::impose_constraints() {

  bool en = true; // enable first iteration loop
  const int max = max_constraint_dependency_depth;
  for(int iter_num=0; en & (iter_num < max); iter_num++) {
    auto configs_orig = m_configs;
    data_streams_t data_streams_orig = m_data_streams;

    impose_constraints_single_pass();

    log_debug("configurator: %s(): constraint dependency iteration: %i", __func__, iter_num);

    // unless a config change is detected, disable next loop, meaning all
    // config dependencies were accounted for
    en = false;

    //bool global_config_changed = get_config_changed(config_orig, config_new);
    //bool global_config_changed = not(configs_orig == m_configs);
    for(auto it=m_configs.begin(); it!= m_configs.end(); it++) {

      LockRConstrConfig& config_new = it->second;
      LockRConstrConfig& config_old = configs_orig.at(it->first);

      bool global_config_changed = (not(config_new == config_old));
      if(global_config_changed) {
        // enable next loop to account for next potential propagated config
        // dependency
        en = true;
        // if logging, we don't break so we can continue the loop and see
        // the print out of all the configs that changed
        if(m_callback_log_debug == 0) {
          break;
        }
      }
      if(m_callback_log_debug != 0) {
        std::ostringstream oss;
        oss << "ranges for config:\t" << it->first.c_str();
        oss << " =\t"  << config_new.get_ranges_possible();
        if(global_config_changed) {
          oss << "\t** note change from previous ranges: ";
          oss << config_old.get_ranges_possible();
        }
        log_debug("configurator: %s(): %s", __func__, oss.str().c_str());
      }
    }
    if(en) {
      // if logging, we don't continue so we can continue the loop and see
      // the print out of all the configs that changed
      if(m_callback_log_debug == 0) {
        continue;
      }
    }

    auto itds = m_data_streams.begin();
    for(; itds!= m_data_streams.end(); itds++) {

      DS stream_orig = data_streams_orig.at(itds->first);

      auto itc = itds->second.m_configs.begin();
      for(; itc != itds->second.m_configs.end(); itc++) {

        LockRConstrConfig& config_new = itc->second;
        LockRConstrConfig& config_old = stream_orig.m_configs.at(itc->first);

        bool data_stream_config_changed = (not(config_new == config_old));
        if(data_stream_config_changed) {
          // enable next loop to account for next potential propagated config
          // dependency
          en = true;
          // if logging, we don't break so we can continue the loop and see
          // the print out of all the configs that changed
          if(m_callback_log_debug == 0) {
            break;
          }
        }
        if(m_callback_log_debug != 0) {
          std::ostringstream oss;
          oss << "ranges for config:\t" << itc->first.c_str();
          oss << "\tfor data stream:\t" << itds->first.c_str();
          oss << " =\t" << config_new.get_ranges_possible();
          if(data_stream_config_changed) {
            oss << "\t** note change from previous ranges: ";
            oss << config_old.get_ranges_possible();
          }
          log_debug("configurator: %s(): %s", __func__, oss.str().c_str());
        }
      }
    }
    if(en) {
      continue;
    }
  }
  //log_debug(en ? "en=true\n" : "en=false\n");
  if(en) { // max_constraint_dependency_depth exceeded
    // the intent is that constraint relationships are defined
    // such that this never happens, although this occurence
    // is theoretically possible for insanely complicated
    // constraints w/ many configs
    throw std::string("max constraint dependency depth exceeded");
  }
}

template<class DS, class LC>
config_value_t Configurator<DS, LC>::get_config_min_valid_value(
    const LockRConstrConfig& config) {

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return config.get_ranges_possible().get_smallest_min();
}

template<class DS, class LC>
config_value_t Configurator<DS, LC>::get_config_max_valid_value(
    const LockRConstrConfig& config) {

  // required because this method's behavior could be affected by
  // the impose_constraints() call we couldn't put in the
  // constructor
  ensure_impose_constraints_first_run_did_occur();

  return config.get_ranges_possible().get_largest_max();
}

template<class DS, class LC>
LockRConstrConfig& Configurator<DS, LC>::get_config(
    const data_stream_ID_t data_stream_key,
    const config_key_t     cfg_key) {

  DS* data_stream;
  try {
    data_stream = &(m_data_streams.at(data_stream_key));
  }
  catch(const std::out_of_range& err) {
    std::ostringstream oss;
    oss << "configurator: attempted to access non-existent data stream \"";
    oss << data_stream_key << "\"";
    oss << " (available data streams are: ";
    bool first = true;
    for(auto it=m_data_streams.begin(); it!=m_data_streams.end(); it++) {
      oss << (first ? "" : ", ") << it->first;
      first = false;
    }
    oss << ")";
    throw oss.str();
  }
  try {
    return data_stream->m_configs.at(cfg_key);
  }
  catch(const std::out_of_range& err) {
    std::ostringstream oss;
    oss << "configurator: attempted to access non-existent config \"";
    oss << cfg_key << "\" for data stream \"" << data_stream_key;
    oss << "\"";
    try {
      m_configs.at(cfg_key);
      oss << " (did you mean to access the data ";
      oss << "stream-agnostic \"" << cfg_key << "\" config?)";
      throw oss.str();
    }
    catch(std::out_of_range) {
      oss << " (available configs are: ";
      bool first = true;
      auto it=data_stream->m_configs.begin();
      for(; it!=data_stream->m_configs.end(); it++) {
        oss << (first ? "" : ", ") << it->first;
        first = false;
      }
      oss << ")";
      throw oss.str();
    }
  }
  // we repeat this line only because otherwise control reaches end of
  // non-void function
  return m_data_streams.at(data_stream_key).m_configs.at(cfg_key);
}

template<class DS, class LC>
const LockRConstrConfig& Configurator<DS, LC>::get_config(
    const data_stream_ID_t data_stream_key,
    const config_key_t     cfg_key) const {
  
  auto it = m_data_streams.find(data_stream_key);
  if(it == m_data_streams.end()) {
    std::ostringstream oss;
    oss << "configurator: attempted to access non-existent data stream \"";
    oss << data_stream_key << "\"";
    oss << " (available data streams are: ";
    bool first = true;
    for(auto it2=m_data_streams.begin(); it2!=m_data_streams.end(); it2++) {
      oss << (first ? "" : ", ") << it2->first;
      first = false;
    }
    oss << ")";
    throw oss.str();
  }
  auto it3 = it->second.m_configs.find(cfg_key);
  if(it3 == it->second.m_configs.end()) {
    std::ostringstream oss;
    oss << "configurator: attempted to access non-existent config \"";
    oss << cfg_key << "\" for data stream \"" << data_stream_key;
    oss << "\"";
    auto it4 = m_configs.find(cfg_key);
    if(it4 != m_configs.end()) {
      oss << " (did you mean to access the data ";
      oss << "stream-agnostic \"" << cfg_key << "\" config?)";
      throw oss.str();
    }
    else {
      oss << " (available configs are: ";
      bool first = true;
      auto it5=it->second.m_configs.begin();
      for(; it5!=it->second.m_configs.end(); it5++) {
        oss << (first ? "" : ", ") << it5->first;
        first = false;
      }
      oss << ")";
      throw oss.str();
    }
  }
  const LockRConstrConfig& r = it3->second;
  return r;
}

template<class DS, class LC>
LockRConstrConfig& Configurator<DS, LC>::get_config(const config_key_t cfg_key){
  try {
    return m_configs.at(cfg_key); // throws std::out_of_range no match
  }
  catch(const std::out_of_range& err) {
    std::ostringstream oss;
    oss << "configurator: attempted to access non-existent data ";
    oss << "stream-agnostic config \"" << cfg_key << "\"";
    try {
      auto it = m_data_streams.begin();
      for(; it != m_data_streams.end(); it++){
        it->second.m_configs.at(cfg_key); // throws std::out_of_range if no match
        oss << " (did you mean to access the data ";
        oss << "stream-specific \"" << cfg_key << " config?)";
        throw oss.str(); // finally throw w/ useful error message
      }
    }
    catch(...) {
    }
    oss << " (available configs are: ";
    bool first = true;
    for(auto it = m_configs.begin(); it != m_configs.end(); it++) {
      oss << (first ? "" : ", ") << it->first;
      first = false;
    }
    oss << ")";
    throw oss.str(); // finally throw w/ useful error message
  }
  // repeat this line otherwise control reaches end of non-void
  // function
  return m_configs.at(cfg_key);
}

template<class DS, class LC>
const LockRConstrConfig& Configurator<DS, LC>::get_config(
    const config_key_t cfg_key) const {

  auto it = m_configs.find(cfg_key); // strong guarantee
  if(it != m_configs.end()) {
    return it->second;
  }
  else {
    std::ostringstream oss;
    oss << "configurator: attempted to access non-existent data ";
    oss << "stream-agnostic config \"" << cfg_key << "\"";
    auto it3 = m_data_streams.begin();
    for(; it3 != m_data_streams.end(); it3++){
      auto it2 = it3->second.m_configs.find(cfg_key); // strong guarantee
      if(it2 != m_configs.end()) {
        oss << " (did you mean to access the data ";
        oss << "stream-specific \"" << cfg_key << " config?)";
        throw oss.str(); // finally throw w/ useful error message
      }
    }
    oss << " (available configs are: ";
    bool first = true;
    for(auto it4 = m_configs.begin(); it4 != m_configs.end(); it++) {
      oss << (first ? "" : ", ") << it4->first;
      first = false;
    }
    oss << ")";
    throw oss.str(); // finally throw w/ useful error message
  }
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_all_XY_such_that_X_equals_Y(
    LockRConstrConfig& cfg_X, LockRConstrConfig& cfg_Y) const {
  bool changed = false;
  changed|=cfg_X.overlap_constrained(cfg_Y.get_ranges_possible());
  changed|=cfg_Y.overlap_constrained(cfg_X.get_ranges_possible());
  return changed;
};

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_X_to_function_of_A_and_B(
    LockRConstrConfig& cfg_X,
    const LockRConstrConfig& cfg_A, const LockRConstrConfig& cfg_B,
    constraint_op_t op) const {

  // newly constrained ConfigValidRanges that will be applied to X
  ConfigValueRanges new_constrained_ranges_X;

  {
    ConfigValueRanges possible_ranges_A = cfg_A.get_ranges_possible();
    ConfigValueRanges possible_ranges_B = cfg_B.get_ranges_possible();

    auto it_A = possible_ranges_A.m_ranges.begin();
    for(; it_A != possible_ranges_A.m_ranges.end(); it_A++) {

      auto it_B = possible_ranges_B.m_ranges.begin();
      for(; it_B != possible_ranges_B.m_ranges.end(); it_B++) {

        config_value_t min, max, tmp1, tmp2;

        switch(op) {
          case constraint_op_t::multiply:
            {
              tmp1 = it_A->get_min() * it_B->get_min();
              tmp2 = it_A->get_max() * it_B->get_min();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() * it_B->get_max();
              tmp2 = it_A->get_max() * it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() * it_B->get_min();
              tmp2 = it_A->get_min() * it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_max() * it_B->get_min();
              tmp2 = it_A->get_max() * it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);
            }
            break;
          case constraint_op_t::divide:
            {
              tmp1 = it_A->get_min() / it_B->get_min();
              tmp2 = it_A->get_max() / it_B->get_min();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() / it_B->get_max();
              tmp2 = it_A->get_max() / it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              if(it_B->get_min() < 0.) {
                min = it_A->get_min() / it_B->get_min();
                max = it_A->get_min() / it_B->get_max();
              }
              else {
                min = it_A->get_min() / it_B->get_max();

                // because it_B->get_min() is guaranteed to be positive,
                // and it_B->get_max() is guaranteed to be > it_B->get_min,
                // max variable is guaranteed to be > min variable
                max = it_A->get_min() / it_B->get_min();
              }
              if(it_B->is_valid(0.)) {
                max = std::numeric_limits<config_value_t>::max();
              }
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              if(it_B->get_min() < 0.) {
                min = it_A->get_max() / it_B->get_min();
                max = it_A->get_max() / it_B->get_max();
              }
              else {
                min = it_A->get_max() / it_B->get_max();
                max = it_A->get_max() / it_B->get_min();
              }
              if(it_B->is_valid(0.)) {
                max = std::numeric_limits<config_value_t>::max();
              }
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);
            }
            break;
          case constraint_op_t::plus:
            {
              tmp1 = it_A->get_min() + it_B->get_min();
              tmp2 = it_A->get_max() + it_B->get_min();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() + it_B->get_max();
              tmp2 = it_A->get_max() + it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() + it_B->get_min();
              tmp2 = it_A->get_min() + it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_max() + it_B->get_min();
              tmp2 = it_A->get_max() + it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);
            }
            break;
          case constraint_op_t::minus:
            {
              tmp1 = it_A->get_min() - it_B->get_min();
              tmp2 = it_A->get_max() - it_B->get_min();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() - it_B->get_max();
              tmp2 = it_A->get_max() - it_B->get_max();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_min() - it_B->get_max();
              tmp2 = it_A->get_min() - it_B->get_min();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);

              tmp1 = it_A->get_max() - it_B->get_max();
              tmp2 = it_A->get_max() - it_B->get_min();
              max = (tmp1 > tmp2) ? tmp1 : tmp2;
              min = (tmp1 > tmp2) ? tmp2 : tmp1;
              min = (min == -0) ? 0 : min;
              max = (max == -0) ? 0 : max;
              new_constrained_ranges_X.add_valid_range(min, max);
            }
            break;
          default:
            break;
        }
      }
    }
  }

  if(not cfg_X.is_locked()) {
    return cfg_X.overlap_constrained(new_constrained_ranges_X);
  }
  return false;
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_X_to_A_divided_by_B(
    LockRConstrConfig& cfg_X,
    const LockRConstrConfig& cfg_A, const LockRConstrConfig& cfg_B) const {

  return constrain_X_to_function_of_A_and_B(cfg_X, cfg_A, cfg_B, constraint_op_t::divide);
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_all_XAB_such_that_X_equals_A_multiplied_by_B(
    LockRConstrConfig& cfg_X,
    LockRConstrConfig& cfg_A, LockRConstrConfig& cfg_B) const {

  bool did_change = false;

  // X is constrained by A*B
  did_change |= constrain_X_to_function_of_A_and_B(cfg_X, cfg_A, cfg_B, constraint_op_t::multiply);

  // A is constrained by X/B
  did_change |= constrain_X_to_function_of_A_and_B(cfg_A, cfg_X, cfg_B, constraint_op_t::divide);

  // B is constrained by X/A
  did_change |= constrain_X_to_function_of_A_and_B(cfg_B, cfg_X, cfg_A, constraint_op_t::divide);

  return did_change;
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_all_XAB_such_that_X_equals_A_divided_by_B(
    LockRConstrConfig& cfg_X,
    LockRConstrConfig& cfg_A, LockRConstrConfig& cfg_B) const {

  bool did_change = false;

  // X is constrained by A/B
  did_change |= constrain_X_to_function_of_A_and_B(cfg_X, cfg_A, cfg_B, constraint_op_t::divide);

  // A is constrained by X*B
  did_change |= constrain_X_to_function_of_A_and_B(cfg_A, cfg_X, cfg_B, constraint_op_t::multiply);

  // B is constrained by A/X
  did_change |= constrain_X_to_function_of_A_and_B(cfg_B, cfg_A, cfg_X, constraint_op_t::divide);

  return did_change;
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_all_XAB_such_that_X_equals_A_plus_B(
    LockRConstrConfig& cfg_X,
    LockRConstrConfig& cfg_A, LockRConstrConfig& cfg_B) const {

  bool did_change = false;

  // X is constrained by A+B (1/3)
  did_change |= constrain_X_to_function_of_A_and_B(cfg_X, cfg_A, cfg_B, constraint_op_t::plus);

  // A is constrained by X-B (2/3)
  did_change |= constrain_X_to_function_of_A_and_B(cfg_A, cfg_X, cfg_B, constraint_op_t::minus);

  // B is constrained by X-A (3/3)
  did_change |= constrain_X_to_function_of_A_and_B(cfg_B, cfg_X, cfg_A, constraint_op_t::minus);

  return did_change;
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_X_to_A_multiplied_by_B(
    LockRConstrConfig& cfg_X,
    const LockRConstrConfig& cfg_A, const LockRConstrConfig& cfg_B) const {

  return constrain_X_to_function_of_A_and_B(cfg_X, cfg_A, cfg_B, constraint_op_t::multiply);
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_Y_less_than_or_equal_to_X(
    LockRConstrConfig& cfg_Y, LockRConstrConfig& cfg_X) const {

  bool changed = false;
  changed |= limit_Y_to_less_than_or_equal_to_X(cfg_Y, cfg_X);
  changed |= limit_Y_to_greater_than_or_equal_to_X(cfg_X, cfg_Y);

  return changed;
}

template<class DS, class LC>
bool Configurator<DS, LC>::constrain_Y_equals_constant(
    LockRConstrConfig& cfg_Y, const config_value_t constant) const {

  if(not cfg_Y.is_locked()) {

    ConfigValueRanges ranges;
    ranges.add_valid_range(constant,constant);

    return cfg_Y.overlap_constrained(ranges);
  }

  return false;
}

template<class DS, class LC>
bool Configurator<DS, LC>::limit_Y_to_less_than_or_equal_to_X(
    LockRConstrConfig& cfg_Y,
    const LockRConstrConfig& cfg_X) const {

  if(cfg_Y.is_locked()) {
    return false;
  }

  ConfigValueRanges possible_ranges_Y = cfg_Y.get_ranges_possible();
  ConfigValueRanges possible_ranges_X = cfg_X.get_ranges_possible();

  // newly constrained ConfigValidRanges that will be applied to Y
  ConfigValueRanges new_constrained_ranges_Y;

  config_value_t min = possible_ranges_Y.get_smallest_min();
  config_value_t max = possible_ranges_X.get_largest_max();
  if(max >= min) {
    new_constrained_ranges_Y.add_valid_range(min, max);
  }
  // if max < min, new constrained will be empty set, and overlap()
  // will cause Y to be an empty set (which is what we want)

  bool changed = cfg_Y.overlap_constrained(new_constrained_ranges_Y);

  return changed;
}

template<class DS, class LC>
bool Configurator<DS, LC>::limit_Y_to_greater_than_or_equal_to_X(
    LockRConstrConfig& cfg_Y,
    const LockRConstrConfig& cfg_X) const {

  if(cfg_Y.is_locked()) {
    return false;
  }

  ConfigValueRanges possible_ranges_Y = cfg_Y.get_ranges_possible();
  ConfigValueRanges possible_ranges_X = cfg_X.get_ranges_possible();

  // newly constrained ConfigValidRanges that will be applied to Y
  ConfigValueRanges new_constrained_ranges_Y;

  config_value_t min = possible_ranges_X.get_smallest_min();
  config_value_t max = possible_ranges_Y.get_largest_max();
  if(max >= min) {
    new_constrained_ranges_Y.add_valid_range(min, max);
  }
  // if max < min, new constrained will be empty set, and overlap()
  // will cause Y to be an empty set (which is what we want)

  bool changed = cfg_Y.overlap_constrained(new_constrained_ranges_Y);

  return changed;
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
