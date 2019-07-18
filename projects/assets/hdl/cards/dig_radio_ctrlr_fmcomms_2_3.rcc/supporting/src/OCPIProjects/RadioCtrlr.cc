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

#include <vector>        // std::vector
#include <string>        // std::string
#include <sstream>       // std::ostringstream
#include <iostream>      // std::ostream
#include <cmath>         // std::abs for doubles
#include "Meas.hh"       // Meas
#include "RadioCtrlr.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

/*std::ostream& operator<< (std::ostream& os,
    const OCPIProjects::RadioCtrlr::gain_mode_value_t& meas) {

  if(meas == OCPIProjects::RadioCtrlr::gain_mode_value_t::_auto) {
    os << "auto";
  }
  else { // (meas == OCPIProjects::RadioCtrlr::gain_mode_value_t::manual)
    os << "manual";
  }
  return os;
}*/

DataStreamConfigLockRequest::DataStreamConfigLockRequest() :
  m_including_data_stream_type   (false),
  m_including_data_stream_ID     (false),
  m_including_routing_ID         (false),
  m_including_tuning_freq_MHz    (false),
  m_including_bandwidth_3dB_MHz  (false),
  m_including_sampling_rate_Msps (false),
  m_including_samples_are_complex(false),
  m_including_gain_mode          (false),
  m_including_gain_dB            (false) {
}

data_stream_type_t DataStreamConfigLockRequest::get_data_stream_type() const {

  if(not m_including_data_stream_type) {
    throw_for_invalid_get_call("data_stream_type");
  }
  return m_data_stream_type;
}

data_stream_ID_t DataStreamConfigLockRequest::get_data_stream_ID() const {

  if(not m_including_data_stream_ID) {
    throw_for_invalid_get_call("data_stream_ID");
  }
  return m_data_stream_ID;
}

routing_ID_t DataStreamConfigLockRequest::get_routing_ID() const {

  if(not m_including_routing_ID) {
    throw_for_invalid_get_call("routing_ID");
  }
  return m_routing_ID;
}

config_value_t DataStreamConfigLockRequest::get_tuning_freq_MHz() const {

  if(not m_including_tuning_freq_MHz) {
    throw_for_invalid_get_call("tuning_freq_MHz");
  }
  return m_tuning_freq_MHz;
}

config_value_t DataStreamConfigLockRequest::get_bandwidth_3dB_MHz() const {

  if(not m_including_bandwidth_3dB_MHz) {
    throw_for_invalid_get_call("bandwidth_3dB_MHz");
  }
  return m_bandwidth_3dB_MHz;
}

config_value_t DataStreamConfigLockRequest::get_sampling_rate_Msps() const {

  if(not m_including_sampling_rate_Msps) {
    throw_for_invalid_get_call("sampling_rate_Msps");
  }
  return m_sampling_rate_Msps;
}

bool DataStreamConfigLockRequest::get_samples_are_complex() const {
  if(not m_including_samples_are_complex) {
    throw_for_invalid_get_call("samples_are_complex");
  }
  return m_samples_are_complex;
}

gain_mode_value_t DataStreamConfigLockRequest::get_gain_mode() const {

  if(not m_including_gain_mode) {
    throw_for_invalid_get_call("gain_mode");
  }
  return m_gain_mode;
}

config_value_t DataStreamConfigLockRequest::get_gain_dB() const {

  if(not m_including_gain_dB) {
    throw_for_invalid_get_call("gain_dB");
  }
  return m_gain_dB;
}

config_value_t
DataStreamConfigLockRequest::get_tolerance_tuning_freq_MHz() const {

  if(not m_including_tuning_freq_MHz) {
    throw_for_invalid_get_call("tuning_freq_MHz");
  }
  return m_tolerance_tuning_freq_MHz;
}

config_value_t
DataStreamConfigLockRequest::get_tolerance_bandwidth_3dB_MHz() const {

  if(not m_including_bandwidth_3dB_MHz) {
    throw_for_invalid_get_call("bandwidth_3dB_MHz");
  }
  return m_tolerance_bandwidth_3dB_MHz;
}

config_value_t
DataStreamConfigLockRequest::get_tolerance_sampling_rate_Msps() const {

  if(not m_including_sampling_rate_Msps) {
    throw_for_invalid_get_call("sampling_rate_Msps");
  }
  return m_tolerance_sampling_rate_Msps;
}

config_value_t
DataStreamConfigLockRequest::get_tolerance_gain_dB() const {

  if(not m_including_gain_dB) {
    throw_for_invalid_get_call("gain_dB");
  }
  return m_tolerance_gain_dB;
}

bool DataStreamConfigLockRequest::get_including_data_stream_type() const {
  return m_including_data_stream_type;
}

bool DataStreamConfigLockRequest::get_including_data_stream_ID() const {
  return m_including_data_stream_ID;
}

bool DataStreamConfigLockRequest::get_including_routing_ID() const {
  return m_including_routing_ID;
}

bool DataStreamConfigLockRequest::get_including_tuning_freq_MHz() const {
  return m_including_tuning_freq_MHz;
}

bool DataStreamConfigLockRequest::get_including_bandwidth_3dB_MHz() const {
  return m_including_bandwidth_3dB_MHz;
}

bool DataStreamConfigLockRequest::get_including_sampling_rate_Msps() const {
  return m_including_sampling_rate_Msps;
}

bool DataStreamConfigLockRequest::get_including_samples_are_complex() const {
  return m_including_samples_are_complex;
}

bool DataStreamConfigLockRequest::get_including_gain_mode() const {
  return m_including_gain_mode;
}

bool DataStreamConfigLockRequest::get_including_gain_dB() const {
  return m_including_gain_dB;
}

void DataStreamConfigLockRequest::include_data_stream_type(
    const data_stream_type_t data_stream_type) {

  m_data_stream_type = data_stream_type;
  m_including_data_stream_type = true;
}

void DataStreamConfigLockRequest::include_data_stream_ID(
    const data_stream_ID_t data_stream_ID) {

  m_data_stream_ID = data_stream_ID;
  m_including_data_stream_ID = true;
}

void DataStreamConfigLockRequest::include_routing_ID(
    const routing_ID_t routing_ID) {

  m_routing_ID = routing_ID;
  m_including_routing_ID = true;
}

void DataStreamConfigLockRequest::include_tuning_freq_MHz(
    const config_value_t tuning_freq_MHz,
    const config_value_t tolerance_tuning_freq_MHz) {

  m_tuning_freq_MHz           = tuning_freq_MHz;
  m_tolerance_tuning_freq_MHz = tolerance_tuning_freq_MHz;
  m_including_tuning_freq_MHz = true;
}

void DataStreamConfigLockRequest::include_bandwidth_3dB_MHz(
    const config_value_t bandwidth_3dB_MHz,
    const config_value_t tolerance_bandwidth_3dB_MHz) {

  m_bandwidth_3dB_MHz           = bandwidth_3dB_MHz;
  m_tolerance_bandwidth_3dB_MHz = tolerance_bandwidth_3dB_MHz;
  m_including_bandwidth_3dB_MHz = true;
}

void DataStreamConfigLockRequest::include_sampling_rate_Msps(
    const config_value_t sampling_rate_Msps,
    const config_value_t tolerance_sampling_rate_Msps) {

  m_sampling_rate_Msps           = sampling_rate_Msps;
  m_tolerance_sampling_rate_Msps = tolerance_sampling_rate_Msps;
  m_including_sampling_rate_Msps = true;
}
void DataStreamConfigLockRequest::include_samples_are_complex(
    const bool samples_are_complex) {

  m_samples_are_complex = samples_are_complex;
  m_including_samples_are_complex = true;
}

void DataStreamConfigLockRequest::include_gain_mode(
    const gain_mode_value_t gain_mode) {

  m_gain_mode = gain_mode;
  m_including_gain_mode = true;
}

void DataStreamConfigLockRequest::include_gain_dB(
    const config_value_t gain_dB,
    const config_value_t tolerance_gain_dB) {

  m_gain_dB           = gain_dB;
  m_tolerance_gain_dB = tolerance_gain_dB;
  m_including_gain_dB = true;
}

void DataStreamConfigLockRequest::throw_for_invalid_get_call(
    const char* config) const {

  std::ostringstream oss;
  oss << "attempted to read radio controller's config request data ";
  oss << "stream's ";
  oss << config;
  oss << ", which was never included as a part of the request";
  throw oss.str();
}

template<class LC, class C>
AnaRadioCtrlr<LC, C>::AnaRadioCtrlr(const char* descriptor,
    C& configurator) :
    m_descriptor(descriptor), m_configurator(configurator) {
}

template<class LC, class C>
const std::string& AnaRadioCtrlr<LC, C>::get_descriptor() const {
  return m_descriptor;
}

template<class LC, class C>
const std::vector<ConfigLock>& AnaRadioCtrlr<LC, C>::get_config_locks() const {
  return m_config_locks;
}

template<class LC, class C>
ConfigValueRanges
AnaRadioCtrlr<LC, C>::get_ranges_possible_tuning_freq_MHz(
    const data_stream_ID_t data_stream_ID) const {

  auto key = config_key_tuning_freq_MHz;
  return m_configurator.get_ranges_possible(data_stream_ID, key);
}

template<class LC, class C>
ConfigValueRanges
AnaRadioCtrlr<LC, C>::get_ranges_possible_bandwidth_3dB_MHz(
    const data_stream_ID_t data_stream_ID) const {

  auto key = config_key_bandwidth_3dB_MHz;
  return m_configurator.get_ranges_possible(data_stream_ID, key);
}

template<class LC, class C>
std::vector<gain_mode_value_t>
AnaRadioCtrlr<LC, C>::get_ranges_possible_gain_mode(
    const data_stream_ID_t data_stream_ID) const {

  std::vector<gain_mode_value_t> ret;
  auto key = config_key_gain_mode;
  auto vr = this->m_configurator.get_ranges_possible(data_stream_ID, key);

  if(vr.is_valid(0)) {
    ret.push_back("auto");
  }
  if(vr.is_valid(1)) {
    ret.push_back("manual");
  }
  return ret;
}

template<class LC, class C>
ConfigValueRanges
AnaRadioCtrlr<LC, C>::get_ranges_possible_gain_dB(
    const data_stream_ID_t data_stream_ID) const {

  auto key = config_key_gain_dB;
  return m_configurator.get_ranges_possible(data_stream_ID, key);
}

/*template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_all_possible_config_values() {
  m_configurator.log_all_possible_config_values();
}*/

template<class LC, class C>
bool AnaRadioCtrlr<LC, C>::request_config_lock(
    config_lock_ID_t         config_lock_ID,
    const ConfigLockRequest& config_lock_request) {

  ConfigLock config_lock;
  config_lock.m_config_lock_ID = config_lock_ID;

  bool configurator_config_lock_request_was_successful = false;

  auto it = config_lock_request.m_data_streams.begin(); 
  for(; it != config_lock_request.m_data_streams.end(); it++) {

    throw_if_data_stream_lock_request_malformed(*it);

    std::vector<data_stream_ID_t> data_streams;

    if(it->get_including_data_stream_type()) {
      this->m_configurator.find_data_streams_of_type(it->get_data_stream_type(), data_streams);

      if(data_streams.empty()) {
        // configurator did not have any data streams of the requested data
        // stream type
        return false;
      }
    }
    else { // assuming including data stream ID
      data_streams.push_back(it->get_data_stream_ID());
    }

    bool found_lock = false;
    auto it_found_streams = data_streams.begin();
    for(; it_found_streams != data_streams.end(); it_found_streams++) {

      found_lock |= do_min_data_stream_config_locks(*it_found_streams, *it);

      const char* ds = it_found_streams->c_str();
      if(found_lock) {
        log_info("data stream %s meet data stream config lock request requirements\n", ds);
        DataStreamConfigLock data_stream_config_lock;

        data_stream_config_lock.m_data_stream_ID      = ds;
        data_stream_config_lock.m_tuning_freq_MHz     = it->get_tuning_freq_MHz();
        data_stream_config_lock.m_bandwidth_3dB_MHz   = it->get_bandwidth_3dB_MHz();

        config_lock.m_data_streams.push_back(data_stream_config_lock);
        break;
      }
      log_info("data stream %s did not meet data stream config lock request requirements\n", ds);
    }
    if(not found_lock) {
      configurator_config_lock_request_was_successful = false;
      break;
    }
    configurator_config_lock_request_was_successful = true;
  }

  if(configurator_config_lock_request_was_successful) {
    m_config_locks.push_back(config_lock);
    log_info("request config lock %s succeeeded", config_lock_ID.c_str());
    return true;
  }

  return false;
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::unlock_config_lock(
    const config_lock_ID_t config_lock_ID) {

  bool found_config_lock = false;
  for(auto itcl = m_config_locks.begin(); itcl != m_config_locks.end(); itcl++){
    if(itcl->m_config_lock_ID.compare(config_lock_ID) == 0) {
      found_config_lock = true;
      auto itds = itcl->m_data_streams.begin();
      for(; itds != itcl->m_data_streams.end(); itds++) {
        unlock_tuning_freq_MHz  (itds->m_data_stream_ID);
        unlock_bandwidth_3dB_MHz(itds->m_data_stream_ID);
      }
      break;
    }
  }
  if(not found_config_lock) {
    std::ostringstream oss;
    oss << "for config unlock request, config lock ID " << config_lock_ID;
    oss << " not found";
    throw oss.str();
  }
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::unlock_all() {

  log_debug("configurator: unlock_all2");
  this->m_configurator.unlock_all();
  m_config_locks.clear();
}

template<class LC, class C>
bool AnaRadioCtrlr<LC, C>::lock_tuning_freq_MHz(
    const data_stream_ID_t ds_ID,
    const config_value_t   val,
    const config_value_t   tol) {

  // the configurator, which is a software emulation of hardware capabilties,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = config_key_tuning_freq_MHz;
  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, val, tol);

  bool is = false; // is within tolerance
  if(did_lock) {
    config_value_t cfglval; // configurator locked value

    /// @todo / FIXME - implement m_configurator.get_locked_value()
    cfglval = this->m_configurator.get_config_min_valid_value(ds_ID, cfg_key);

    Meas<config_value_t> meas = set_tuning_freq_MHz(ds_ID, cfglval);

    is = config_val_is_within_tolerance(val, tol, meas);

    log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "MHz", &meas);
  }
  else {
    log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "MHz");
  }

  return did_lock and is;
}

template<class LC, class C>
bool AnaRadioCtrlr<LC, C>::lock_bandwidth_3dB_MHz(
    const data_stream_ID_t ds_ID,
    const config_value_t   val,
    const config_value_t   tol) {

  // the configurator, which is a software emulation of hardware capabilties,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = config_key_bandwidth_3dB_MHz;
  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, val, tol);

  bool is = false; // is within tolerance
  if(did_lock) {
    config_value_t cfglval; // configurator locked value

    /// @todo / FIXME - implement m_configurator.get_locked_value()
    cfglval = this->m_configurator.get_config_min_valid_value(ds_ID, cfg_key);

    Meas<config_value_t> meas = set_bandwidth_3dB_MHz(ds_ID, cfglval);

    is = config_val_is_within_tolerance(val, tol, meas);

    log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "MHz", &meas);
  }
  else {
    log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "MHz");
  }

  return did_lock and is;
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::unlock_tuning_freq_MHz(
    const data_stream_ID_t data_stream_ID) {
  
  unlock_config(data_stream_ID, config_key_tuning_freq_MHz);
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::unlock_bandwidth_3dB_MHz(
    const data_stream_ID_t data_stream_ID) {

  unlock_config(data_stream_ID, config_key_bandwidth_3dB_MHz);
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::unlock_config(
    const data_stream_ID_t ds_ID,
    const config_key_t     cfg_key) {
  
  log_info("for data stream %s: unlocking config %s", ds_ID.c_str(), cfg_key.c_str());
  this->m_configurator.unlock_config(ds_ID, cfg_key);
}

template<class LC, class C>
bool AnaRadioCtrlr<LC, C>::do_min_data_stream_config_locks(
    const data_stream_ID_t data_stream_ID,
    const DataStreamConfigLockRequest& data_stream_config_lock_request) {

  const DataStreamConfigLockRequest& req = data_stream_config_lock_request;

  throw_if_data_stream_lock_request_malformed(req);

  {
    const config_value_t& val = req.get_tuning_freq_MHz();
    const config_value_t& tol = req.get_tolerance_tuning_freq_MHz();

    if(not lock_tuning_freq_MHz(data_stream_ID, val, tol)) {
      goto unrollandfail;
    }
  }
  {
    const config_value_t& val = req.get_bandwidth_3dB_MHz();
    const config_value_t& tol = req.get_tolerance_bandwidth_3dB_MHz();

    if(not lock_bandwidth_3dB_MHz(data_stream_ID, val, tol)) {
      goto unrollandfail;
    }
  }

  return true;

  unrollandfail:
  unlock_tuning_freq_MHz  (data_stream_ID);
  unlock_bandwidth_3dB_MHz(data_stream_ID);
  return false;
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::throw_if_data_stream_lock_request_malformed(
    const DataStreamConfigLockRequest& data_stream_config_lock_request) const {

  const DataStreamConfigLockRequest& req = data_stream_config_lock_request;

  if(not req.get_including_data_stream_type() and
     not req.get_including_data_stream_ID()) {
    throw std::string("radio controller's data stream config lock request malformed: did not include at least one of: data_stream_type (RX or TX), data_stream_ID");
  }
  // note that including routing ID is not *universally* necessary, so
  // we don't check for it here (it's really only necessary for radio
  // controllers w/ multiple data streams of the same type)
  if(not req.get_including_tuning_freq_MHz()) {
    throw std::string("radio controller's data stream config lock request malformed: did not include tuning_freq_MHz");
  }
  if(not req.get_including_bandwidth_3dB_MHz()) {
    throw std::string("radio controller's data stream config lock request malformed: did not include bandwidth_3dB_MHz");
  }
}

template<class LC, class C>
bool AnaRadioCtrlr<LC, C>::config_val_is_within_tolerance(
  const config_value_t        expected_val,
  const config_value_t        tolerance,
  const Meas<config_value_t>& meas) const {

  return std::abs(meas.m_value - expected_val) <= tolerance;
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::throw_if_data_stream_disabled_for_write(
    const data_stream_ID_t& ds_ID,
    const char* config_description) const {

  if(not get_data_stream_is_enabled(ds_ID)) {
    std::ostringstream oss;
    oss << "requested write of " << config_description << " for disabled ";
    oss << ds_ID << " data stream";
    throw oss.str();
  }
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::throw_if_data_stream_disabled_for_read(
    const data_stream_ID_t& ds_ID,
    const char* config_description) const {

  if(not get_data_stream_is_enabled(ds_ID)) {
    std::ostringstream oss;
    oss << "requested read of " << config_description << " for disabled ";
    oss << ds_ID << " data stream";
    throw oss.str();
  }
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_info_config_lock(const bool did_lock, const bool is,
    const data_stream_ID_t& ds_ID,
    const config_value_t val, const config_value_t tol,
    const config_key_t cfg_key, const char* unit,
    const Meas<config_value_t>* meas) const {
  std::ostringstream oss;
  oss << "lock " << ((did_lock and is) ? "SUCCEEDED " : "FAILED ");
  oss << "for data stream: " << ds_ID << " for config: " << cfg_key << " ";
  oss << "for requested value: " << val << " ";
  oss << "w/ tolerance: +/- " << tol;
  if(did_lock) {
    if(not is) {
      oss << ", on-hardware value outside of requested tolerance,";
    }
    if(meas != 0) {
      oss <<" (actual on-hardware value: " << *meas << ")";
    }
  }
  else {
    oss << ", requested value was outside of the ";
    auto x = m_configurator.get_ranges_possible(ds_ID, cfg_key);
    if(x.size() > 1) {
      oss << "set of ranges of current possible values: " << x;
    }
    else {
      oss << "range of current possible values: " << x;
    }
  }
  log_info(oss.str().c_str());
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_info(const char* msg, ...) const {
  va_list arg;
  va_start(arg, msg);
  std::ostringstream oss;
  oss << this->m_descriptor << ": " << msg;
  LogForwarder<LC>::log_info(oss.str().c_str(), arg);
  va_end(arg);
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_debug(const char* msg, ...) const {
  va_list arg;
  va_start(arg, msg);
  std::ostringstream oss;
  oss << this->m_descriptor << ": " << msg;
  LogForwarder<LC>::log_debug(oss.str().c_str(), arg);
  va_end(arg);
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_trace(const char* msg, ...) const {
  va_list arg;
  va_start(arg, msg);
  std::ostringstream oss;
  oss << this->m_descriptor << ": " << msg;
  LogForwarder<LC>::log_trace(oss.str().c_str(), arg);
  va_end(arg);
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_warn(const char* msg, ...) const {
  va_list arg;
  va_start(arg, msg);
  std::ostringstream oss;
  oss << this->m_descriptor << ": " << msg;
  LogForwarder<LC>::log_warn(oss.str().c_str(), arg);
  va_end(arg);
}

template<class LC, class C>
void AnaRadioCtrlr<LC, C>::log_error(const char* msg, ...) const {
  va_list arg;
  va_start(arg, msg);
  std::ostringstream oss;
  oss << this->m_descriptor << ": " << msg;
  LogForwarder<LC>::log_error(oss.str().c_str(), arg);
  va_end(arg);
}

template<class LC, class C>
DigRadioCtrlr<LC, C>::DigRadioCtrlr(const char* descriptor,
                                C&          configurator) :
    AnaRadioCtrlr<LC, C>(descriptor, configurator) {
}

template<class LC, class C>
ConfigValueRanges
DigRadioCtrlr<LC, C>::get_ranges_possible_sampling_rate_Msps(
    const data_stream_ID_t data_stream_ID) const {

  auto key = config_key_sampling_rate_Msps;
  return this->m_configurator.get_ranges_possible(data_stream_ID, key);
}

template<class LC, class C>
std::vector<bool>
DigRadioCtrlr<LC, C>::get_ranges_possible_samples_are_complex(
    const data_stream_ID_t data_stream_ID) const {

  std::vector<bool> ret;
  auto key = config_key_samples_are_complex;
  auto vr = this->m_configurator.get_ranges_possible(data_stream_ID, key);

  if(vr.is_valid(0)) {
    ret.push_back(false);
  }
  if(vr.is_valid(1)) {
    ret.push_back(true);
  }
  return ret;
}

template<class LC, class C>
bool DigRadioCtrlr<LC, C>::request_config_lock(
    const config_lock_ID_t   config_lock_ID,
    const ConfigLockRequest& config_lock_request) {

  ConfigLock config_lock;
  config_lock.m_config_lock_ID = config_lock_ID;

  bool configurator_config_lock_request_was_successful = false;

  auto it = config_lock_request.m_data_streams.begin(); 
  for(; it != config_lock_request.m_data_streams.end(); it++) {

    throw_if_data_stream_lock_request_malformed(*it);

    std::vector<data_stream_ID_t> data_streams;

    if(it->get_including_data_stream_type()) {

      if(it->get_including_data_stream_ID()) {
        data_streams.push_back(it->get_data_stream_ID());
      }
      else {
        log_info("for requested config lock ID: %s, requesting data_stream_type: %s", config_lock_ID.c_str(), (it->get_data_stream_type() == data_stream_type_t::RX ? "RX" : "TX"));
        this->m_configurator.find_data_streams_of_type(it->get_data_stream_type(), data_streams);

        if(data_streams.empty()) {
          // configurator did not have any data streams of the requested data
          // stream type
          return false;
        }
      }
    }
    else {
      data_streams.push_back(it->get_data_stream_ID());
    }

    log_info_ds_cfg_lock_req_vals(*it, config_lock_ID);

    bool found_lock = false;
    auto it_found_streams = data_streams.begin();
    for(; it_found_streams != data_streams.end(); it_found_streams++) {

      if(not this->get_data_stream_is_enabled(*it_found_streams)) {
        if(it->get_including_data_stream_ID()) {
          std::ostringstream oss;
          oss << "requested config lock specifically for data stream ID ";
          oss << it_found_streams->c_str() << ", which is not currently ";
          oss << "enabled";
          throw oss.str();
        }
        continue;
      }

      log_debug("for config lock ID %s, attempting to config lock data stream %s", config_lock_ID.c_str(), it_found_streams->c_str());
      found_lock |= do_min_data_stream_config_locks(*it_found_streams, *it);

      const char* ds = it_found_streams->c_str();
      if(found_lock) {
        log_info("data stream %s met data stream config lock request requirements\n", ds);
        DataStreamConfigLock data_stream_config_lock;

        data_stream_config_lock.m_data_stream_ID      = ds;
        data_stream_config_lock.m_tuning_freq_MHz     = it->get_tuning_freq_MHz();
        data_stream_config_lock.m_bandwidth_3dB_MHz   = it->get_bandwidth_3dB_MHz();
        data_stream_config_lock.m_sampling_rate_Msps  = it->get_sampling_rate_Msps();
        data_stream_config_lock.m_samples_are_complex = it->get_samples_are_complex();
        if(it->get_including_gain_mode()) {
          data_stream_config_lock.m_gain_mode         = it->get_gain_mode();
          data_stream_config_lock.m_including_gain_mode = true;
        }
        else {
          data_stream_config_lock.m_including_gain_mode = false;
        }
        if(it->get_including_gain_dB()) {
          data_stream_config_lock.m_gain_dB           = it->get_gain_dB();
          data_stream_config_lock.m_including_gain_dB = true;
        }
        else {
          data_stream_config_lock.m_including_gain_dB = false;
        }

        config_lock.m_data_streams.push_back(data_stream_config_lock);
        break;
      }
      log_info("data stream %s did not meet data stream config lock request requirements\n", ds);
    }
    if(not found_lock) {
      configurator_config_lock_request_was_successful = false;
      break;
    }
    configurator_config_lock_request_was_successful = true;
  }

  if(configurator_config_lock_request_was_successful) {
    m_config_locks.push_back(config_lock);
    log_info("requested config lock %s succeeded", config_lock_ID.c_str());
    
    return true;
  }

  return false;
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::unlock_config_lock(
    const config_lock_ID_t config_lock_ID) {

  bool found_config_lock = false;
  for(auto itcl = m_config_locks.begin(); itcl != m_config_locks.end(); itcl++) {
    if(itcl->m_config_lock_ID.compare(config_lock_ID) == 0) {
      found_config_lock = true;
      auto itds = itcl->m_data_streams.begin();
      for(; itds != itcl->m_data_streams.end(); itds++) {
        this->unlock_tuning_freq_MHz  (itds->m_data_stream_ID);
        this->unlock_bandwidth_3dB_MHz(itds->m_data_stream_ID);
        unlock_sampling_rate_Msps     (itds->m_data_stream_ID);
        unlock_samples_are_complex    (itds->m_data_stream_ID);
        if(itds->m_including_gain_mode) {
          unlock_gain_mode            (itds->m_data_stream_ID);
        }
        if(itds->m_including_gain_dB) {
          unlock_gain_dB              (itds->m_data_stream_ID);
        }
      }
      break;
    }
  }
  if(not found_config_lock) {
    std::ostringstream oss;
    oss << "for config unlock request, config lock ID " << config_lock_ID;
    oss << " not found";
    throw oss.str();
  }
}

template<class LC, class C>
bool DigRadioCtrlr<LC, C>::lock_sampling_rate_Msps(
    const data_stream_ID_t ds_ID,
    const config_value_t   val,
    const config_value_t   tol) {

  // the configurator, which is a software emulation of hardware capabilities,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = config_key_sampling_rate_Msps;
  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, val, tol);

  bool is = false; // is within tolerance
  if(did_lock) {
    config_value_t cfglval; // configurator locked value

    /// @todo / FIXME - implement m_configurator.get_locked_value()
    cfglval = this->m_configurator.get_config_min_valid_value(ds_ID, cfg_key);

    Meas<config_value_t> meas = set_sampling_rate_Msps(ds_ID, cfglval);

    is = config_val_is_within_tolerance(val, tol, meas);

    this->log_info_config_lock(did_lock, is,ds_ID,val,tol,cfg_key,"Msps",&meas);
  }
  else {
    this->log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "Msps");
  }

  return did_lock and is;
}

template<class LC, class C>
bool DigRadioCtrlr<LC, C>::lock_samples_are_complex(
    const data_stream_ID_t  ds_ID,
    const bool              val) {

  // the configurator, which is a software emulation of hardware capabilties,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = config_key_samples_are_complex;

  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, val);

  if(did_lock) {
    config_value_t cfglval; // configurator locked value

    /// @todo / FIXME - implement m_configurator.get_locked_value()
    cfglval = this->m_configurator.get_config_min_valid_value(ds_ID, cfg_key);

    bool are = set_samples_are_complex(ds_ID, cfglval);
    if(are != val) { // exact for samples_are_complex
      did_lock = false;
    }
    log_info_config_lock(did_lock, ds_ID, val, cfg_key, &are);
  }
  else {
    log_info_config_lock(did_lock, ds_ID, val, cfg_key);
  }

  return did_lock;
}

template<class LC, class C>
bool DigRadioCtrlr<LC, C>::lock_gain_mode(
    const data_stream_ID_t  ds_ID,
    const gain_mode_value_t val) {

  // the configurator, which is a software emulation of hardware capabilties,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = config_key_gain_mode;
  config_value_t v = val.compare("manual") == 0 ? 1 : 0; // auto=0, manual=1
  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, v);

  if(did_lock) {
    Meas<gain_mode_value_t> meas = set_gain_mode(ds_ID, val);

    if(meas.m_value != val) { // exact for gain mode
      did_lock = false;
    }
    log_info_config_lock(did_lock, ds_ID, val, cfg_key, &meas);
  }
  else {
    log_info_config_lock(did_lock, ds_ID, val, cfg_key);
  }

  return did_lock;
}

template<class LC, class C>
bool DigRadioCtrlr<LC, C>::lock_gain_dB(
    const data_stream_ID_t ds_ID,
    const config_value_t   val,
    const config_value_t   tol) {

  // the configurator, which is a software emulation of hardware capabilties,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = config_key_gain_dB;
  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, val, tol);

  bool is = false; // is within tolerance
  if(did_lock) {
    config_value_t cfglval; // configurator locked value

    /// @todo / FIXME - implement m_configurator.get_locked_value()
    cfglval = this->m_configurator.get_config_min_valid_value(ds_ID, cfg_key);

    Meas<config_value_t> meas = set_gain_dB(ds_ID, cfglval);

    is = config_val_is_within_tolerance(val, tol, meas);

    this->log_info_config_lock(did_lock, is, ds_ID, val,tol,cfg_key,"dB",&meas);
  }
  else {
    this->log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "dB");
  }

  return did_lock and is;
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::unlock_sampling_rate_Msps(
    const data_stream_ID_t data_stream_ID) {

  this->unlock_config(data_stream_ID, config_key_sampling_rate_Msps);
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::unlock_samples_are_complex(
    const data_stream_ID_t data_stream_ID) {

  this->unlock_config(data_stream_ID, config_key_samples_are_complex);
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::unlock_gain_mode(
    const data_stream_ID_t data_stream_ID) {

  this->unlock_config(data_stream_ID, config_key_gain_mode);
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::unlock_gain_dB(
    const data_stream_ID_t data_stream_ID) {

  this->unlock_config(data_stream_ID, config_key_gain_dB);
}

template<class LC, class C>
bool DigRadioCtrlr<LC, C>::do_min_data_stream_config_locks(
    const data_stream_ID_t             data_stream_ID,
    const DataStreamConfigLockRequest& data_stream_config_lock_request) {

  const DataStreamConfigLockRequest& req = data_stream_config_lock_request;

  throw_if_data_stream_lock_request_malformed(req);

  // first perform all the analog-specific config locks
  {
    const data_stream_ID_t& ds_ID = data_stream_ID;
    if(not AnaRadioCtrlr<LC, C>::do_min_data_stream_config_locks(ds_ID, req)) {
      return false;
    }
  }

  // second perform all the digital-specific config locks
  config_value_t val, tol;

  val = req.get_sampling_rate_Msps();
  tol = req.get_tolerance_sampling_rate_Msps();
  if(not lock_sampling_rate_Msps(data_stream_ID, val, tol)) {
    goto unrollandfail;
  }

  val = req.get_samples_are_complex();
  if(not lock_samples_are_complex(data_stream_ID, val)) {
    goto unrollandfail;
  }

  if(req.get_including_gain_mode()) {
    if(not lock_gain_mode(data_stream_ID, req.get_gain_mode())) {
      goto unrollandfail;
    }
  }
  if(req.get_including_gain_dB()) {
    val = req.get_gain_dB();
    tol = req.get_tolerance_gain_dB();

    if(not lock_gain_dB(data_stream_ID, val, tol)) {
      goto unrollandfail;
    }
  }

  return true;

  unrollandfail:
  this->unlock_tuning_freq_MHz  (data_stream_ID);
  this->unlock_bandwidth_3dB_MHz(data_stream_ID);
  unlock_sampling_rate_Msps     (data_stream_ID);
  unlock_samples_are_complex    (data_stream_ID);
  unlock_gain_mode              (data_stream_ID);
  unlock_gain_dB                (data_stream_ID);
  return false;
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::log_info_config_lock(const bool did_lock,
    const data_stream_ID_t& ds_ID, const config_value_t val,
    const config_key_t cfg_key, const bool* meas) const {
  std::ostringstream oss;
  oss << "lock " << (did_lock ? "SUCCEEDED " : "FAILED ");
  oss << "for data stream: " << ds_ID << " for config: " << cfg_key << " ";
  oss << "for requested value: " << (val ? "true" : "false");
  if(did_lock) {
    if(meas != 0) {
      oss <<" (actual on-hardware value: " << (*meas ? "true" : "false") << ")";
    }
  }
  else {
    oss << ", requested value was outside of the ";
    auto x = this->m_configurator.get_ranges_possible(ds_ID, cfg_key);
    if(x.size() > 1) {
      oss << "set of ranges of current possible values: " << x;
    }
    else {
      oss << "range of current possible values: " << x;
    }
  }
  log_info(oss.str().c_str());
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::log_info_config_lock(const bool did_lock,
    const data_stream_ID_t& ds_ID, const gain_mode_value_t val,
    const config_key_t cfg_key, const Meas<gain_mode_value_t>* meas) const {
  std::ostringstream oss;
  oss << "lock " << (did_lock ? "SUCCEEDED " : "FAILED ");
  oss << "for data stream: " << ds_ID << " for config: " << cfg_key << " ";
  oss << "for requested value: " << val;
  if(did_lock) {
    if(meas != 0) {
      oss <<" (actual on-hardware value: " << *meas << ")";
    }
  }
  else {
    oss << ", requested value was outside of the ";
    auto x = this->m_configurator.get_ranges_possible(ds_ID, cfg_key);
    if(x.is_valid(0)) { // auto
      if(x.is_valid(1)) { // manual
        oss << "set of ranges of current possible values: {[auto],[manual]}";
      }
      else {
        oss << "range of current possible values: {[auto]}";
      }
    }
    else {
      if(x.is_valid(1)) { // manual
        oss << "range of current possible values: {[manual]}";
      }
      else {
        oss << "range of current possible values: {}";
      }
    }
  }
  log_info(oss.str().c_str());
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::log_info_ds_cfg_lock_req_vals(
    const DataStreamConfigLockRequest& dsreq,
    const config_lock_ID_t& config_lock_ID) const{

  std::ostringstream oss;
  oss << "for requested config lock ID: " << config_lock_ID << ", requesting ";

  if(dsreq.get_including_data_stream_ID()) {
    auto val = dsreq.get_data_stream_ID().c_str();
    log_info("%sdata_stream_ID: %s", oss.str().c_str(), val);
  }
  if(dsreq.get_including_tuning_freq_MHz()) {
    auto val = dsreq.get_tuning_freq_MHz();
    log_info("%stuning_freq_MHz: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_bandwidth_3dB_MHz()) {
    auto val = dsreq.get_bandwidth_3dB_MHz();
    log_info("%sbandwidth_3dB_MHz: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_sampling_rate_Msps()) {
    auto val = dsreq.get_sampling_rate_Msps();
    log_info("%ssampling_rate_Msps: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_samples_are_complex()) {
    auto val = dsreq.get_samples_are_complex() ? "true" : "false";
    log_info("%ssamples_are_complex: %s", oss.str().c_str(), val);
  }
  if(dsreq.get_including_gain_mode()) {
    auto val = dsreq.get_gain_mode().c_str();
    log_info("%sgain_mode: %s", oss.str().c_str(), val);
  }
  if(dsreq.get_including_gain_dB()) {
    auto val = dsreq.get_gain_dB();
    log_info("%sgain_dB: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_tuning_freq_MHz()) {
    auto val = dsreq.get_tolerance_tuning_freq_MHz();
    log_info("%stolerance_tuning_freq_MHz: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_bandwidth_3dB_MHz()) {
    auto val = dsreq.get_tolerance_bandwidth_3dB_MHz();
    log_info("%stolerance_bandwidth_3dB_MHz: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_sampling_rate_Msps()) {
    auto val = dsreq.get_tolerance_sampling_rate_Msps();
    log_info("%stolerance_sampling_rate_Msps: %0.15f", oss.str().c_str(), val);
  }
  if(dsreq.get_including_gain_dB()) {
    auto val = dsreq.get_tolerance_gain_dB();
    log_info("%stolerance_gain_dB: %0.15f", oss.str().c_str(), val);
  }
}

template<class LC, class C>
void DigRadioCtrlr<LC, C>::throw_if_data_stream_lock_request_malformed(
    const DataStreamConfigLockRequest& data_stream_config_lock_request) const {

  const DataStreamConfigLockRequest& req = data_stream_config_lock_request;

  // first check all the analog-specific config locks
  AnaRadioCtrlr<LC, C>::throw_if_data_stream_lock_request_malformed(req);

  // second check all the digital-specific config locks
  if(not req.get_including_sampling_rate_Msps()) {
    throw std::string("digital radio controller's data stream config lock request malformed: did not include sampling_rate_Msps");
  }
  if(not req.get_including_samples_are_complex()) {
    throw std::string("digital radio controller's data stream config lock request malformed: did not include samples_are_complex");
  }
  if(req.get_including_gain_dB()) {
    if(not req.get_including_gain_mode()) {
      throw std::string("radio controller's data stream config lock request malformed: included gain_dB in the request (which implies manual gain), but did not include gain_mode with a value of manual, which is required when including gain_dB");
    }
    else if(req.get_gain_mode().compare("auto") == 0) {
      throw std::string("radio controller's data stream config lock request malformed: included gain_dB in the request (which implies manual gain) along with a gain_mode of auto, which is an invalid combination");
    }
  }
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
