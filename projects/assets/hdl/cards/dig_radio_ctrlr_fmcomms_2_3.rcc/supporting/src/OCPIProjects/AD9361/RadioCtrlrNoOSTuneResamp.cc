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

#include <cstdint>  // int32_t, uint8_t, etc
#include <sstream>  // std::ostringstream
#include <cmath>    // round()
#include <unistd.h> // usleep()
#include <cinttypes> // PRIu32
#include "RadioCtrlr.hh"
#include "RadioCtrlrConfigurator.hh" // ConfigValueRanges
#include "LogForwarder.hh"    // LogForwarder
#include "UtilValidRanges.hh" // Util::Range, Util::ValidRanges
#include "No_OS_platform_opencpi_slave_ad9361_config.h" // ad9361_config_config_t 
#include "RadioCtrlrNoOSTuneResamp.hh"
#include "OcpiApi.hh"         // OCPI::API::
#include "RCC_Worker.h"       // OCPI::RCC::RCCUserSlave
#include "OcpiOsDebugApi.hh"  // OCPI_LOG_..., OCPI::OS::logPrintV()
#include "readers_ad9361_rf_rx_pll.h" // get_AD9361_Rx_RFPLL_LO_freq_Hz()
#include "readers_ad9361_rf_tx_pll.h" // get_AD9361_Tx_RFPLL_LO_freq_Hz()
#include "readers_ad9361_bb_rx_adc.h" // get_AD9361_RX_SAMPL_FREQ_Hz()
#include "readers_ad9361_bb_tx_dac.h" // get_AD9361_TX_SAMPL_FREQ_Hz()

extern "C" {
#include "ad9361.h" // (from No-OS) struct ad9361_rf_phy
}

namespace OCPIProjects {

namespace RadioCtrlr {

namespace OA = OCPI::API;

template<class C, class S>
RadioCtrlrNoOSTuneResamp<C, S>::RadioCtrlrNoOSTuneResamp(
    const char* descriptor, S& slave, OA::Application& app) :
    DigRadioCtrlr<OCPI_log_func_args_t, C>(descriptor, m_configurator),
    _ad9361_rf_phy(0),
    m_ad9361_rf_phy(_ad9361_rf_phy),
    m_slave(slave),
    m_AD9361_FREF_Hz(40e6), // just to match up w/ configurator for now...
    m_ad9361_init_ret(-1),
    m_ad9361_init_called(false),
    m_app(app),
    m_configurator_tune_resamp_locked(false),
    m_readback_gain_mode_as_standard_value(false) {

  init_AD9361_InitParam();

  register_OpenCPI_logging_API();
}

template<class C, class S>
RadioCtrlrNoOSTuneResamp<C, S>::RadioCtrlrNoOSTuneResamp(
    const char* descriptor, S& slave, OA::Application& app,
    struct ad9361_rf_phy*& ad9361_rf_phy) :
    DigRadioCtrlr<OCPI_log_func_args_t, C>(descriptor, m_configurator),
    _ad9361_rf_phy(0),
    m_ad9361_rf_phy(ad9361_rf_phy),
    m_slave(slave),
    m_AD9361_FREF_Hz(40e6), // just to match up w/ configurator for now...
    m_ad9361_init_ret(-1),
    m_ad9361_init_called(false),
    m_app(app),
    m_configurator_tune_resamp_locked(false),
    m_readback_gain_mode_as_standard_value(false) {

  init_AD9361_InitParam();

  register_OpenCPI_logging_API();
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::register_OpenCPI_logging_API() {
  // register OpenCPI logging API with the configurator
  set_forwarding_callback_log_info(OCPI::OS::logPrintV);
  set_arg0_log_info(OCPI_LOG_INFO);
  set_forwarding_callback_log_debug(OCPI::OS::logPrintV);
  set_arg0_log_debug(OCPI_LOG_DEBUG);
  set_forwarding_callback_log_warn(OCPI::OS::logPrintV);
  set_arg0_log_warn(OCPI_LOG_INFO); // not sure if warn->info is best...
  set_forwarding_callback_log_error(OCPI::OS::logPrintV);
  set_arg0_log_error(OCPI_LOG_BAD);
  m_configurator.set_forwarding_callback_log_info(OCPI::OS::logPrintV);
  m_configurator.set_arg0_log_info(OCPI_LOG_INFO);
  m_configurator.set_forwarding_callback_log_debug(OCPI::OS::logPrintV);
  m_configurator.set_arg0_log_debug(OCPI_LOG_DEBUG);
  m_configurator.set_forwarding_callback_log_warn(OCPI::OS::logPrintV);
  m_configurator.set_arg0_log_warn(OCPI_LOG_INFO); // not sure if warn->info is best...
  m_configurator.set_forwarding_callback_log_error(OCPI::OS::logPrintV);
  m_configurator.set_arg0_log_error(OCPI_LOG_BAD);
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::request_config_lock(
    const config_lock_ID_t   config_lock_ID,
    const ConfigLockRequest& config_lock_request) {

  if(not configurator_check_and_reinit(config_lock_request, m_configurator)) {
    return false;
  }

  auto ID = config_lock_ID;
  auto request = config_lock_request;
  return DigRadioCtrlr<OCPI_log_func_args_t, C>::request_config_lock(ID,request);
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::lock_tuning_freq_complex_mixer_MHz(
    const data_stream_ID_t ds_ID, const std::string& inst,
    const config_value_t val, const config_value_t tol) {

  // the configurator, which is a software emulation of hardware capabilties,
  // tells us whether a hardware attempt to set value will corrupt
  // any existing locks
  const config_key_t cfg_key = "tuning_freq_complex_mixer_MHz";
  bool did_lock = this->m_configurator.lock_config(ds_ID, cfg_key, val, tol);

  bool is = false; // is within tolerance
  if(did_lock) {
    Meas<config_value_t> meas = set_tuning_freq_complex_mixer_MHz(ds_ID, inst, val);

    is = this->config_val_is_within_tolerance(val, tol, meas);

    this->log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "MHz", &meas);
  }
  else {
    this->log_info_config_lock(did_lock, is, ds_ID, val, tol, cfg_key, "MHz");
  }

  return did_lock and is;
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::unlock_tuning_freq_complex_mixer_MHz(
    const data_stream_ID_t ds_ID) {
  
  this->unlock_config(ds_ID, "tuning_freq_complex_mixer_MHz");
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::do_min_data_stream_config_locks(
    const data_stream_ID_t             ds_ID,
    const DataStreamConfigLockRequest& req) {

  throw_if_data_stream_lock_request_malformed(req);

  config_key_t key;
  config_value_t val, tol;
  std::string inst;

  if((ds_ID == "SMA_TX1A") or (ds_ID == "SMA_TX2A")) {
    inst.assign(get_inst_name_cic_int(req));
    key = "CIC_int_interpolation_factor";
    if(inst.empty()) {
      val = 1;
    }
    else {
      val = m_app.getPropertyValue<OA::UShort>(inst, "R");
    }
  }
  else if((ds_ID == "SMA_RX1A") or (ds_ID == "SMA_RX2A")) {
    inst.assign(get_inst_name_cic_dec(req));
    key = "CIC_dec_decimation_factor";
    if(inst.empty()) {
      val = 1;
    }
    else {
      val = m_app.getPropertyValue<OA::UShort>(inst, "R");
    }
  }
  tol = 0;
  if(not m_configurator.lock_config(ds_ID, key, val, tol)) {
    log_debug("unexpected config lock failure");
    goto unrollandfail;
  }

  inst.assign(get_inst_name_complex_mixer(req));

  // there is currently no use case to lock complex_mixer tune freq to any
  // value other than 0
  val = 0.;
  tol = 0.;

  if(inst.empty()) {
    key = "tuning_freq_complex_mixer_MHz";
    // intentionally ignoring return value
    m_configurator.lock_config(ds_ID, key, val, tol);
  }
  else {
    log_debug("data stream %s, which is associated w/ routing ID %s, has complex mixer", ds_ID.c_str(), req.get_routing_ID().c_str());
    if(not lock_tuning_freq_complex_mixer_MHz(ds_ID, inst, val, tol)) {
      goto unrollandfail;
    }
  }

  if(not DigRadioCtrlr<OCPI_log_func_args_t, C>::do_min_data_stream_config_locks(ds_ID, req)) {
    return false; // unroll done inside do_min...(), so not necessary again
  }

  return true;

  unrollandfail:
  if((ds_ID == "SMA_TX1A") or (ds_ID == "SMA_TX2A")) {
    m_configurator.unlock_config(ds_ID, "CIC_int_interpolation_factor");
  }
  else if((ds_ID == "SMA_RX1A") or (ds_ID == "SMA_RX2A")) {
    m_configurator.unlock_config(ds_ID, "CIC_dec_decimation_factor");
  }
  unlock_tuning_freq_complex_mixer_MHz(ds_ID);
  return false;
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::throw_if_data_stream_lock_request_malformed(
    const DataStreamConfigLockRequest& req) const {

  DigRadioCtrlr<OCPI_log_func_args_t, C>::throw_if_data_stream_lock_request_malformed(req);

  // this class's implementation data stream config lock requests to include
  // routing ID (because there is more than one data stream of the same type)
  if(not req.get_including_routing_ID()) {
    throw std::string("radio controller's data stream config lock request malformed: did not include routing ID");
  }

  if((req.get_routing_ID() != "RX0") and
     (req.get_routing_ID() == "RX1") and
     (req.get_routing_ID() == "TX0") and
     (req.get_routing_ID() == "TX1")) {
    std::ostringstream oss;
    oss << "radio controller's data stream config lock request malformed: ";
    oss << "routing ID of " << req.get_routing_ID() << " was not one of the ";
    oss << "supported value: RX0, RX1, TX0, TX1";
    throw oss.str();
  }
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::get_tuning_freq_MHz(
    const data_stream_ID_t data_stream_ID) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "tuning freq");

  if(not m_ad9361_init_called) {
    throw std::string("attempted to read one of the AD961 data stream's tuning_freq_MHz values before AD9361 was initialized");
  }

  // this function essentially maps the generic representation which a
  // digital radio controller provides to methods which calculate
  // the thereotical value (direct AD9361 register reads) with high precision

  auto configurator_copy = m_configurator; // const necessitates this...
  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {

    const ConfigValueRanges& x = configurator_copy.get_ranges_possible(data_stream_ID,"tuning_freq_complex_mixer_MHz");

    /// @todo / FIXME - get locked value instead of assuming smallest min is equivalent to locked value
    config_value_t tuning_freq_complex_mixer_MHz = x.get_smallest_min();

    Meas<config_value_t> meas(meas_type_t::THEORETICAL);
    meas.m_unit.assign("MHz"); // we usually stick with MHz as the standard

    double val;
    const double& fref = m_AD9361_FREF_Hz;
    this->throw_if_ad9361_init_failed("calculation of Rx RFPLL LO freq Hz based on AD9361 register contents");

    const char* err = get_AD9361_Rx_RFPLL_LO_freq_Hz(val, m_slave, fref);
    if(err != 0) {
      throw err;
    }
    meas.m_value = (val/1e6) - tuning_freq_complex_mixer_MHz;
    std::ostringstream oss;
    oss << meas;
    log_debug("data stream: %s, read from hardware tuning freq value of: %s", data_stream_ID.c_str(), oss.str().c_str());

    return meas;
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {

    const ConfigValueRanges& x = configurator_copy.get_ranges_possible(data_stream_ID,"tuning_freq_complex_mixer_MHz");

    /// @todo / FIXME - get locked value instead of assuming smallest min is equivalent to locked value
    config_value_t tuning_freq_complex_mixer_MHz = x.get_smallest_min();

    // get_AD9361_Tx_RFPLL_LO_freq_Hz() is a (highly precise) theoretical
    // calculation based upon AD9361 register values
    Meas<config_value_t> meas(meas_type_t::THEORETICAL);
    meas.m_unit.assign("MHz"); // we usually stick with MHz as the standard

    double val;
    const double& fref = m_AD9361_FREF_Hz;
    this->throw_if_ad9361_init_failed("calculation of Tx RFPLL LO freq Hz based on AD9361 register contents");
    const char* err = get_AD9361_Tx_RFPLL_LO_freq_Hz(val, m_slave, fref);
    if(err != 0) {
      throw err;
    }
    meas.m_value = (val/1e6) - tuning_freq_complex_mixer_MHz;
    std::ostringstream oss;
    oss << meas;
    log_debug("data stream: %s, read from hardware tuning freq value of: %s", data_stream_ID.c_str(), oss.str().c_str());

    return meas;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::get_bandwidth_3dB_MHz(
    const data_stream_ID_t data_stream_ID) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "bandwidth");

  if(not m_ad9361_init_called) {
    throw std::string("attempted to read one of the AD961 data stream's bandwidth_3dB_MHz values before AD9361 was initialized");
  }

  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {

    // No-OS ad9361_get_rx_rf_bandwidth() call has nominal precision only
    Meas<config_value_t> meas(meas_type_t::NOMINAL);
    meas.m_unit.assign("MHz"); // we usually stick with MHz as the standard

    uint32_t bandwidth_hz;
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_get_rx_rf_bandwidth()");
    int32_t ret = ad9361_get_rx_rf_bandwidth(m_ad9361_rf_phy, &bandwidth_hz);
    if(ret != 0) {
      throw this->get_No_OS_err_str("ad9361_get_rx_rf_bandwidth", ret);
    }
    log_debug("No-OS call: ad9361_get_rx_rf_bandwidth, read value: %" PRIu32, bandwidth_hz);
    meas.m_value = ((double) bandwidth_hz) / 1e6;
    std::ostringstream oss;
    oss << meas;
    log_debug("data stream: %s, read from hardware AD9361 rx_rf_bandwidth value of: %s", data_stream_ID.c_str(), oss.str().c_str());
  
    return meas;
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {

    // No-OS ad9361_get_tx_rf_bandwidth() call has nominal precision
    Meas<config_value_t> meas(meas_type_t::NOMINAL);
    meas.m_unit.assign("MHz"); // we usually stick with MHz as the standard

    //NO_OS_CALL(ret, ad9361_get_tx_rf_bandwidth, phy, meas.m_value)
    uint32_t bandwidth_hz;
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_get_tx_rf_bandwidth()");
    int32_t ret = ad9361_get_tx_rf_bandwidth(m_ad9361_rf_phy, &bandwidth_hz);
    if(ret != 0) {
      throw this->get_No_OS_err_str("ad9361_get_tx_rf_bandwidth", ret);
    }
    log_debug("No-OS call: ad9361_get_tx_rf_bandwidth, read value: %" PRIu32, bandwidth_hz);
    meas.m_value = ((double) bandwidth_hz) / 1e6;
    std::ostringstream oss;
    oss << meas;
    log_debug("read from hardware AD9361 tx_rf_bandwidth value of: %s", oss.str().c_str());

    return meas;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::get_tuning_freq_complex_mixer_MHz(
    const data_stream_ID_t data_stream_ID,
    const std::string&     inst) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "complex_mixer tuning freq");

  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A") or
     (data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {

    // measurement is based on samp rate, which is theoretical
    Meas<config_value_t> meas(meas_type_t::THEORETICAL);

    meas.m_unit.assign("MHz");

    const Meas<config_value_t> sr= get_sampling_rate_Msps(data_stream_ID);
    const config_value_t sample_freq = sr.m_value;

    OA::Short phs_inc = m_app.getPropertyValue<OA::Short>(inst, "phs_inc");

    // see Complex_Mixer.pdf eq. (4)
    double nco_output_freq = ((double)sample_freq) * ((double)phs_inc) / 65536.;
    
    // It is desired that setting a + IF freq results in mixing *down*. Because
    // complex_mixer's NCO mixes *up* for + freqs (see complex mixer datasheet),
    // IF frequency is accurately reported as the negative of the NCO freq.
    meas.m_value = -nco_output_freq;

    //log_info("meas.m_value=%0.15f",meas.m_value);
    //log_info("phs_inc=%li",phs_inc);
    //log_info("sample_freq=%0.15f",sample_freq);
    std::ostringstream oss;
    oss << meas;
    log_debug("data stream: %s, read from hardware tuning freq complex mixer value of: %s", data_stream_ID.c_str(), oss.str().c_str());

    return meas;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
}

/*template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::get_worker_exists_in_app(
    const char* inst) const {
  std::string val;
  app.getProperty(inst, "ocpi_debug", val);
  return (val.compare("true") == 0);
}*/

/// @todo / FIXME - check for existence of qadc/qdac?
template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::get_data_stream_is_enabled(
    const data_stream_ID_t data_stream_ID) const {

  if(not m_ad9361_init_called) {
    //throw std::string("ad9361_init was never called");
    return false;
  }

  bool data_stream_is_enabled = false;

  if(data_stream_ID == "SMA_RX1A") {
    if(m_AD9361_InitParam.two_rx_two_tx_mode_enable) {
      data_stream_is_enabled = true;
    }
    else {
      if(m_AD9361_InitParam.one_rx_one_tx_mode_use_rx_num == 1) {
        data_stream_is_enabled = true;
      }
    }
  }
  else if(data_stream_ID == "SMA_RX2A") {
    if(m_AD9361_InitParam.two_rx_two_tx_mode_enable) {
      data_stream_is_enabled = true;
    }
    else {
      if(m_AD9361_InitParam.one_rx_one_tx_mode_use_rx_num == 2) {
        data_stream_is_enabled = true;
      }
    }
  }
  else if(data_stream_ID == "SMA_TX1A") {
    if(m_AD9361_InitParam.two_rx_two_tx_mode_enable) {
      data_stream_is_enabled = true;
    }
    else {
      if(m_AD9361_InitParam.one_rx_one_tx_mode_use_tx_num == 1) {
        data_stream_is_enabled = true;
      }
    }
  }
  else if(data_stream_ID == "SMA_TX2A") {
    if(m_AD9361_InitParam.two_rx_two_tx_mode_enable) {
      data_stream_is_enabled = true;
    }
    else {
      if(m_AD9361_InitParam.one_rx_one_tx_mode_use_tx_num == 2) {
        data_stream_is_enabled = true;
      }
    }
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
  return data_stream_is_enabled;
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::get_sampling_rate_Msps(
    const data_stream_ID_t data_stream_ID) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "sampling rate");

  // this function essentially maps the generic representation which a
  // digital radio controller provides to methods which calculate
  // the thereotical value (direct AD9361 register reads) with high precision

  auto configurator_copy = m_configurator; // const necessitates this...
  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {

    /// @todo / FIXME - the best thing to do is probably throw an exception if CIC_dec_decimation_factor is not locked (how can we read the hardware sampling rate if we don't know the decimation factor?)
    const ConfigValueRanges& x = configurator_copy.get_ranges_possible(data_stream_ID,"CIC_dec_decimation_factor");
    config_value_t CIC_dec_decimation_factor = x.get_smallest_min();

    // get_AD9361_RX_SAMPL_FREQ_Hz() is a (highly precise) theoretical
    // calculation based upon AD9361 register values
    Meas<config_value_t> meas(meas_type_t::THEORETICAL);
    meas.m_unit.assign("MHz"); // we usually stick with MHz as the standard

    double val;
    const double& fref = m_AD9361_FREF_Hz;
    this->throw_if_ad9361_init_failed("No-OS API call get_AD9361_RX_SAMPL_FREQ_Hz()");
    const char* err = get_AD9361_RX_SAMPL_FREQ_Hz(val, m_slave, fref);
    if(err != 0) {
      throw err;
    }
    meas.m_value = val/CIC_dec_decimation_factor/1e6;
    std::ostringstream oss;
    oss << meas;


    log_debug("data stream: %s, read from hardware sampling rate value of: %s", data_stream_ID.c_str(), oss.str().c_str());
    {
      Meas<config_value_t> m2 = meas;
      m2.m_value = val/1e6;
      oss.str("");
      oss.clear();
      oss << m2;
      log_debug("data stream: %s, read from hardware AD9361 RX_SAMPL_FREQ value of: %s", data_stream_ID.c_str(), oss.str().c_str());
    }

    return meas;
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {

    /// @todo / FIXME - the best thing to do is probably throw an exception if CIC_int_interpolation_factor is not locked (how can we read the hardware sampling rate if we don't know the interpolation factor?)
    const ConfigValueRanges& x = configurator_copy.get_ranges_possible(data_stream_ID,"CIC_int_interpolation_factor");
    config_value_t CIC_int_interpolation_factor = x.get_smallest_min();

    // get_AD9361_RX_SAMPL_FREQ_Hz() is a (highly precise) theoretical
    // calculation based upon AD9361 register values
    Meas<config_value_t> meas(meas_type_t::THEORETICAL);
    meas.m_unit.assign("MHz"); // we usually stick with MHz as the standard

    double val;
    const double& fref = m_AD9361_FREF_Hz;
    this->throw_if_ad9361_init_failed("No-OS API call get_AD9361_TX_SAMPL_FREQ_Hz()");
    const char* err = get_AD9361_TX_SAMPL_FREQ_Hz(val, m_slave, fref);
    if(err != 0) {
      throw err;
    }
    meas.m_value = val/CIC_int_interpolation_factor/1e6;
    std::ostringstream oss;
    oss << meas;
    log_debug("data stream: %s, read from hardware sampling rate value of: %s", data_stream_ID.c_str(), oss.str().c_str());
    {
      Meas<config_value_t> m2 = meas;
      m2.m_value = val/1e6;
      oss.str("");
      oss.clear();
      oss << m2;
      log_debug("data stream: %s, read from hardware AD9361 TX_SAMPL_FREQ value of: %s", data_stream_ID.c_str(), oss.str().c_str());
    }

    return meas;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::get_samples_are_complex(
    const data_stream_ID_t data_stream_ID) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "samples are complex");

  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A") or
     (data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {

    // AD9361 samples are always complex
    log_debug("data stream: %s, read from hardware samples_are_complex value of: true", data_stream_ID.c_str());
    return true;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
}

template<class C, class S>
Meas<gain_mode_value_t> RadioCtrlrNoOSTuneResamp<C, S>::get_gain_mode(
    const data_stream_ID_t data_stream_ID) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "gain mode");

  Meas<gain_mode_value_t> meas(meas_type_t::EXACT);

  // assign to No-OS-specific "channel" macro (RX1/RX2/TX1/TX2 in ad9361_api.h)
  uint8_t ch;

  bool do_rx = false;
  if(data_stream_ID == "SMA_RX1A") {
    if(m_ad9361_rf_phy->pdata->rx2tx2) {
      ch = RX1;
    }
    else {
      ch = (m_AD9361_InitParam.one_rx_one_tx_mode_use_rx_num == 1) ? RX1 : RX2;
    }
    do_rx = true;
  }
  else if(data_stream_ID == "SMA_RX2A") {
    if(m_ad9361_rf_phy->pdata->rx2tx2) {
      ch = RX2;
    }
    else {
      ch = (m_AD9361_InitParam.one_rx_one_tx_mode_use_rx_num == 2) ? RX1 : RX2;
    }
    do_rx = true;
  }
  if(do_rx) {

    uint8_t gc_mode;
    int32_t ret;
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_get_rx_gain_control_mode()");
    ret = ad9361_get_rx_gain_control_mode(m_ad9361_rf_phy, ch, &gc_mode);
    if(ret != 0) {
      throw this->get_No_OS_err_str("ad9361_get_rx_gain_control_mode", ret);
    }
    log_debug("No-OS call: ad9361_get_rx_gain_control_mode, read value: %" PRIu8 ", channel: %" PRIu8, gc_mode, ch);

    switch(gc_mode) {
      case RF_GAIN_MGC:
        log_debug("data stream: %s, read from hardware gain mode value of: RF_GAIN_MGC (manual)", data_stream_ID.c_str());
        if(m_readback_gain_mode_as_standard_value) {
          meas.m_value.assign("manual");
        }
        else {
          meas.m_value.assign("RF_GAIN_SLOWATTACK_AGC");
        }
        break;
      case RF_GAIN_FASTATTACK_AGC:
        log_debug("data stream: %s, read from hardware gain mode value of: RF_GAIN_FASTATTACK_AGC", data_stream_ID.c_str());
        meas.m_value.assign("RF_GAIN_FASTATTACK_AGC");
        break;
      case RF_GAIN_SLOWATTACK_AGC:
        log_debug("data stream: %s, read from hardware gain mode value of: RF_GAIN_SLOWATTACK_AGC (auto)", data_stream_ID.c_str());
        if(m_readback_gain_mode_as_standard_value) {
          meas.m_value.assign("auto");
        }
        else {
          meas.m_value.assign("RF_GAIN_SLOWATTACK_AGC");
        }
        break;
      case RF_GAIN_HYBRID_AGC:
        log_debug("data stream: %s, read from hardware gain mode value of: RF_GAIN_HYBRID_AGC", data_stream_ID.c_str());
        meas.m_value.assign("RF_GAIN_HYBRID_AGC");
        break;
      default:
        std::ostringstream oss;
        oss << "invalid value read from ad9361_get_rx_gain_control_mode(): ";
        oss << (unsigned) gc_mode;
        oss << ", expected one of the values: ";
        oss << RF_GAIN_MGC << ", ";
        oss << RF_GAIN_FASTATTACK_AGC << ", ";
        oss << RF_GAIN_SLOWATTACK_AGC << ", ";
        oss << RF_GAIN_HYBRID_AGC;
        throw oss.str().c_str();
    }
    return meas;
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {

    meas.m_value.assign("manual");

    return meas;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::get_gain_dB(
    const data_stream_ID_t data_stream_ID) const {

  this->throw_if_data_stream_disabled_for_read(data_stream_ID, "gain");

  Meas<config_value_t> meas(meas_type_t::THEORETICAL);
  meas.m_unit.assign("dB"); // we usually stick with dB as the standard

  // assign to No-OS-specific "channel" macro (RX1/RX2/TX1/TX2 in ad9361_api.h)
  uint8_t ch;

  bool do_rx = false;
  if(data_stream_ID == "SMA_RX1A") {
    ch = RX1;
    do_rx = true;
  }
  else if(data_stream_ID == "SMA_RX2A") {
    ch = m_ad9361_rf_phy->pdata->rx2tx2 ? RX2 : RX1;
    do_rx = true;
  }
  bool do_tx = false;
  if(do_rx) {

    int32_t gain_db;
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_get_rx_rf_gain()");
    int32_t ret = ad9361_get_rx_rf_gain(m_ad9361_rf_phy, ch, &gain_db);
    if(ret != 0) {
      throw this->get_No_OS_err_str("ad9361_get_rx_rf_gain", ret);
    }
    log_debug("No-OS call: ad9361_get_rx_rf_gain, read value: %" PRIi32 ", channel: %" PRIu8, gain_db, ch);

    meas.m_value = (double) gain_db;
    std::ostringstream oss;
    oss << meas;
    log_debug("data stream: %s, read from hardware AD9361 rx_rf_gain value of: %s", data_stream_ID.c_str(), oss.str().c_str());

    return meas;
  }
  else if(data_stream_ID == "SMA_TX1A") {
    ch = TX1;
    do_tx = true;
  }
  else if(data_stream_ID == "SMA_TX2A") {
    ch = m_ad9361_rf_phy->pdata->rx2tx2 ? TX2 : TX1;
    do_tx = true;
  }
  if(do_tx) {

    // note that gain mode is never auto for either TX data stream

    uint32_t attenuation_mdb;
    int32_t ret;
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_get_tx_attenuation()");
    ret = ad9361_get_tx_attenuation(m_ad9361_rf_phy, ch, &attenuation_mdb);
    if(ret != 0) {
      throw this->get_No_OS_err_str("ad9361_get_tx_attenuation", ret);
    }
    log_debug("No-OS call: ad9361_get_tx_attenuation, read value: %" PRIu32 ", channel: %" PRIu8, attenuation_mdb, ch);
    meas.m_value = -((double) attenuation_mdb)/1000.;
    std::ostringstream oss;
    oss << attenuation_mdb;
    log_debug("data stream: %s, read from hardware AD9361 tx_attenuation value of: %s", data_stream_ID.c_str(), oss.str().c_str());
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
  return meas;
}

template<class C, class S>
RadioCtrlrNoOSTuneResamp<C, S>::~RadioCtrlrNoOSTuneResamp() {
  // only free internally managed No-OS memory
  ad9361_free(_ad9361_rf_phy); // this function added in ad9361.patch
}

template<class LC, class C>
void RadioCtrlrNoOSTuneResamp<LC, C>::unlock_all() {

  log_debug("configurator: unlock_all1");
  this->m_configurator.unlock_all();
  m_config_locks.clear();

  // required for all ...TuneResamp classes
  m_configurator_tune_resamp_locked = false;
  //ensure_configurator_lock_tune_resamp();
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::set_tuning_freq_MHz(
    const data_stream_ID_t data_stream_ID,
    const config_value_t   tuning_freq_MHz) {

  this->throw_if_data_stream_disabled_for_write(data_stream_ID, "tuning freq");

  // this function essentially maps the generic representation which a
  // digital radio controller provides to the underlying No-OS API calls
  // ---------------------------------------------------------------------
  // digital radio controller data stream   | No-OS API call
  // ---------------------------------------------------------------------
  // RX1                                    | ad9361_set_rx_lo_freq ()
  // RX2                                    | ad9361_set_rx_lo_freq ()
  // TX1                                    | ad9361_set_tx_lo_freq ()
  // TX2                                    | ad9361_set_tx_lo_freq ()

  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {
    uint64_t lo_freq_hz = round(tuning_freq_MHz*1e6);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_rx_lo_freq()");

    log_debug("data stream: %s, configuring hardware w/ tuning freq value of: %" PRIu64 " Hz", data_stream_ID.c_str(), lo_freq_hz);
    log_debug("No-OS call: ad9361_set_rx_lo_freq w/ value: %" PRIu64, lo_freq_hz);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_rx_lo_freq(m_ad9361_rf_phy, lo_freq_hz);
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {
    uint64_t lo_freq_hz = round(tuning_freq_MHz*1e6);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_tx_lo_freq()");

    log_debug("data stream: %s, configuration hardware w/ tuning freq value of: %" PRIu64 " Hz", data_stream_ID.c_str(), lo_freq_hz);
    log_debug("No-OS call: ad9361_set_tx_lo_freq w/ value: %" PRIu64, lo_freq_hz);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_tx_lo_freq(m_ad9361_rf_phy, lo_freq_hz);
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
  return get_tuning_freq_MHz(data_stream_ID);
}

template<class C, class S>
Meas<config_value_t>
RadioCtrlrNoOSTuneResamp<C, S>::set_bandwidth_3dB_MHz(
    const data_stream_ID_t data_stream_ID,
    const config_value_t   bandwidth_3dB_MHz) {

  this->throw_if_data_stream_disabled_for_write(data_stream_ID, "bandwidth");

  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {
    uint32_t bandwidth_hz = round(bandwidth_3dB_MHz*1e6);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_rx_rf_bandwidth()");

    log_debug("data stream: %s, configuring hardware w/ 3dB bandwidth value of: %" PRIu32 " Hz", data_stream_ID.c_str(), bandwidth_hz);
    log_debug("No-OS call: ad9361_set_rx_rf_bandwidth w/ value: %" PRIu32, bandwidth_hz);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_rx_rf_bandwidth(m_ad9361_rf_phy, bandwidth_hz);
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {
    uint32_t bandwidth_hz = round(bandwidth_3dB_MHz*1e6);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_tx_rf_bandwidth()");

    log_debug("data stream: %s, configuring hardware w/ 3dB bandwidth value of: %" PRIu32 " Hz", data_stream_ID.c_str(), bandwidth_hz);
    log_debug("No-OS call: ad9361_set_tx_rf_bandwidth w/ value: %" PRIu32, bandwidth_hz);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_tx_rf_bandwidth(m_ad9361_rf_phy, bandwidth_hz);
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
  return get_bandwidth_3dB_MHz(data_stream_ID);
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::set_sampling_rate_Msps(
    const data_stream_ID_t data_stream_ID,
    const config_value_t   sampling_rate_Msps) {

  this->throw_if_data_stream_disabled_for_write(data_stream_ID, "sampling rate");

  if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {
    const ConfigValueRanges& x = m_configurator.get_ranges_possible(data_stream_ID,"CIC_dec_decimation_factor");

    /// @todo / FIXME - get locked value instead of assuming smallest min is equivalent to locked value
    config_value_t tmp = x.get_smallest_min();

    uint32_t sampling_freq_hz = round(sampling_rate_Msps*tmp*1e6);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_rx_sampling_freq()");

    log_debug("data stream: %s, configuring hardware w/ sampling rate value of: %f sps", data_stream_ID.c_str(), ((double)sampling_freq_hz)/tmp);
    log_debug("No-OS call: ad9361_set_rx_sampling_freq w/ value: %" PRIu32, sampling_freq_hz);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_rx_sampling_freq(m_ad9361_rf_phy, sampling_freq_hz);
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {
    const ConfigValueRanges& x = m_configurator.get_ranges_possible(data_stream_ID,"CIC_int_interpolation_factor");

    /// @todo / FIXME - get locked value instead of assuming smallest min is equivalent to locked value
    config_value_t tmp = x.get_smallest_min();

    uint32_t sampling_freq_hz = round(sampling_rate_Msps*tmp*1e6);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_tx_sampling_freq()");

    log_debug("data stream: %s, configuring hardware w/ sampling rate value of: %f sps", data_stream_ID.c_str(), sampling_freq_hz/tmp);
    log_debug("No-OS call: ad9361_set_tx_sampling_freq w/ value: %" PRIu32, sampling_freq_hz);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_tx_sampling_freq(m_ad9361_rf_phy, sampling_freq_hz);
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
  return get_sampling_rate_Msps(data_stream_ID);
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::set_samples_are_complex(
    const data_stream_ID_t data_stream_ID,
    const bool             samples_are_complex) {

  this->throw_if_data_stream_disabled_for_write(data_stream_ID, "samples are complex");

  // samples are always complex for AD9361, so there is nothing to do

  if(samples_are_complex) { // purposefully ignore compiler warning
  }

  if(not ((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A") or
          (data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A"))) {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str().c_str();
  }
  log_debug("data stream: %s, configuring hardware w/ samples are complex value of true", data_stream_ID.c_str());
  return get_samples_are_complex(data_stream_ID);
}

template<class C, class S>
Meas<gain_mode_value_t> RadioCtrlrNoOSTuneResamp<C, S>::set_gain_mode(
    const data_stream_ID_t  data_stream_ID,
    const gain_mode_value_t gain_mode) {

  this->throw_if_data_stream_disabled_for_write(data_stream_ID, "gain mode");

  // assign to No-OS-specific "channel" macro (RX1/RX2/TX1/TX2 in ad9361_api.h)
  uint8_t ch;

  bool do_rx = false;
  if(data_stream_ID == "SMA_RX1A") {
    ch = RX1;
    do_rx = true;
  }
  else if(data_stream_ID == "SMA_RX2A") {
    ch = m_ad9361_rf_phy->pdata->rx2tx2 ? RX2 : RX1;
    do_rx = true;
  }

  if(do_rx) {

    log_debug("data stream: %s, configuring hardware w/ gain mode setting of %s", data_stream_ID.c_str(), gain_mode.c_str());
    uint8_t gc_mode;
    if(gain_mode.compare("manual") == 0) {
      gc_mode = RF_GAIN_MGC;
      m_readback_gain_mode_as_standard_value = true;
    }
    else if(gain_mode.compare("RF_GAIN_MGC") == 0) {
      gc_mode = RF_GAIN_MGC;
      m_readback_gain_mode_as_standard_value = false;
    }
    else if(gain_mode.compare("auto") == 0) {
      gc_mode = RF_GAIN_SLOWATTACK_AGC;
      m_readback_gain_mode_as_standard_value = true;
    }
    else if(gain_mode.compare("RF_GAIN_SLOWATTACK_AGC") == 0) {
      gc_mode = RF_GAIN_SLOWATTACK_AGC;
      m_readback_gain_mode_as_standard_value = false;
    }
    else if(gain_mode.compare("RF_GAIN_FASTATTACK_AGC") == 0) {
      gc_mode = RF_GAIN_FASTATTACK_AGC;
    }
    else if(gain_mode.compare("RF_GAIN_HYBRID_AGC") == 0) {
      gc_mode = RF_GAIN_HYBRID_AGC;
    }
    else {
      std::string str("gain mode of ");
      str += gain_mode;
      str += " is invalid";
      throw str;
    }

    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_rx_gain_control_mode()");

    log_debug("No-OS call: ad9361_set_rx_gain_control_mode w/ value: %" PRIu32 ", channel: %" PRIu8, gc_mode, ch);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_rx_gain_control_mode(m_ad9361_rf_phy, ch, gc_mode);
  }
  else if((data_stream_ID == "SMA_TX1A") or (data_stream_ID == "SMA_TX2A")) {
    if(gain_mode.compare("manual") != 0) { 
      std::string str("gain mode of ");
      str += gain_mode;
      str += " is invalid for data stream";
      str += data_stream_ID.c_str();
      throw str;
    }
    // AD9361 does not support a changable TX gain mode, so there's nothing to
    // do
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str();
  }
  return get_gain_mode(data_stream_ID);
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::set_gain_dB(
    const data_stream_ID_t data_stream_ID,
    const config_value_t   gain_dB) {

  this->throw_if_data_stream_disabled_for_write(data_stream_ID, "gain");

  // assign to No-OS-specific "channel" macro (RX1/RX2/TX1/TX2 in ad9361_api.h)
  uint8_t ch;

  bool do_rx = false;
  if(data_stream_ID == "SMA_RX1A") {
    ch = RX1;
    do_rx = true;
  }
  else if(data_stream_ID == "SMA_RX2A") {
    ch = m_ad9361_rf_phy->pdata->rx2tx2 ? RX2 : RX1;
    do_rx = true;
  }
  bool do_tx = false;
  if(do_rx) {

    Meas<gain_mode_value_t> mode = get_gain_mode(data_stream_ID);

    int32_t gain_db = round(gain_dB);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_rx_rf_gain()");

    log_debug("data stream: %s, configuring hardware w/ gain value of %" PRIi32 " dB", data_stream_ID.c_str(), gain_db);
    log_debug("No-OS call: ad9361_set_rx_rf_gain w/ value: %" PRIi32 ", channel: %" PRIu8, gain_db, ch);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_rx_rf_gain(m_ad9361_rf_phy, ch, gain_db);
  }
  else if(data_stream_ID == "SMA_TX1A") {
    ch = TX1;
    do_tx = true;
  }
  else if(data_stream_ID == "SMA_TX2A") {
    ch = m_ad9361_rf_phy->pdata->rx2tx2 ? TX2 : TX1;
    do_tx = true;
  }
  else {
    std::ostringstream oss;
    oss << "invalid data stream ID requested: " << data_stream_ID;
    throw oss.str();
  }
  if(do_tx) {
    // this check should be done before calling
    // ad9361_get_rx_rf_gain(_,RX2,_);
    if((m_ad9361_rf_phy->pdata->rx2tx2 == 0) and (ch == RX2)) {
      /// @todo / FIXME - query for controller data stream disablement here?
      throw std::string("requested read of gain for disabled AD9361 RX2 data stream");
    }

    // note that gain mode is never auto for either TX data stream

    uint32_t attenuation_mdb = round(-gain_dB*1000.);
    this->throw_if_ad9361_init_failed("No-OS API call ad9361_set_tx_attenuation()");

    log_debug("data stream: %s, configuring hardware w/ gain value of %f dB", data_stream_ID.c_str(), (double)-attenuation_mdb*1000);
    log_debug("No-OS call: ad9361_set_tx_attenuation w/ value: %" PRIu32 ", channel: %" PRIu8, attenuation_mdb, ch);
    // intentionally ignoring return value because this function provides no
    // guarantee of success
    ad9361_set_tx_attenuation(m_ad9361_rf_phy, ch, attenuation_mdb);
  }
  return get_gain_dB(data_stream_ID);
}

template<class C, class S>
Meas<config_value_t> RadioCtrlrNoOSTuneResamp<C, S>::set_tuning_freq_complex_mixer_MHz(
    const data_stream_ID_t data_stream_ID, const std::string& inst,
    const config_value_t tuning_freq_MHz) {

  const Meas<config_value_t> meas = get_sampling_rate_Msps(data_stream_ID);
  const config_value_t sample_freq = meas.m_value;

  // It is desired that setting a + IF freq results in mixing *down*.
  // Because complex_mixer's NCO mixes *up* for + freqs (see complex mixer
  // datasheet), IF tune freq must be negated in order to achieve the
  // desired effect.
  config_value_t nco_output_freq = -tuning_freq_MHz;

  // todo this math might be better off in a small proxy that sits on top of complex_mixer
  // from complex mixer datasheet, nco_output_freq =
  // sample_freq * phs_inc / 2^phs_acc_width, phs_acc_width is fixed at 16
  OA::Short phs_inc = round(nco_output_freq / sample_freq * 65536.);

  m_app.setPropertyValue<OA::Short>(inst, "phs_inc", phs_inc);

  return get_tuning_freq_complex_mixer_MHz(data_stream_ID, inst);
}

/*template<class LC, class C>
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
}*/

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::log_debug_ad9361_init(
    AD9361_InitParam*& init_param) const {

  log_debug("No-OS call: ad9361_init w/ values: "
            "rx_data_clock_delay=%" PRIu32
            ",rx_data_delay=%" PRIu32
            ",tx_fb_clock_delay=%" PRIu32
            ",tx_data_delay=%" PRIu32,
            init_param->rx_data_clock_delay,
            init_param->rx_data_delay,
            init_param->tx_fb_clock_delay,
            init_param->tx_data_delay);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::init() {

  // only initialize if there are no existing locks which conflict
  if(any_configurator_configs_locked_which_prevent_ad9361_init()) {
    throw std::string("reinit required but configurator configs locked which prevent reinit");
  }

  if(not m_ad9361_init_called) { // spi_init() only needs to happen once
    // nasty cast below included since compiler wouldn't let us cast from
    // ...WorkerTypes::...WorkerBase::Slave to
    // OCPI::RCC_RCCUserSlave since the former inherits privately from the
    // latter inside the RCC worker's generated header
    spi_init(static_cast<OCPI::RCC::RCCUserSlave*>(static_cast<void *>(&m_slave)));
  }

  // initialize No-OS using the No-OS platform_opencpi layer and a slave
  // device of ad9361_config.hdl

  // assign m_AD9361_InitParam.gpio_resetb to the arbitrarily defined GPIO_RESET_PIN so
  // that the No-OS opencpi platform driver knows to drive the force_reset
  // property of the sub-device
  m_AD9361_InitParam.gpio_resetb = GPIO_RESET_PIN;

  ad9361_bitstream_config_t ad9361_bitstream_config;
  const char* inst = m_app_inst_name_ad9361_data_sub.c_str();
  ad9361_bitstream_get_config(m_app, m_slave, inst, ad9361_bitstream_config);

  slave_ad9361_config_apply_config_to_AD9361_InitParam(ad9361_bitstream_config, m_AD9361_InitParam);

  m_AD9361_InitParam.reference_clk_rate = (uint32_t) round(m_AD9361_FREF_Hz);
  // printf("No-OS required rounding AD9361 reference clock rate from %0.15f to %" PRIu32, refclk, m_AD9361_InitParam.reference_clk_rate);

  // here is where we enforce the ad9361_config OWD comment
  // "[the force_two_r_two_t_timing] property is expected to correspond to the
  // D2 bit of the Parallel Port Configuration 1 register at SPI address 0x010
  m_slave.set_force_two_r_two_t_timing(m_AD9361_InitParam.two_t_two_r_timing_enable);

  struct ad9361_rf_phy** ad9361_phy = &m_ad9361_rf_phy;
  AD9361_InitParam* init_param = &m_AD9361_InitParam;

  m_ad9361_init_called = true;

  // ADI forum post recommended setting ENABLE/TXNRX pins high *prior to
  // ad9361_init() call* when
  // frequency_division_duplex_independent_mode_enable is set to 1

  m_slave.set_ENABLE_force_set(true);
  m_slave.set_TXNRX_force_set(true);

  // sleep duration chosen to be relatively small in relation to AD9361
  // initialization duration (which, through observation, appears to be
  // roughly 200 ms), but a long enough pulse that AD9361 is likely
  // recognizing it many, many times over
  usleep(1000);

  log_debug_ad9361_init(init_param);
  m_ad9361_init_ret = ad9361_init(ad9361_phy, init_param);

  m_slave.set_ENABLE_force_set(false);
  m_slave.set_TXNRX_force_set(false);

  if(m_ad9361_init_ret == -ENODEV) {
    throw "AD9361 initialization failed: SPI communication could not be established";
  }
  else if(m_ad9361_init_ret != 0) {
    throw "AD9361 initialization failed";
  }
  if(m_ad9361_rf_phy == 0) {
    std::string str;
    str += "AD9361 initialization failed:";
    str += "unknown failure resulted in null pointer";
    throw str.c_str();
  }

  enforce_ensm_config();

  //because channel config potentially changed
  slave_ad9361_config_set_FPGA_channel_config(m_slave);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::init_AD9361_InitParam() {

  AD9361_InitParam init_param = {
    /* Device selection */
    ID_AD9361,	// dev_sel
    /* Identification number */
    0,		//id_no
    /* Reference Clock */
    40000000UL,	//reference_clk_rate
    /* Base Configuration */
    1,		//two_rx_two_tx_mode_enable *** adi,2rx-2tx-mode-enable
    1,		//one_rx_one_tx_mode_use_rx_num *** adi,1rx-1tx-mode-use-rx-num
    1,		//one_rx_one_tx_mode_use_tx_num *** adi,1rx-1tx-mode-use-tx-num
    1,		//frequency_division_duplex_mode_enable *** adi,frequency-division-duplex-mode-enable

    // frequency_division_duplex_independent_mode_enable=1 required for
    // ad9361_dac.hdl event port to operate as intended
    1,		//frequency_division_duplex_independent_mode_enable *** adi,frequency-division-duplex-independent-mode-enable

    0,		//tdd_use_dual_synth_mode_enable *** adi,tdd-use-dual-synth-mode-enable
    0,		//tdd_skip_vco_cal_enable *** adi,tdd-skip-vco-cal-enable
    0,		//tx_fastlock_delay_ns *** adi,tx-fastlock-delay-ns
    0,		//rx_fastlock_delay_ns *** adi,rx-fastlock-delay-ns
    0,		//rx_fastlock_pincontrol_enable *** adi,rx-fastlock-pincontrol-enable
    0,		//tx_fastlock_pincontrol_enable *** adi,tx-fastlock-pincontrol-enable
    0,		//external_rx_lo_enable *** adi,external-rx-lo-enable
    0,		//external_tx_lo_enable *** adi,external-tx-lo-enable
    5,		//dc_offset_tracking_update_event_mask *** adi,dc-offset-tracking-update-event-mask
    6,		//dc_offset_attenuation_high_range *** adi,dc-offset-attenuation-high-range
    5,		//dc_offset_attenuation_low_range *** adi,dc-offset-attenuation-low-range
    0x28,	//dc_offset_count_high_range *** adi,dc-offset-count-high-range
    0x32,	//dc_offset_count_low_range *** adi,dc-offset-count-low-range
    0,		//split_gain_table_mode_enable *** adi,split-gain-table-mode-enable
    MAX_SYNTH_FREF,	//trx_synthesizer_target_fref_overwrite_hz *** adi,trx-synthesizer-target-fref-overwrite-hz
    0,		// qec_tracking_slow_mode_enable *** adi,qec-tracking-slow-mode-enable
    /* ENSM Control */
    0,		//ensm_enable_pin_pulse_mode_enable *** adi,ensm-enable-pin-pulse-mode-enable
    0,		//ensm_enable_txnrx_control_enable *** adi,ensm-enable-txnrx-control-enable
    /* LO Control */
    2400000000UL,	//rx_synthesizer_frequency_hz *** adi,rx-synthesizer-frequency-hz
    2400000000UL,	//tx_synthesizer_frequency_hz *** adi,tx-synthesizer-frequency-hz
    1,        //tx_lo_powerdown_managed_enable *** adi,tx-lo-powerdown-managed-enable
    /* Rate & BW Control */
    {983040000, 245760000, 122880000, 61440000, 30720000, 30720000},// rx_path_clock_frequencies[6] *** adi,rx-path-clock-frequencies
    {983040000, 122880000, 122880000, 61440000, 30720000, 30720000},// tx_path_clock_frequencies[6] *** adi,tx-path-clock-frequencies
    18000000,//rf_rx_bandwidth_hz *** adi,rf-rx-bandwidth-hz
    18000000,//rf_tx_bandwidth_hz *** adi,rf-tx-bandwidth-hz
    /* RF Port Control */
    0,		//rx_rf_port_input_select *** adi,rx-rf-port-input-select
    0,		//tx_rf_port_input_select *** adi,tx-rf-port-input-select
    /* TX Attenuation Control */
    10000,	//tx_attenuation_mdB *** adi,tx-attenuation-mdB
    0,		//update_tx_gain_in_alert_enable *** adi,update-tx-gain-in-alert-enable
    /* Reference Clock Control */
    0,		//xo_disable_use_ext_refclk_enable *** adi,xo-disable-use-ext-refclk-enable
    {8, 5920},	//dcxo_coarse_and_fine_tune[2] *** adi,dcxo-coarse-and-fine-tune
    CLKOUT_DISABLE,	//clk_output_mode_select *** adi,clk-output-mode-select
    /* Gain Control */
    2,		//gc_rx1_mode *** adi,gc-rx1-mode
    2,		//gc_rx2_mode *** adi,gc-rx2-mode
    58,		//gc_adc_large_overload_thresh *** adi,gc-adc-large-overload-thresh
    4,		//gc_adc_ovr_sample_size *** adi,gc-adc-ovr-sample-size
    47,		//gc_adc_small_overload_thresh *** adi,gc-adc-small-overload-thresh
    8192,	//gc_dec_pow_measurement_duration *** adi,gc-dec-pow-measurement-duration
    0,		//gc_dig_gain_enable *** adi,gc-dig-gain-enable
    800,	//gc_lmt_overload_high_thresh *** adi,gc-lmt-overload-high-thresh
    704,	//gc_lmt_overload_low_thresh *** adi,gc-lmt-overload-low-thresh
    24,		//gc_low_power_thresh *** adi,gc-low-power-thresh
    15,		//gc_max_dig_gain *** adi,gc-max-dig-gain
    /* Gain MGC Control */
    2,		//mgc_dec_gain_step *** adi,mgc-dec-gain-step
    2,		//mgc_inc_gain_step *** adi,mgc-inc-gain-step
    0,		//mgc_rx1_ctrl_inp_enable *** adi,mgc-rx1-ctrl-inp-enable
    0,		//mgc_rx2_ctrl_inp_enable *** adi,mgc-rx2-ctrl-inp-enable
    0,		//mgc_split_table_ctrl_inp_gain_mode *** adi,mgc-split-table-ctrl-inp-gain-mode
    /* Gain AGC Control */
    10,		//agc_adc_large_overload_exceed_counter *** adi,agc-adc-large-overload-exceed-counter
    2,		//agc_adc_large_overload_inc_steps *** adi,agc-adc-large-overload-inc-steps
    0,		//agc_adc_lmt_small_overload_prevent_gain_inc_enable *** adi,agc-adc-lmt-small-overload-prevent-gain-inc-enable
    10,		//agc_adc_small_overload_exceed_counter *** adi,agc-adc-small-overload-exceed-counter
    4,		//agc_dig_gain_step_size *** adi,agc-dig-gain-step-size
    3,		//agc_dig_saturation_exceed_counter *** adi,agc-dig-saturation-exceed-counter
    1000,	// agc_gain_update_interval_us *** adi,agc-gain-update-interval-us
    0,		//agc_immed_gain_change_if_large_adc_overload_enable *** adi,agc-immed-gain-change-if-large-adc-overload-enable
    0,		//agc_immed_gain_change_if_large_lmt_overload_enable *** adi,agc-immed-gain-change-if-large-lmt-overload-enable
    10,		//agc_inner_thresh_high *** adi,agc-inner-thresh-high
    1,		//agc_inner_thresh_high_dec_steps *** adi,agc-inner-thresh-high-dec-steps
    12,		//agc_inner_thresh_low *** adi,agc-inner-thresh-low
    1,		//agc_inner_thresh_low_inc_steps *** adi,agc-inner-thresh-low-inc-steps
    10,		//agc_lmt_overload_large_exceed_counter *** adi,agc-lmt-overload-large-exceed-counter
    2,		//agc_lmt_overload_large_inc_steps *** adi,agc-lmt-overload-large-inc-steps
    10,		//agc_lmt_overload_small_exceed_counter *** adi,agc-lmt-overload-small-exceed-counter
    5,		//agc_outer_thresh_high *** adi,agc-outer-thresh-high
    2,		//agc_outer_thresh_high_dec_steps *** adi,agc-outer-thresh-high-dec-steps
    18,		//agc_outer_thresh_low *** adi,agc-outer-thresh-low
    2,		//agc_outer_thresh_low_inc_steps *** adi,agc-outer-thresh-low-inc-steps
    1,		//agc_attack_delay_extra_margin_us; *** adi,agc-attack-delay-extra-margin-us
    0,		//agc_sync_for_gain_counter_enable *** adi,agc-sync-for-gain-counter-enable
    /* Fast AGC */
    64,		//fagc_dec_pow_measuremnt_duration ***  adi,fagc-dec-pow-measurement-duration
    260,	//fagc_state_wait_time_ns ***  adi,fagc-state-wait-time-ns
    /* Fast AGC - Low Power */
    0,		//fagc_allow_agc_gain_increase ***  adi,fagc-allow-agc-gain-increase-enable
    5,		//fagc_lp_thresh_increment_time ***  adi,fagc-lp-thresh-increment-time
    1,		//fagc_lp_thresh_increment_steps ***  adi,fagc-lp-thresh-increment-steps
    /* Fast AGC - Lock Level (Lock Level is set via slow AGC inner high threshold) */
    1,		//fagc_lock_level_lmt_gain_increase_en ***  adi,fagc-lock-level-lmt-gain-increase-enable
    5,		//fagc_lock_level_gain_increase_upper_limit ***  adi,fagc-lock-level-gain-increase-upper-limit
    /* Fast AGC - Peak Detectors and Final Settling */
    1,		//fagc_lpf_final_settling_steps ***  adi,fagc-lpf-final-settling-steps
    1,		//fagc_lmt_final_settling_steps ***  adi,fagc-lmt-final-settling-steps
    3,		//fagc_final_overrange_count ***  adi,fagc-final-overrange-count
    /* Fast AGC - Final Power Test */
    0,		//fagc_gain_increase_after_gain_lock_en ***  adi,fagc-gain-increase-after-gain-lock-enable
    /* Fast AGC - Unlocking the Gain */
    0,		//fagc_gain_index_type_after_exit_rx_mode ***  adi,fagc-gain-index-type-after-exit-rx-mode
    1,		//fagc_use_last_lock_level_for_set_gain_en ***  adi,fagc-use-last-lock-level-for-set-gain-enable
    1,		//fagc_rst_gla_stronger_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-stronger-sig-thresh-exceeded-enable
    5,		//fagc_optimized_gain_offset ***  adi,fagc-optimized-gain-offset
    10,		//fagc_rst_gla_stronger_sig_thresh_above_ll ***  adi,fagc-rst-gla-stronger-sig-thresh-above-ll
    1,		//fagc_rst_gla_engergy_lost_sig_thresh_exceeded_en ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-exceeded-enable
    1,		//fagc_rst_gla_engergy_lost_goto_optim_gain_en ***  adi,fagc-rst-gla-engergy-lost-goto-optim-gain-enable
    10,		//fagc_rst_gla_engergy_lost_sig_thresh_below_ll ***  adi,fagc-rst-gla-engergy-lost-sig-thresh-below-ll
    8,		//fagc_energy_lost_stronger_sig_gain_lock_exit_cnt ***  adi,fagc-energy-lost-stronger-sig-gain-lock-exit-cnt
    1,		//fagc_rst_gla_large_adc_overload_en ***  adi,fagc-rst-gla-large-adc-overload-enable
    1,		//fagc_rst_gla_large_lmt_overload_en ***  adi,fagc-rst-gla-large-lmt-overload-enable
    0,		//fagc_rst_gla_en_agc_pulled_high_en ***  adi,fagc-rst-gla-en-agc-pulled-high-enable
    0,		//fagc_rst_gla_if_en_agc_pulled_high_mode ***  adi,fagc-rst-gla-if-en-agc-pulled-high-mode
    64,		//fagc_power_measurement_duration_in_state5 ***  adi,fagc-power-measurement-duration-in-state5
    /* RSSI Control */
    1,		//rssi_delay *** adi,rssi-delay
    1000,	//rssi_duration *** adi,rssi-duration
    3,		//rssi_restart_mode *** adi,rssi-restart-mode
    0,		//rssi_unit_is_rx_samples_enable *** adi,rssi-unit-is-rx-samples-enable
    1,		//rssi_wait *** adi,rssi-wait
    /* Aux ADC Control */
    256,	//aux_adc_decimation *** adi,aux-adc-decimation
    40000000UL,	//aux_adc_rate *** adi,aux-adc-rate
    /* AuxDAC Control */
    1,		//aux_dac_manual_mode_enable ***  adi,aux-dac-manual-mode-enable
    0,		//aux_dac1_default_value_mV ***  adi,aux-dac1-default-value-mV
    0,		//aux_dac1_active_in_rx_enable ***  adi,aux-dac1-active-in-rx-enable
    0,		//aux_dac1_active_in_tx_enable ***  adi,aux-dac1-active-in-tx-enable
    0,		//aux_dac1_active_in_alert_enable ***  adi,aux-dac1-active-in-alert-enable
    0,		//aux_dac1_rx_delay_us ***  adi,aux-dac1-rx-delay-us
    0,		//aux_dac1_tx_delay_us ***  adi,aux-dac1-tx-delay-us
    0,		//aux_dac2_default_value_mV ***  adi,aux-dac2-default-value-mV
    0,		//aux_dac2_active_in_rx_enable ***  adi,aux-dac2-active-in-rx-enable
    0,		//aux_dac2_active_in_tx_enable ***  adi,aux-dac2-active-in-tx-enable
    0,		//aux_dac2_active_in_alert_enable ***  adi,aux-dac2-active-in-alert-enable
    0,		//aux_dac2_rx_delay_us ***  adi,aux-dac2-rx-delay-us
    0,		//aux_dac2_tx_delay_us ***  adi,aux-dac2-tx-delay-us
    /* Temperature Sensor Control */
    256,	//temp_sense_decimation *** adi,temp-sense-decimation
    1000,	//temp_sense_measurement_interval_ms *** adi,temp-sense-measurement-interval-ms
    (int8_t)0xCE,	//temp_sense_offset_signed *** adi,temp-sense-offset-signed //0xCE,	//temp_sense_offset_signed *** adi,temp-sense-offset-signed
    1,		//temp_sense_periodic_measurement_enable *** adi,temp-sense-periodic-measurement-enable
    /* Control Out Setup */
    0xFF,	//ctrl_outs_enable_mask *** adi,ctrl-outs-enable-mask
    0,		//ctrl_outs_index *** adi,ctrl-outs-index
    /* External LNA Control */
    0,		//elna_settling_delay_ns *** adi,elna-settling-delay-ns
    0,		//elna_gain_mdB *** adi,elna-gain-mdB
    0,		//elna_bypass_loss_mdB *** adi,elna-bypass-loss-mdB
    0,		//elna_rx1_gpo0_control_enable *** adi,elna-rx1-gpo0-control-enable
    0,		//elna_rx2_gpo1_control_enable *** adi,elna-rx2-gpo1-control-enable
    0,		//elna_gaintable_all_index_enable *** adi,elna-gaintable-all-index-enable
    /* Digital Interface Control */
    0,		//digital_interface_tune_skip_mode *** adi,digital-interface-tune-skip-mode
    0,		//digital_interface_tune_fir_disable *** adi,digital-interface-tune-fir-disable
    1,		//pp_tx_swap_enable *** adi,pp-tx-swap-enable
    1,		//pp_rx_swap_enable *** adi,pp-rx-swap-enable
    0,		//tx_channel_swap_enable *** adi,tx-channel-swap-enable
    0,		//rx_channel_swap_enable *** adi,rx-channel-swap-enable
    1,		//rx_frame_pulse_mode_enable *** adi,rx-frame-pulse-mode-enable
    0,		//two_t_two_r_timing_enable *** adi,2t2r-timing-enable
    0,		//invert_data_bus_enable *** adi,invert-data-bus-enable
    0,		//invert_data_clk_enable *** adi,invert-data-clk-enable
    0,		//fdd_alt_word_order_enable *** adi,fdd-alt-word-order-enable
    0,		//invert_rx_frame_enable *** adi,invert-rx-frame-enable
    0,		//fdd_rx_rate_2tx_enable *** adi,fdd-rx-rate-2tx-enable
    0,		//swap_ports_enable *** adi,swap-ports-enable
    0,		//single_data_rate_enable *** adi,single-data-rate-enable
    1,		//lvds_mode_enable *** adi,lvds-mode-enable
    0,		//half_duplex_mode_enable *** adi,half-duplex-mode-enable
    0,		//single_port_mode_enable *** adi,single-port-mode-enable
    0,		//full_port_enable *** adi,full-port-enable
    0,		//full_duplex_swap_bits_enable *** adi,full-duplex-swap-bits-enable
    0,		//delay_rx_data *** adi,delay-rx-data
    0,		//rx_data_clock_delay *** adi,rx-data-clock-delay
    4,		//rx_data_delay *** adi,rx-data-delay
    7,		//tx_fb_clock_delay *** adi,tx-fb-clock-delay
    0,		//tx_data_delay *** adi,tx-data-delay
  #ifdef ALTERA_PLATFORM
    300,	//lvds_bias_mV *** adi,lvds-bias-mV
  #else
    150,	//lvds_bias_mV *** adi,lvds-bias-mV
  #endif
    1,		//lvds_rx_onchip_termination_enable *** adi,lvds-rx-onchip-termination-enable
    0,		//rx1rx2_phase_inversion_en *** adi,rx1-rx2-phase-inversion-enable
    0xFF,	//lvds_invert1_control *** adi,lvds-invert1-control
    0x0F,	//lvds_invert2_control *** adi,lvds-invert2-control
    /* GPO Control */
    0,		//gpo0_inactive_state_high_enable *** adi,gpo0-inactive-state-high-enable
    0,		//gpo1_inactive_state_high_enable *** adi,gpo1-inactive-state-high-enable
    0,		//gpo2_inactive_state_high_enable *** adi,gpo2-inactive-state-high-enable
    0,		//gpo3_inactive_state_high_enable *** adi,gpo3-inactive-state-high-enable
    0,		//gpo0_slave_rx_enable *** adi,gpo0-slave-rx-enable
    0,		//gpo0_slave_tx_enable *** adi,gpo0-slave-tx-enable
    0,		//gpo1_slave_rx_enable *** adi,gpo1-slave-rx-enable
    0,		//gpo1_slave_tx_enable *** adi,gpo1-slave-tx-enable
    0,		//gpo2_slave_rx_enable *** adi,gpo2-slave-rx-enable
    0,		//gpo2_slave_tx_enable *** adi,gpo2-slave-tx-enable
    0,		//gpo3_slave_rx_enable *** adi,gpo3-slave-rx-enable
    0,		//gpo3_slave_tx_enable *** adi,gpo3-slave-tx-enable
    0,		//gpo0_rx_delay_us *** adi,gpo0-rx-delay-us
    0,		//gpo0_tx_delay_us *** adi,gpo0-tx-delay-us
    0,		//gpo1_rx_delay_us *** adi,gpo1-rx-delay-us
    0,		//gpo1_tx_delay_us *** adi,gpo1-tx-delay-us
    0,		//gpo2_rx_delay_us *** adi,gpo2-rx-delay-us
    0,		//gpo2_tx_delay_us *** adi,gpo2-tx-delay-us
    0,		//gpo3_rx_delay_us *** adi,gpo3-rx-delay-us
    0,		//gpo3_tx_delay_us *** adi,gpo3-tx-delay-us
    /* Tx Monitor Control */
    37000,	//low_high_gain_threshold_mdB *** adi,txmon-low-high-thresh
    0,		//low_gain_dB *** adi,txmon-low-gain
    24,		//high_gain_dB *** adi,txmon-high-gain
    0,		//tx_mon_track_en *** adi,txmon-dc-tracking-enable
    0,		//one_shot_mode_en *** adi,txmon-one-shot-mode-enable
    511,	//tx_mon_delay *** adi,txmon-delay
    8192,	//tx_mon_duration *** adi,txmon-duration
    2,		//tx1_mon_front_end_gain *** adi,txmon-1-front-end-gain
    2,		//tx2_mon_front_end_gain *** adi,txmon-2-front-end-gain
    48,		//tx1_mon_lo_cm *** adi,txmon-1-lo-cm
    48,		//tx2_mon_lo_cm *** adi,txmon-2-lo-cm
    /* GPIO definitions */
    -1,		//gpio_resetb *** reset-gpios
    /* MCS Sync */
    -1,		//gpio_sync *** sync-gpios
    -1,		//gpio_cal_sw1 *** cal-sw1-gpios
    -1,		//gpio_cal_sw2 *** cal-sw2-gpios
    /* External LO clocks */
    NULL,	//(*ad9361_rfpll_ext_recalc_rate)()
    NULL,	//(*ad9361_rfpll_ext_round_rate)()
    NULL	//(*ad9361_rfpll_ext_set_rate)()
  };
  m_AD9361_InitParam = init_param;
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::enforce_ensm_config() {

  m_slave.set_Half_Duplex_Mode(not m_ad9361_rf_phy->pdata->fdd);

  uint8_t ensm_config_1 = m_slave.get_ensm_config_1();
  m_slave.set_ENSM_Pin_Control((ensm_config_1 & 0x10) == 0x10);
  m_slave.set_Level_Mode((ensm_config_1 & 0x08) == 0x08);

  uint8_t ensm_config_2 = m_slave.get_ensm_config_2();
  m_slave.set_FDD_External_Control_Enable((ensm_config_2 & 0x80) == 0x80);
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::configurator_check_and_reinit(
    const ConfigLockRequest& config_lock_request,
    C                        configurator_copy) {

  /*if(m_configurator_tune_resamp_locked) {
    log_debug("skipping locking tune resamp");
  }
  else {
    log_debug("not skipping locking tune resamp");
    m_configurator_tune_resamp_locked = true;
  }*/

  //configurator_copy.log_all_possible_config_values();

  auto it = config_lock_request.m_data_streams.begin(); 

  if(it == config_lock_request.m_data_streams.end()) {
    log_debug("config lock request was erroneously empty");
    return false;
  }

  bool reql_RX1A = false; // config_lock_request requires locking SMA RX1A
  bool reql_RX2A = false; // config_lock_request requires locking SMA RX2A
  bool reql_TX1A = false; // config_lock_request requires locking SMA TX1A
  bool reql_TX2A = false; // config_lock_request requires locking SMA TX2A

  for(; it != config_lock_request.m_data_streams.end(); it++) {

    throw_if_data_stream_lock_request_malformed(*it);

    if(not it->get_including_routing_ID()) {
      throw std::string("radio controller's data stream config lock request malformed: did not include routing_ID");
    }

    std::vector<data_stream_ID_t> data_streams;

    if(it->get_including_data_stream_ID()) {
      data_streams.push_back(it->get_data_stream_ID());
    }
    else { // assuming request by type only
      configurator_copy.find_data_streams_of_type(it->get_data_stream_type(), data_streams);

      if(data_streams.empty()) {
        // configurator_copy did not have any data streams of the requested data
        // stream type
        return false;
      }
    }

    bool found_lock = false;
    // IDs of data streams what have the potential to meet the
    // requirements of the current data stream config lock request
    auto potential_ds_IDs = data_streams.begin();

    for(; potential_ds_IDs != data_streams.end(); potential_ds_IDs++) {

      config_key_t key;
      config_value_t val, tol;
      bool v;
      std::string inst;

      key = "tuning_freq_complex_mixer_MHz";
      val = 0;
      tol = 0;
      found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, val, tol);
      if(not found_lock) {
        this->log_info_config_lock(false, false, *potential_ds_IDs,val,tol,key,"MHz");
        goto unrollandcontinue;
      }

      if((*potential_ds_IDs == "SMA_TX1A") or (*potential_ds_IDs == "SMA_TX2A")) {
        inst.assign(get_inst_name_cic_int(*it));
        key = "CIC_int_interpolation_factor";
      }
      else if((*potential_ds_IDs == "SMA_RX1A") or (*potential_ds_IDs == "SMA_RX2A")) {
        inst.assign(get_inst_name_cic_dec(*it));
        key = "CIC_dec_decimation_factor";
      }
      val = inst.empty() ? 1 : m_app.getPropertyValue<OA::UShort>(inst, "R");
      tol = 0;
      found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, val, tol);
      if(not found_lock) {
        this->log_info_config_lock(false, false, *potential_ds_IDs,val,tol,key,"");
        goto unrollandcontinue;
      }

      key = config_key_tuning_freq_MHz;
      val = it->get_tuning_freq_MHz();
      tol = it->get_tolerance_tuning_freq_MHz();
      found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, val, tol);
      if(not found_lock) {
        this->log_info_config_lock(false, false, *potential_ds_IDs,val,tol,key,"MHz");
        goto unrollandcontinue;
      }

      key = config_key_bandwidth_3dB_MHz;
      val = it->get_bandwidth_3dB_MHz();
      tol = it->get_tolerance_bandwidth_3dB_MHz();
      found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, val, tol);
      if(not found_lock) {
        this->log_info_config_lock(false, false, *potential_ds_IDs,val,tol,key,"MHz");
        goto unrollandcontinue;
      }

      key = config_key_sampling_rate_Msps;
      val = it->get_sampling_rate_Msps();
      tol = it->get_tolerance_sampling_rate_Msps();
      found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, val, tol);
      if(not found_lock) {
        this->log_info_config_lock(false, false, *potential_ds_IDs,val,tol,key,"Msps");
        goto unrollandcontinue;
      }

      key = config_key_samples_are_complex;
      v = it->get_samples_are_complex();
      found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, v);
      if(not found_lock) {
        this->log_info_config_lock(false, *potential_ds_IDs,val,key,&v);
        goto unrollandcontinue;
      }

      if(it->get_including_gain_mode()) {
        key = config_key_gain_mode;

        config_value_t _auto = 0;
        config_value_t manual= 1;
        if((it->get_gain_mode().compare("manual") == 0) or
           (it->get_gain_mode().compare("RF_GAIN_MGC") == 0)) {
          found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, manual);
        }
        else if((it->get_gain_mode().compare("auto") == 0) or
                (it->get_gain_mode().compare("RF_GAIN_SLOWATTACK_AGC") == 0) or
                (it->get_gain_mode().compare("RF_GAIN_FASTATTACK_AGC") == 0) or
                (it->get_gain_mode().compare("RF_GAIN_HYBRID_AGC") == 0)) {
          found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, _auto);
        }

        if(not found_lock) {
          this->log_info_config_lock(false, *potential_ds_IDs,it->get_gain_mode(),key);
          goto unrollandcontinue;
        }
      }

      if(it->get_including_gain_dB()) {
        key = config_key_gain_dB;
        val = it->get_gain_dB();
        tol = it->get_tolerance_gain_dB();
        found_lock |= configurator_copy.lock_config(*potential_ds_IDs, key, val, tol);
        if(not found_lock) {
          this->log_info_config_lock(false, false, *potential_ds_IDs,val,tol,key,"dB");
          goto unrollandcontinue;
        }
      }

      if(*potential_ds_IDs == "SMA_RX1A") {
        reql_RX1A = true;
      }
      else if(*potential_ds_IDs == "SMA_RX2A") {
        reql_RX2A = true;
      }
      else if(*potential_ds_IDs == "SMA_TX1A") {
        reql_TX1A = true;
      }
      else if(*potential_ds_IDs == "SMA_TX2A") {
        reql_TX2A = true;
      }

      break;
      unrollandcontinue:
      configurator_copy.unlock_config(*potential_ds_IDs, "tuning_freq_complex_mixer_MHz");
      if((*potential_ds_IDs == "SMA_TX1A") or (*potential_ds_IDs == "SMA_TX2A")) {
        configurator_copy.unlock_config(*potential_ds_IDs, "CIC_int_interpolation_factor");
      }
      else if((*potential_ds_IDs == "SMA_RX1A") or (*potential_ds_IDs == "SMA_RX2A")) {
        configurator_copy.unlock_config(*potential_ds_IDs, "CIC_dec_decimation_factor");
      }
      configurator_copy.unlock_config(*potential_ds_IDs, config_key_tuning_freq_MHz);
      configurator_copy.unlock_config(*potential_ds_IDs, config_key_bandwidth_3dB_MHz);
      configurator_copy.unlock_config(*potential_ds_IDs, config_key_sampling_rate_Msps);
      configurator_copy.unlock_config(*potential_ds_IDs, config_key_samples_are_complex);
      configurator_copy.unlock_config(*potential_ds_IDs, config_key_gain_mode);
      configurator_copy.unlock_config(*potential_ds_IDs, config_key_gain_dB);
    }

    if(not found_lock) {
      log_debug("configurator will not allow lock request\n");
      return false;
    }
  }

  bool s; // Boolean indication of whether any initialization procedure failed
          //(which may occur depending on current config locks)
  s = reinit_AD9361_if_required(reql_RX1A,reql_RX2A,reql_TX1A,reql_TX2A,configurator_copy);
  if(s) {
    log_debug("configurator will allow lock request\n");
  }
  return s;
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::reinit_AD9361_if_required(
  const bool reql_RX1A, const bool reql_RX2A,
  const bool reql_TX1A, const bool reql_TX2A,
  const C& configurator_copy) {
  log_debug("reql_RX1A=%s,reql_RX2A=%s,reql_TX1A=%s,reql_TX2A=%s",
 (reql_RX1A ? "t" : "f"),
 (reql_RX2A ? "t" : "f"),
 (reql_TX1A ? "t" : "f"),
 (reql_TX2A ? "t" : "f"));

  if((!reql_RX1A) and (!reql_RX2A) and (!reql_TX1A) and (!reql_TX2A)) {
    // this function should never be called if we end up here, but we handle it
    // to be robust
    return true;
  }
  else {
    // indicator that previous ad9361_init() was called w/ values that are
    // incompatible with the pending config lock request, thus requiring a
    // "re"-initialization
    bool requires_reinit = false;

    // values required by pending config lock request
    bool two_rx_two_tx;
    uint8_t use_rx_num;
    uint8_t use_tx_num;
    uint8_t fdd_rx_rate_2tx_enable;

    if((reql_RX1A and reql_RX2A) or (reql_TX1A and reql_TX2A)) {
      two_rx_two_tx = true;
      use_rx_num = 1; // don't care, just set to 1
      use_tx_num = 1; // don't care, just set to 1
    }
    else {
      two_rx_two_tx = false;
      use_rx_num = reql_RX1A ? 1 : 2;
      use_tx_num = reql_TX1A ? 1 : 2;
    }

    if(m_ad9361_init_called) {
      if(two_rx_two_tx != m_AD9361_InitParam.two_rx_two_tx_mode_enable) {
        log_debug("requires_reinit two_rx_two_tx");
        requires_reinit = true;
      }
      if(not two_rx_two_tx) {
        if(reql_RX1A or reql_RX2A) {
          if(use_rx_num != m_AD9361_InitParam.one_rx_one_tx_mode_use_rx_num) {
            log_debug("requires_reinit use_rx_num");
            requires_reinit = true;
          }
        }
        if(reql_TX1A or reql_TX2A) {
          if(use_tx_num != m_AD9361_InitParam.one_rx_one_tx_mode_use_tx_num) {
            log_debug("requires_reinit use_tx_num");
            requires_reinit = true;
          }
        }
      }
    }

    auto copycopy = configurator_copy; // const necessitates this...
    const ConfigValueRanges& x = copycopy.get_ranges_possible("DAC_Clk_divider");
    fdd_rx_rate_2tx_enable = (x.get_smallest_min() < 1.5) ? 0 : 1;

    if(m_ad9361_init_called) {
      config_value_t DAC_Clk_divider;
      DAC_Clk_divider = (m_AD9361_InitParam.fdd_rx_rate_2tx_enable == 1) ? 2 :1;
      if(!x.is_valid(DAC_Clk_divider)) {
        log_debug("requires_reinit fdd_rx_rate_2tx_enable");
        requires_reinit = true;
      }
    }

    if((not m_ad9361_init_called) or requires_reinit) {
      if(requires_reinit) {
        if(any_configurator_configs_locked_which_prevent_ad9361_init()) {
          log_debug("reinit required but configurator configs locked which prevent reinit");
          return false;
        }
        log_info("re-initializing AD9361");
      }
      // prepare m_AD9361_InitParam and perform AD9361 initialization
      m_AD9361_InitParam.two_rx_two_tx_mode_enable = two_rx_two_tx;
      m_AD9361_InitParam.one_rx_one_tx_mode_use_rx_num = use_rx_num;
      m_AD9361_InitParam.one_rx_one_tx_mode_use_tx_num = use_tx_num;
      m_AD9361_InitParam.fdd_rx_rate_2tx_enable = fdd_rx_rate_2tx_enable;
      init();
    }
  }

  return true;
}

template<class C, class S>
bool RadioCtrlrNoOSTuneResamp<C, S>::any_configurator_configs_locked_which_prevent_ad9361_init() const {

  std::vector<data_stream_ID_t> data_streams;
  data_streams.push_back(std::string("SMA_RX1A"));
  data_streams.push_back(std::string("SMA_RX2A"));
  data_streams.push_back(std::string("SMA_TX1A"));
  data_streams.push_back(std::string("SMA_TX2A"));

  // unfortunately necessary to make this function const
  auto configurator_copy = m_configurator;

  auto it = data_streams.begin();
  for(; it != data_streams.end(); it++) {
    if(configurator_copy.get_config_is_locked(*it, config_key_tuning_freq_MHz)) {
      return true;
    }
    if(configurator_copy.get_config_is_locked(*it, config_key_bandwidth_3dB_MHz)) {
      return true;
    }
    if(configurator_copy.get_config_is_locked(*it, config_key_sampling_rate_Msps)) {
      return true;
    }
    if(configurator_copy.get_config_is_locked(*it, config_key_gain_mode)) {
      return true;
    }
    if(configurator_copy.get_config_is_locked(*it, config_key_gain_dB)) {
      return true;
    }
  }
  return false;
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::throw_if_ad9361_init_failed(
    const char* operation) const {

  if(m_ad9361_init_ret != 0) {
    std::ostringstream oss;
    oss << "cannot perform ";
    oss << (operation ? operation : "operation");
    oss << " because ad9361_init() failed";
    throw oss.str();
  }
}

template<class C, class S>
std::vector<gain_mode_value_t>
RadioCtrlrNoOSTuneResamp<C, S>::get_ranges_possible_gain_mode(
    const data_stream_ID_t data_stream_ID) const {

  std::vector<gain_mode_value_t> ret;
  auto key = config_key_gain_mode;
  auto configurator_copy = m_configurator;
  auto vr = configurator_copy.get_ranges_possible(data_stream_ID, key);

  if(vr.is_valid(0)) {
    // auto is generic value (which corresponds to AD9361-specific
    // RF_GAIN_SLOWATTACK_AGC)
    ret.push_back("auto");
  }
  if(vr.is_valid(1)) {
    // auto is generic value (which corresponds to AD9361-specific
    // RF_GAIN_MGC)
    ret.push_back("manual");
  }
  bool is_locked = vr.is_valid(0) xor vr.is_valid(1); // this is an assumption

  if(not is_locked) {
    if((data_stream_ID == "SMA_RX1A") or (data_stream_ID == "SMA_RX2A")) {
      // AD9361/No-OS-specific values
      ret.push_back("RF_GAIN_FASTATTACK_AGC");
      ret.push_back("RF_GAIN_SLOWATTACK_AGC");
      ret.push_back("RF_GAIN_HYBRID_AGC");
    }
  }

  return ret;
}

template<class C, class S>
std::string RadioCtrlrNoOSTuneResamp<C, S>::get_inst_name_complex_mixer(
    const DataStreamConfigLockRequest& req) const {

  throw_if_data_stream_lock_request_malformed(req);

  std::string inst;
  if(req.get_routing_ID() == "RX0") {
    inst.assign(m_app_inst_name_RX0_complex_mixer);
  }
  else if(req.get_routing_ID() == "RX1") {
    inst.assign(m_app_inst_name_RX1_complex_mixer);
  }
  else if(req.get_routing_ID() == "TX0") {
    inst.assign(m_app_inst_name_TX0_complex_mixer);
  }
  else { // (req.get_routing_ID() == "TX1")
    inst.assign(m_app_inst_name_TX1_complex_mixer);
  }
  return inst;
}

template<class C, class S>
std::string RadioCtrlrNoOSTuneResamp<C, S>::get_inst_name_cic_int(
    const DataStreamConfigLockRequest& req) const {

  throw_if_data_stream_lock_request_malformed(req);

  std::string inst;
  if(req.get_routing_ID() == "TX0") {
    inst.assign(m_app_inst_name_TX0_cic_int);
  }
  else if(req.get_routing_ID() == "TX1") {
    inst.assign(m_app_inst_name_TX1_cic_int);
  }
  else {
    std::ostringstream oss;
    oss << "attempted to access cic_int app inst name for bad routing ID of ";
    oss << req.get_routing_ID();
    throw oss.str();
  }
  return inst;
}

template<class C, class S>
std::string RadioCtrlrNoOSTuneResamp<C, S>::get_inst_name_cic_dec(
    const DataStreamConfigLockRequest& req) const {

  throw_if_data_stream_lock_request_malformed(req);

  std::string inst;
  if(req.get_routing_ID() == "RX0") {
    inst.assign(m_app_inst_name_RX0_cic_dec);
  }
  else if(req.get_routing_ID() == "RX1") {
    inst.assign(m_app_inst_name_RX1_cic_dec);
  }
  else {
    std::ostringstream oss;
    oss << "attempted to access cic_int app inst name for bad routing ID of ";
    oss << req.get_routing_ID();
    throw oss.str();
  }
  return inst;
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_TX0_qdac(const char* val) {
  m_app_inst_name_TX0_qdac.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_TX0_complex_mixer(const char* val) {
  m_app_inst_name_TX0_complex_mixer.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_TX0_cic_int(const char* val) {
  m_app_inst_name_TX0_cic_int.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_TX1_qdac(const char* val) {
  m_app_inst_name_TX1_qdac.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_TX1_complex_mixer(const char* val) {
  m_app_inst_name_TX1_complex_mixer.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_TX1_cic_int(const char* val) {
  m_app_inst_name_TX1_cic_int.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_RX0_qadc(const char* val) {
  m_app_inst_name_RX0_qadc.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_RX0_complex_mixer(const char* val) {
  m_app_inst_name_RX0_complex_mixer.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_RX0_cic_dec(const char* val) {
  m_app_inst_name_RX0_cic_dec.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_RX1_qadc(const char* val) {
  m_app_inst_name_RX1_qadc.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_RX1_complex_mixer(const char* val) {
  m_app_inst_name_RX1_complex_mixer.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_RX1_cic_dec(const char* val) {
  m_app_inst_name_RX1_cic_dec.assign(val);
}

template<class C, class S>
void RadioCtrlrNoOSTuneResamp<C, S>::set_app_inst_name_ad9361_data_sub(const char* val) {
  m_app_inst_name_ad9361_data_sub.assign(val);
}

} // namespace RadioCtrlr

} // namespace OCPIProjects
