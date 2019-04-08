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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Mar 14 15:38:04 2018 EDT
 * BASED ON THE FILE: dig_radio_ctrlr_fmcomms_2_3.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the dig_radio_ctrlr_fmcomms_2_3 worker in C++
 */

#include <algorithm> // std::min()
#include <sstream> // std::ostringstream
#include <cinttypes> // PRIi32
#include "dig_radio_ctrlr_fmcomms_2_3-worker.hh"
#include "this_opencpi_worker.h" // THIS_OPENCPI_WORKER_STRING

#include "RadioCtrlrFMCOMMS2TuneResamp.hh"
#include "RadioCtrlrFMCOMMS3TuneResamp.hh"

#define COMP THIS_OPENCPI_COMPONENT_STRING

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Dig_radio_ctrlr_fmcomms_2_3WorkerTypes;

using namespace OCPIProjects;
using namespace OCPIProjects::RadioCtrlr;

/// @todo / FIXME - add functionality back once RadioCtrlrFMCOMMS* classes correctly read back valid values
#define IS_BROKEN

class Dig_radio_ctrlr_fmcomms_2_3Worker : public Dig_radio_ctrlr_fmcomms_2_3WorkerBase {

  struct ad9361_rf_phy* m_ad9361_rf_phy; //No-OS struct

#if OCPI_PARAM_dig_radio_ctrlr_fmcomms_2_3_FMCOMMS_NUM_p() == 2
  RadioCtrlrFMCOMMS2TuneResamp<Slave1> m_ctrlr;
#endif
#if OCPI_PARAM_dig_radio_ctrlr_fmcomms_2_3_FMCOMMS_NUM_p() == 3
  RadioCtrlrFMCOMMS3TuneResamp<Slave1> m_ctrlr;
#endif

  RunCondition m_aRunCondition;
public:
  Dig_radio_ctrlr_fmcomms_2_3Worker() :
      m_ctrlr(COMP, slave, getApplication(), m_ad9361_rf_phy),
      m_aRunCondition(RCC_NO_PORTS) {

    m_ad9361_rf_phy = 0;

    //Run function should never be called
    setRunCondition(&m_aRunCondition);
  }
  ~Dig_radio_ctrlr_fmcomms_2_3Worker() {
    ad9361_free(m_ad9361_rf_phy); // this function added in ad9361.patch
  }

private:

  RCCResult validate_routing_ID(std::string routing_ID) {

    if(routing_ID.compare("RX0") == 0) {
      return RCC_OK;
    }
    else if(routing_ID.compare("RX1") == 0) {
      return RCC_OK;
    }
    else if(routing_ID.compare("TX0") == 0) {
      return RCC_OK;
    }
    else if(routing_ID.compare("TX1") == 0) {
      return RCC_OK;
    }
    std::ostringstream oss;
    oss << "config lock request contained bad routing ID of: \"" << routing_ID;
    oss  << "\", expected values for FMCOMMS2/3 are RX0, RX1, TX0, or TX1\n";
    return setError(oss.str().c_str());
  }

  uint32_t get_num_streams() {
    uint32_t num_streams; // adding two uint16_t to get a uint32_t
    num_streams = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P;
    num_streams += DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P;
    return num_streams;
  }

  template<typename R>
  void get_data_stream_ID_and_or_dir_for_valid_values_idx(R dir_rx,
      R dir_tx, unsigned idx, std::string* ID, R* dir = 0) {

    // SMA RX1A (can be configured for only the rx direction)
    // SMA RX2A (can be configured for only the rx direction)
    // SMA TX1A (can be configured for only the tx direction)
    // SMA TX2A (can be configured for only the tx direction)

    if(ID != 0) {
      unsigned ii_rx = 0;
      unsigned ii_tx = 0;
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_NUM_DATA_STREAM_IDS_RX_P) {
        ID->assign(DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx]);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_NUM_DATA_STREAM_IDS_TX_P) {
        ID->assign(DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx]);
        ii_tx++;
      }
    }

    if(dir != 0) {
      switch(idx) {
        case 0:
          *dir = dir_rx;
          break;
        case 1:
          *dir = dir_rx;
          break;
        case 2:
          *dir = dir_tx;
          break;
        default: // 3
          *dir = dir_tx;
          break;
      }
    }
  }

  template<typename T, typename R> void init_valid_values_read(T& this_prop,
      R trx, R ttx, unsigned num_streams) {

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;

    std::string data_stream_ID;
    R direction;

    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < num_streams; ii++) {

      std::string* ID = &data_stream_ID;
      R* dir = &direction;
      get_data_stream_ID_and_or_dir_for_valid_values_idx(trx, ttx, ii, ID, dir);

      auto& prop_entry = this_prop[ii];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_NUM_DATA_STREAM_IDS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop_entry.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_NUM_DATA_STREAM_IDS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop_entry.data_stream_ID, src, num);
        ii_tx++;
      }
    }
  }

  template<typename T>
  void populate_valid_values(T& prop_entry, const ConfigValueRanges& vr) {

    // valid_values member is a sequence of length 32
    const size_t mbr_len = 32;

    const size_t len = std::min(mbr_len, vr.size());
    prop_entry.valid_values.resize(len);
    unsigned ii=0;
    for(auto it = vr.m_ranges.begin(); it != vr.m_ranges.end(); ++it) {
      if(ii >= len) {
        std::ostringstream ostr;
        ostr << "read valid ranges that was greater than maximum member ";
        ostr << "sequence length of " << mbr_len;
        throw ostr.str();
      }
      prop_entry.valid_values.data[ii].min = it->get_min();
      prop_entry.valid_values.data[ii].max = it->get_max();
      ii++;
    }
  }

  template<typename T>
  void populate_valid_values(T& prop_entry, const std::vector<bool>& vr) {
    // valid_values member is a sequence of length 2
    const size_t mbr_len = 2;

    const size_t len = std::min(mbr_len, vr.size());
    prop_entry.valid_values.resize(len);
    unsigned ii=0;
    for(auto it = vr.begin(); it != vr.end(); ++it) {
      if(ii >= len) {
        std::ostringstream ostr;
        ostr << "read valid ranges that was greater than maximum member ";
        ostr << "sequence length of " << mbr_len;
        throw ostr.str();
      }
      prop_entry.valid_values.data[ii] = *it;
      ii++;
    }
  }

  template<typename T>
  void populate_valid_values(T& prop_entry,
      const std::vector<gain_mode_value_t>& vr) {

    // valid_values member is a sequence of length 32
    const size_t mbr_len = 32;

    const size_t len = std::min(mbr_len, vr.size());
    prop_entry.valid_values.resize(len);
    unsigned ii=0;
    for(auto it = vr.begin(); it != vr.end(); ++it) {
      if(ii >= len) {
        std::ostringstream ostr;
        ostr << "read valid ranges that was greater than maximum member ";
        ostr << "sequence length of " << mbr_len;
        throw ostr.str();
      }
      size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
      strncpy(prop_entry.valid_values.data[ii], it->c_str(), num);
      ii++;
    }
  }

  bool get_data_stream_is_enabled(std::string& data_stream_ID) {
    return m_ctrlr.get_data_stream_is_enabled(data_stream_ID);
  }

  // notification that request_config_lock property has been written
  RCCResult request_config_lock_written() {

    ConfigLockRequest config_lock_request;

    const Request_config_lock& prop = m_properties.request_config_lock;

    for(size_t ii=0; ii<prop.data_streams.size(); ii++) {

      const Request_config_lockData_streams& data_stream = prop.data_streams.data[ii];

      DataStreamConfigLockRequest data_stream_config_lock_request;
      DataStreamConfigLockRequest& dsreq = data_stream_config_lock_request;

      if(data_stream.direction == REQUEST_CONFIG_LOCKDATA_STREAMS_DIRECTION_RX) {
        dsreq.include_data_stream_type(data_stream_type_t::RX); /// @todo / FIXME change 'type' to direction
        std::string ID(data_stream.data_stream_ID);
        if(not ID.empty()) {
          dsreq.include_data_stream_ID(data_stream.data_stream_ID);
        }
      }
      else if(data_stream.direction == REQUEST_CONFIG_LOCKDATA_STREAMS_DIRECTION_TX) {
        dsreq.include_data_stream_type(data_stream_type_t::TX); /// @todo / FIXME change 'type' to direction
        std::string ID(data_stream.data_stream_ID);
        if(not ID.empty()) {
          dsreq.include_data_stream_ID(data_stream.data_stream_ID);
        }
      }
      else { // == REQUEST_CONFIG_LOCKDATA_STREAMS_DATA_STREAM_TYPE_NULL
        dsreq.include_data_stream_ID(data_stream.data_stream_ID);
      }
      RCCResult ret = validate_routing_ID(data_stream.routing_ID);
      if(ret != RCC_OK) {
        return ret;
      }
      dsreq.include_routing_ID(data_stream.routing_ID);
      {
        const double& val = data_stream.tuning_freq_MHz;
        const double& tol = data_stream.tolerance_tuning_freq_MHz;
        dsreq.include_tuning_freq_MHz(val, tol);
      }
      {
        const double& val = data_stream.bandwidth_3dB_MHz;
        const double& tol = data_stream.tolerance_bandwidth_3dB_MHz;
        dsreq.include_bandwidth_3dB_MHz(val, tol);
      }
      {
        const double& val = data_stream.sampling_rate_Msps;
        const double& tol = data_stream.tolerance_sampling_rate_Msps;
        dsreq.include_sampling_rate_Msps(val, tol);
      }
      {
        const double& val = data_stream.samples_are_complex;
        dsreq.include_samples_are_complex(val);
      }
      {
        /// @todo / FIXME - support gain_mode of null, which would bypass next line
        dsreq.include_gain_mode(data_stream.gain_mode);
      }
      const std::string manual_str("manual");
      if(manual_str.compare(data_stream.gain_mode) == 0) {

        const double& val = data_stream.gain_dB;
        const double& tol = data_stream.tolerance_gain_dB;
        dsreq.include_gain_dB(val, tol);
      }
      config_lock_request.m_data_streams.push_back(data_stream_config_lock_request);
    }

    // request the config lock
    bool success;
    try {
      auto& clreq = config_lock_request;
      success = m_ctrlr.request_config_lock(prop.config_lock_ID, clreq);
    }
    catch(const char* err) {
      return setError(err);
    }
    if(not success) {
      return setError("config lock request was unsuccessful, set OCPI_LOG_LEVEL to 8 (or higher) for more info");
    }

    /*ad9361_set_rx_quad_track_en_dis(m_ad9361_rf_phy, 0);
    slave.set_rx_pgo_force_bits(0x02);
    slave.set_rx_pgo_gain_corr_rx2_ina(0);*/

    return RCC_OK;
  }
  // notification that config_locks property will be read
  RCCResult config_locks_read() {
    m_properties.config_locks.resize(0);

    size_t ii_max;
    ii_max = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P;
    ii_max += DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P;

    auto config_locks = m_ctrlr.get_config_locks();

    ii_max = std::min(ii_max, config_locks.size());

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    for(unsigned ii=0; ii < ii_max; ii++) {

      m_properties.config_locks.resize(m_properties.config_locks.length+1);

      auto& prop = m_properties.config_locks.data[ii];

      auto it = config_locks[ii].m_data_streams.begin();
      if(it != config_locks[ii].m_data_streams.end()) {
        strncpy(prop.config_lock_ID, config_locks[ii].m_config_lock_ID.c_str(), num);
      }

      unsigned n = 0;
      prop.data_streams.resize(0);
      for(; it != config_locks[ii].m_data_streams.end(); it++) {

        if(it->m_data_stream_ID.compare("SMA_RX1A") == 0) {
          prop.data_streams.resize(n+1);
          const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[0];
          strncpy(prop.data_streams.data[n].data_stream_ID, src, num);
        }
        else if(it->m_data_stream_ID.compare("SMA_RX2A") == 0) {
          prop.data_streams.resize(n+1);
          const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[1];
          strncpy(prop.data_streams.data[n].data_stream_ID, src, num);
        }
        else if(it->m_data_stream_ID.compare("SMA_TX1A") == 0) {
          prop.data_streams.resize(n+1);
          const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[0];
          strncpy(prop.data_streams.data[n].data_stream_ID, src, num);
        }
        else if(it->m_data_stream_ID.compare("SMA_TX2A") == 0) {
          prop.data_streams.resize(n+1);
          const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[1];
          strncpy(prop.data_streams.data[n].data_stream_ID, src, num);
        }
        else {
          return setError("Unknown ctrlr config lock data stream ID read: %s", it->m_data_stream_ID.c_str());
        }
        /// @todo / FIXME - read routing ID from *it ?
        prop.data_streams.data[n].tuning_freq_MHz     = it->m_tuning_freq_MHz;
        prop.data_streams.data[n].bandwidth_3dB_MHz   = it->m_bandwidth_3dB_MHz;
        prop.data_streams.data[n].sampling_rate_Msps  = it->m_sampling_rate_Msps;
        prop.data_streams.data[n].samples_are_complex = it->m_samples_are_complex;
        prop.data_streams.data[n].gain_dB             = it->m_gain_dB;
        if(it->m_including_gain_mode) {
          strncpy(prop.data_streams.data[n].gain_mode_lock, it->m_gain_mode.c_str(), num);
        }
        else {
          prop.data_streams.data[n].gain_mode_lock[0] = '\0';
        }
        prop.data_streams.data[n].gain_dB             = it->m_gain_dB;
        n++;
      }
    }

    return RCC_OK;
  }
  // notification that unlock_config_lock property has been written
  RCCResult unlock_config_lock_written() {
    m_ctrlr.unlock_config_lock(m_properties.unlock_config_lock.config_lock_ID);
    return RCC_OK;
  }
  // notification that unlock_all property has been written
  RCCResult unlock_all_written() {
    m_ctrlr.unlock_all();
    return RCC_OK;
  }
  // notification that data_stream_is_enabled property will be read
  RCCResult data_stream_is_enabled_read() {

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    unsigned ii_max = DIG_RADIO_CTRLR_FMCOMMS_2_3_NUM_DATA_STREAM_IDS_P;
    for(unsigned ii=0; ii < ii_max; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      auto& prop = m_properties.data_stream_is_enabled[ii];

      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        prop.data_stream_is_enabled = get_data_stream_is_enabled(data_stream_ID);
      }
      catch(const char* err) {
        return setError(err);
      }
    }

    return RCC_OK;
  }
  // notification that direction_readabck property will be read
  RCCResult direction_readback_read() {

    m_properties.direction_readback.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.direction_readback.resize(m_properties.direction_readback.length+1);

      auto& prop = m_properties.direction_readback.data[m_properties.direction_readback.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      switch(ii) {
        case 0:
          prop.direction_val = DIRECTION_READBACK_DIRECTION_VAL_RX;
          break;
        case 1:
          prop.direction_val = DIRECTION_READBACK_DIRECTION_VAL_RX;
          break;
        case 2:
          prop.direction_val = DIRECTION_READBACK_DIRECTION_VAL_TX;
          break;
        default: // 3
          prop.direction_val = DIRECTION_READBACK_DIRECTION_VAL_TX;
          break;
      }
    }

    return RCC_OK;
  }
  // notification that tuning_freq_MHz property will be read
  RCCResult tuning_freq_MHz_read() {

    m_properties.tuning_freq_MHz.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.tuning_freq_MHz.resize(m_properties.tuning_freq_MHz.length+1);

      auto& prop = m_properties.tuning_freq_MHz.data[m_properties.tuning_freq_MHz.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        Meas<config_value_t> meas = m_ctrlr.get_tuning_freq_MHz(data_stream_ID);
        prop.tuning_freq_MHz = meas.m_value;
      }
      catch(const char* err) {
        return setError(err);
      }
    }

    return RCC_OK;
  }
  // notification that bandwidth_3dB_MHz property will be read
  RCCResult bandwidth_3dB_MHz_read() {

    m_properties.bandwidth_3dB_MHz.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.bandwidth_3dB_MHz.resize(m_properties.bandwidth_3dB_MHz.length+1);

      auto& prop = m_properties.bandwidth_3dB_MHz.data[m_properties.bandwidth_3dB_MHz.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        Meas<config_value_t> meas = m_ctrlr.get_bandwidth_3dB_MHz(data_stream_ID);
        prop.bandwidth_3dB_MHz = meas.m_value;
      }
      catch(const char* err) {
        return setError(err);
      }
    }

    return RCC_OK;
  }
  // notification that sampling_rate_Msps property will be read
  RCCResult sampling_rate_Msps_read() {

    m_properties.sampling_rate_Msps.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.sampling_rate_Msps.resize(m_properties.sampling_rate_Msps.length+1);

      auto& prop = m_properties.sampling_rate_Msps.data[m_properties.sampling_rate_Msps.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        Meas<config_value_t> meas = m_ctrlr.get_sampling_rate_Msps(data_stream_ID);
        prop.sampling_rate_Msps = meas.m_value;
      }
      catch(const char* err) {
        return setError(err);
      }
    }

    return RCC_OK;
  }
  // notification that samples_are_complex property will be read
  RCCResult samples_are_complex_read() {

    m_properties.samples_are_complex.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.samples_are_complex.resize(m_properties.samples_are_complex.length+1);

      auto& prop = m_properties.samples_are_complex.data[m_properties.samples_are_complex.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        bool b = m_ctrlr.get_samples_are_complex(data_stream_ID);
        prop.samples_are_complex = b;
      }
      catch(const char* err) {
        return setError(err);
      }
    }

    return RCC_OK;
  }
  // notification that valid_values_tuning_freq_MHz property will be read
  RCCResult valid_values_tuning_freq_MHz_read() {

    auto& this_prop = m_properties.valid_values_tuning_freq_MHz;
    const unsigned num_streams = get_num_streams();
    Direction_tuning drx, dtx;
    drx = VALID_VALUES_TUNING_FREQ_MHZ_DIRECTION_TUNING_RX;
    dtx = VALID_VALUES_TUNING_FREQ_MHZ_DIRECTION_TUNING_TX;

    init_valid_values_read(this_prop, drx, dtx, num_streams);

    for(unsigned ii = 0; ii < num_streams; ii++) {

      Direction_tuning dd;
#ifdef IS_BROKEN
      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, 0, &dd);
      this_prop[ii].direction_tuning = dd;

      this_prop[ii].valid_values.resize(0);
#else
      std::string ID;

      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, &ID, 0);
      this_prop[ii].direction_tuning = dd;

      ConfigValueRanges vr = m_ctrlr.get_ranges_possible_tuning_freq_MHz(ID);
      try {
        populate_valid_values(this_prop[ii], vr);
      }
      catch(std::string& err) {
        return setError("%s", err.c_str());
      }
#endif
    }

    return RCC_OK;
  }
  // notification that valid_values_bandwidth_3dB_MHz property will be read
  RCCResult valid_values_bandwidth_3dB_MHz_read() {

    auto& this_prop = m_properties.valid_values_bandwidth_3dB_MHz;
    const unsigned num_streams = get_num_streams();
    Direction_bandwidth drx, dtx;
    drx = VALID_VALUES_BANDWIDTH_3DB_MHZ_DIRECTION_BANDWIDTH_RX;
    dtx = VALID_VALUES_BANDWIDTH_3DB_MHZ_DIRECTION_BANDWIDTH_TX;

    init_valid_values_read(this_prop, drx, dtx, num_streams);

    std::string data_stream_ID;

    for(unsigned ii = 0; ii < num_streams; ii++) {

      Direction_bandwidth dd;
#ifdef IS_BROKEN
      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, 0, &dd);
      this_prop[ii].direction_bandwidth = dd;

      this_prop[ii].valid_values.resize(0);
#else
      std::string ID;

      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, &ID, &dd);
      this_prop[ii].direction_bandwidth = dd;

      ConfigValueRanges vr = m_ctrlr.get_ranges_possible_bandwidth_3dB_MHz(ID);
      try {
        populate_valid_values(this_prop[ii], vr);
      }
      catch(std::string& err) {
        return setError("%s", err.c_str());
      }
#endif
    }

    return RCC_OK;
  }
  // notification that valid_values_sampling_rate_Msps property will be read
  RCCResult valid_values_sampling_rate_Msps_read() {

    auto& this_prop = m_properties.valid_values_sampling_rate_Msps;
    const unsigned num_streams = get_num_streams();
    Direction_sampling drx, dtx;
    drx = VALID_VALUES_SAMPLING_RATE_MSPS_DIRECTION_SAMPLING_RX;
    dtx = VALID_VALUES_SAMPLING_RATE_MSPS_DIRECTION_SAMPLING_TX;

    init_valid_values_read(this_prop, drx, dtx, num_streams);

    std::string data_stream_ID;

    for(unsigned ii = 0; ii < num_streams; ii++) {

      Direction_sampling dd;
#ifdef IS_BROKEN
      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, 0, &dd);
      this_prop[ii].direction_sampling = dd;

      this_prop[ii].valid_values.resize(0);
#else
      std::string ID;

      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, &ID, &dd);
      this_prop[ii].direction_sampling = dd;

      ConfigValueRanges vr = m_ctrlr.get_ranges_possible_sampling_rate_Msps(ID);
      try {
        populate_valid_values(this_prop[ii], vr);
      }
      catch(std::string& err) {
        return setError("%s", err.c_str());
      }
#endif
    }

    return RCC_OK;
  }
  // notification that valid_values_samples_are_complex property will be read
  RCCResult valid_values_samples_are_complex_read() {
    auto& this_prop = m_properties.valid_values_samples_are_complex;
    const unsigned num_streams = get_num_streams();
    Direction_samples_are drx, dtx;
    drx = VALID_VALUES_SAMPLES_ARE_COMPLEX_DIRECTION_SAMPLES_ARE_RX;
    dtx = VALID_VALUES_SAMPLES_ARE_COMPLEX_DIRECTION_SAMPLES_ARE_TX;

    init_valid_values_read(this_prop, drx, dtx, num_streams);

    std::string data_stream_ID;

    for(unsigned ii = 0; ii < num_streams; ii++) {

      Direction_samples_are dd;
      std::string ID;

      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, &ID, &dd);

      this_prop[ii].direction_samples_are = dd;
      typedef std::vector<bool> tmp;
      const tmp& vr = m_ctrlr.get_ranges_possible_samples_are_complex(ID);
      try {
        populate_valid_values(this_prop[ii], vr);
      }
      catch(std::string& err) {
        return setError("%s", err.c_str());
      }
    }

    return RCC_OK;
  }
  // notification that gain_mode property will be read
  RCCResult gain_mode_readback_read() {

    m_properties.gain_mode_readback.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.gain_mode_readback.resize(m_properties.gain_mode_readback.length+1);

      auto& prop = m_properties.gain_mode_readback.data[m_properties.gain_mode_readback.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        Meas<gain_mode_value_t> meas = m_ctrlr.get_gain_mode(data_stream_ID);
        strncpy(&prop.gain_mode_readback_val[0], meas.m_value.c_str(), num);
      }
      catch(const char* err) {
        return setError(err);
      }
    }
    return RCC_OK;
  }
  // notification that gain_dB property will be read
  RCCResult gain_dB_read() {

    m_properties.gain_dB.resize(0);

    std::string data_stream_ID;

    size_t num = DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_STRING_LENGTH_P;
    unsigned ii_rx = 0;
    unsigned ii_tx = 0;
    for(unsigned ii=0; ii < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P+DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P; ii++) {

      /// @TODO / FIXME - check that this mapping is correct
      switch(ii) {
        case 0:
          data_stream_ID.assign("SMA_RX1A");
          break;
        case 1:
          data_stream_ID.assign("SMA_RX2A");
          break;
        case 2:
          data_stream_ID.assign("SMA_TX1A");
          break;
        default: // 3
          data_stream_ID.assign("SMA_TX2A");
          break;
      }

      if(not get_data_stream_is_enabled(data_stream_ID)) {
        if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
          ii_rx++;
        }
        else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
          ii_tx++;
        }
        continue;
      }
      m_properties.gain_dB.resize(m_properties.gain_dB.length+1);

      auto& prop = m_properties.gain_dB.data[m_properties.gain_dB.length-1];
      if(ii_rx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_RX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_RX_P[ii_rx];
        strncpy(prop.data_stream_ID, src, num);
        ii_rx++;
      }
      else if(ii_tx < DIG_RADIO_CTRLR_FMCOMMS_2_3_MAX_NUM_DATA_STREAMS_TX_P) {
        const char* src = DIG_RADIO_CTRLR_FMCOMMS_2_3_DATA_STREAM_IDS_TX_P[ii_tx];
        strncpy(prop.data_stream_ID, src, num);
        ii_tx++;
      }

      try {
        Meas<config_value_t> meas = m_ctrlr.get_gain_dB(data_stream_ID);
        prop.gain_dB = meas.m_value;
      }
      catch(const char* err) {
        return setError(err);
      }
    }
    return RCC_OK;
  }
  // notification that app_inst_name_TX0_qdac property has been written
  RCCResult app_inst_name_TX0_qdac_written() {
    m_ctrlr.set_app_inst_name_TX0_qdac(m_properties.app_inst_name_TX0_qdac);
    return RCC_OK;
  }
  // notification that app_inst_name_TX0_complex_mixer property has been written
  RCCResult app_inst_name_TX0_complex_mixer_written() {
    m_ctrlr.set_app_inst_name_TX0_complex_mixer(m_properties.app_inst_name_TX0_complex_mixer);
    return RCC_OK;
  }
  // notification that app_inst_name_TX0_cic_int property has been written
  RCCResult app_inst_name_TX0_cic_int_written() {
    m_ctrlr.set_app_inst_name_TX0_cic_int(m_properties.app_inst_name_TX0_cic_int);
    return RCC_OK;
  }
  // notification that app_inst_name_TX1_qdac property has been written
  RCCResult app_inst_name_TX1_qdac_written() {
    m_ctrlr.set_app_inst_name_TX1_qdac(m_properties.app_inst_name_TX1_qdac);
    return RCC_OK;
  }
  // notification that app_inst_name_TX1_complex_mixer property has been written
  RCCResult app_inst_name_TX1_complex_mixer_written() {
    m_ctrlr.set_app_inst_name_TX1_complex_mixer(m_properties.app_inst_name_TX1_complex_mixer);
    return RCC_OK;
  }
  // notification that app_inst_name_TX1_cic_int property has been written
  RCCResult app_inst_name_TX1_cic_int_written() {
    m_ctrlr.set_app_inst_name_TX1_cic_int(m_properties.app_inst_name_TX1_cic_int);
    return RCC_OK;
  }
  // notification that app_inst_name_RX0_qadc property has been written
  RCCResult app_inst_name_RX0_qadc_written() {
    m_ctrlr.set_app_inst_name_RX0_qadc(m_properties.app_inst_name_RX0_qadc);
    return RCC_OK;
  }
  // notification that app_inst_name_RX0_complex_mixer property has been written
  RCCResult app_inst_name_RX0_complex_mixer_written() {
    m_ctrlr.set_app_inst_name_RX0_complex_mixer(m_properties.app_inst_name_RX0_complex_mixer);
    return RCC_OK;
  }
  // notification that app_inst_name_RX0_cic_dec property has been written
  RCCResult app_inst_name_RX0_cic_dec_written() {
    m_ctrlr.set_app_inst_name_RX0_cic_dec(m_properties.app_inst_name_RX0_cic_dec);
    return RCC_OK;
  }
  // notification that app_inst_name_RX1_qadc property has been written
  RCCResult app_inst_name_RX1_qadc_written() {
    m_ctrlr.set_app_inst_name_RX1_qadc(m_properties.app_inst_name_RX1_qadc);
    return RCC_OK;
  }
  // notification that app_inst_name_RX1_complex_mixer property has been written
  RCCResult app_inst_name_RX1_complex_mixer_written() {
    m_ctrlr.set_app_inst_name_RX1_complex_mixer(m_properties.app_inst_name_RX1_complex_mixer);
    return RCC_OK;
  }
  // notification that app_inst_name_RX1_cic_dec property has been written
  RCCResult app_inst_name_RX1_cic_dec_written() {
    m_ctrlr.set_app_inst_name_RX1_cic_dec(m_properties.app_inst_name_RX1_cic_dec);
    return RCC_OK;
  }
  // notification that app_inst_name_ad9361_data_sub property has been written
  RCCResult app_inst_name_ad9361_data_sub_written() {
    auto& inst = m_properties.app_inst_name_ad9361_data_sub;
    m_ctrlr.set_app_inst_name_ad9361_data_sub(inst);
    return RCC_OK;
  }
  // notification that valid_values_gain_mode property will be read
  RCCResult valid_values_gain_mode_read() {

    auto& this_prop = m_properties.valid_values_gain_mode;
    const unsigned num_streams = get_num_streams();
    Direction_gain_mode drx, dtx;
    drx = VALID_VALUES_GAIN_MODE_DIRECTION_GAIN_MODE_RX;
    dtx = VALID_VALUES_GAIN_MODE_DIRECTION_GAIN_MODE_TX;

    init_valid_values_read(this_prop, drx, dtx, num_streams);

    std::string data_stream_ID;

    for(unsigned ii = 0; ii < num_streams; ii++) {

      Direction_gain_mode dd;
#ifdef IS_BROKEN
      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, 0, &dd);
      this_prop[ii].direction_gain_mode = dd;

      this_prop[ii].valid_values.resize(0);
#else
      std::string ID;

      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, &ID, &dd);
      this_prop[ii].direction_gain_mode = dd;

      typedef std::vector<gain_mode_value_t> tmp;
      const tmp& vr = m_ctrlr.get_ranges_possible_gain_mode(ID);
      try {
        populate_valid_values(this_prop[ii], vr);
      }
      catch(std::string& err) {
        return setError("%s", err.c_str());
      }
#endif
    }

    return RCC_OK;
  }
  // notification that valid_values_gain_dB property will be read
  RCCResult valid_values_gain_dB_read() {

    auto& this_prop = m_properties.valid_values_gain_dB;
    const unsigned num_streams = get_num_streams();
    Direction_gain drx, dtx;
    drx = VALID_VALUES_GAIN_DB_DIRECTION_GAIN_RX;
    dtx = VALID_VALUES_GAIN_DB_DIRECTION_GAIN_TX;

    init_valid_values_read(this_prop, drx, dtx, num_streams);

    std::string data_stream_ID;

    for(unsigned ii = 0; ii < num_streams; ii++) {

      Direction_gain dd;
#ifdef IS_BROKEN
      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, 0, &dd);
      this_prop[ii].direction_gain = dd;

      this_prop[ii].valid_values.resize(0);
#else
      std::string ID;

      get_data_stream_ID_and_or_dir_for_valid_values_idx(drx, dtx, ii, &ID, &dd);
      this_prop[ii].direction_gain = dd;

      ConfigValueRanges vr = m_ctrlr.get_ranges_possible_gain_dB(ID);
      try {
        populate_valid_values(this_prop[ii], vr);
      }
      catch(std::string& err) {
        return setError("%s", err.c_str());
      }
#endif
    }

    return RCC_OK;
  }
  // notification that bist_loopback property has been written
  RCCResult bist_loopback_written() {

    int32_t mode;
    std::string msg;

    mode = (m_properties.bist_loopback == BIST_LOOPBACK_DISABLED) ? 0 : 1;

    if(m_ad9361_rf_phy == 0) {
      try {
        // performs ad9361_init(), which is usually done under-the-hood by the
        // ctrlr when performing config locks, but here it is necessary because
        // we are making low-level No-OS API calls
        m_ctrlr.init();
      }
      catch(std::string& err) {
        return setError(err.c_str());
      }
    }

    msg.assign("No-OS call: ad9361_bist_loopback w/ value: ");
    log(OCPI_LOG_DEBUG, "%s%" PRIi32 "\n", msg.c_str(), mode);

    int32_t ret = ad9361_bist_loopback(m_ad9361_rf_phy, mode);

    if(ret != 0) {
      msg.assign("ad9361_bist_loopback returned non-zero value: ");
      return setError("%s%" PRIi32, msg.c_str(), ret);
    }
    return RCC_OK;
  }
  // notification that bist_loopback property will be read
  RCCResult bist_loopback_read() {

    int32_t mode;
    std::string msg;

    if(m_ad9361_rf_phy == 0) {
      m_properties.bist_loopback = BIST_LOOPBACK_AD9361_INIT_HAS_NOT_OCCURRED;
      return RCC_OK;
    }

    ad9361_get_bist_loopback(m_ad9361_rf_phy, &mode);
    msg.assign("No-OS call: ad9361_get_bist_loopback, read value: ");
    log(OCPI_LOG_DEBUG, "%s%" PRIi32 "\n", msg.c_str(), mode);

    switch(mode) {
      case 0:
        m_properties.bist_loopback = BIST_LOOPBACK_DISABLED;
        break;
      case 1:
        m_properties.bist_loopback = BIST_LOOPBACK_LOOPBACK_AD9361_INTERNAL;
        break;
      default:
        msg.assign("unsupported value read via ad9361_get_bist_loopback: ");
        return setError("%s%" PRIi32, msg.c_str(), mode);
        break;
    }
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/) {
    // this method never gets called (see constructor at top of this file)
    // this was done because there are not ports/no data to process

    // we don't care what is returned, because this method is never called
    return RCC_DONE;
  }
};

DIG_RADIO_CTRLR_FMCOMMS_2_3_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
DIG_RADIO_CTRLR_FMCOMMS_2_3_END_INFO
