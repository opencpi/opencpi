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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_NO_OS_TUNE_RESAMP_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_NO_OS_TUNE_RESAMP_HH

#include <cstdint>         // uint8_t
#include "RadioCtrlr.hh"   // DigRadioCtrlr
#include "LogForwarder.hh"

// needed to use ADI No-OS library (via the OpenCPI ad9361 prerequisite)
extern "C" {
#include "config.h"     // from No-OS
#include "ad9361_api.h" // from No-OS
#include "ad9361.h"     // from No-OS, ad9361_bist...() functions
#include "no-OS/ad9361/sw/platform_opencpi/parameters.h"
}

// needed to use ADI No-OS library (via the OpenCPI ad9361 prerequisite)
extern "C" {
int32_t spi_init(OCPI::RCC::RCCUserSlave* _slave);
}

namespace OA = OCPI::API;

namespace OCPIProjects {

namespace RadioCtrlr {

/// @brief controls AD9361 via an OpenCPI device proxy's slave
template<class configurator_t, class slave_t>
class RadioCtrlrNoOSTuneResamp :
    public DigRadioCtrlr<OCPI_log_func_args_t, configurator_t> {

protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::m_config_locks;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::do_min_data_stream_config_locks;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::throw_if_data_stream_lock_request_malformed;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::log_info;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::log_debug;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::log_trace;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::log_warn;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::log_error;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_forwarding_callback_log_info;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_forwarding_callback_log_debug;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_forwarding_callback_log_trace;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_forwarding_callback_log_warn;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_forwarding_callback_log_error;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_arg0_log_info;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_arg0_log_debug;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_arg0_log_warn;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_arg0_log_error;
protected : using DigRadioCtrlr<OCPI_log_func_args_t, configurator_t>::set_arg0_log_trace;

protected : configurator_t m_configurator;

/// @brief No-OS struct pointer which is only used if managing No-OS internally
protected : struct ad9361_rf_phy* _ad9361_rf_phy;

/// @brief No-OS struct pointer (used in all scenarios)
protected : struct ad9361_rf_phy*& m_ad9361_rf_phy;

/// @brief reference to OpenCPI RCC slave object
protected : slave_t& m_slave;

protected : const double m_AD9361_FREF_Hz;

protected : int32_t m_ad9361_init_ret;

protected : bool m_ad9361_init_called;

/// @brief No-OS struct used to initialize AD9361
protected : AD9361_InitParam m_AD9361_InitParam;

protected : OA::Application& m_app;

protected : bool m_configurator_tune_resamp_locked;

protected : bool m_readback_gain_mode_as_standard_value;

protected : std::string m_app_inst_name_TX0_qdac;
protected : std::string m_app_inst_name_TX0_complex_mixer;
protected : std::string m_app_inst_name_TX0_cic_int;
protected : std::string m_app_inst_name_TX1_qdac;
protected : std::string m_app_inst_name_TX1_complex_mixer;
protected : std::string m_app_inst_name_TX1_cic_int;
protected : std::string m_app_inst_name_RX0_qadc;
protected : std::string m_app_inst_name_RX0_complex_mixer;
protected : std::string m_app_inst_name_RX0_cic_dec;
protected : std::string m_app_inst_name_RX1_qadc;
protected : std::string m_app_inst_name_RX1_complex_mixer;
protected : std::string m_app_inst_name_RX1_cic_dec;
protected : std::string m_app_inst_name_ad9361_data_sub;

/*! @brief Managing No-OS internally (this class instanced somewhere that
 *         doesn't need to use No-OS).
 ******************************************************************************/
public    : RadioCtrlrNoOSTuneResamp(const char* descriptor, slave_t& slave,
                OA::Application& app);

/*! @brief Managing No-OS externally (this class instanced somewhere that
 *         doesn't need to use No-OS). ad9361_rf_phy must be freed
 *         (using ad9361_free) outside of this class!!
 ******************************************************************************/
public    : RadioCtrlrNoOSTuneResamp(const char* descriptor, slave_t& slave,
                OA::Application& app, struct ad9361_rf_phy*& ad9361_rf_phy);

public    : void register_OpenCPI_logging_API(); /// @todo / FIXME comment back in

public    : virtual bool request_config_lock(
                config_lock_ID_t         config_lock_ID,
                const ConfigLockRequest& config_lock_request);

/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] inst  Application instance name of complex_mixer worker which
 *                   is associated with the data stream ID (*THIS* is why
 *                   we have a routing ID).
 *  @param[in] val   Desired value to lock to.
 *  @param[in] tol   If the configurator lock succeeds and the read back
 *                   on-hardware value is within val +/- tol, the lock will be
 *                   considered successful.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : bool lock_tuning_freq_complex_mixer_MHz(data_stream_ID_t   ds_ID,
                                                    const std::string& inst,
                                                    config_value_t     val,
                                                    config_value_t     tol);

/// @brief Unlocks configurator lock (no hardware action is performed).
protected : void unlock_tuning_freq_complex_mixer_MHz(data_stream_ID_t ds_ID);

public    : void throw_if_data_stream_lock_request_malformed(
                const DataStreamConfigLockRequest&
                data_stream_config_lock_request) const;

/*! @brief Performs the minimum config locks required per data stream
 *         for a AnaRadioCtrlr.
 ******************************************************************************/
protected : virtual bool do_min_data_stream_config_locks(
                data_stream_ID_t ds_ID, const DataStreamConfigLockRequest& req);

/*! @brief Measure value as it exists on hardware. Exception will be throw
 *         if hardware access fails or if invalid data stream ID is
 *         requested.
 ******************************************************************************/
public    : Meas<config_value_t> get_tuning_freq_MHz(
                data_stream_ID_t data_stream_ID) const;
/*! @brief Measure value as it exists on hardware. Exception will be throw
 *         if hardware access fails or if invalid data stream ID is
 *         requested.
 ******************************************************************************/
public    : Meas<config_value_t> get_bandwidth_3dB_MHz(
                data_stream_ID_t data_stream_ID) const;
/*! @brief Determine whether the AD9361 data stream is powered on, fully
 *         active, and data is flowing.
 ******************************************************************************/
public    : bool get_data_stream_is_enabled(
                data_stream_ID_t data_stream_ID) const;
/*! @brief Measure value as it exists on hardware. Exception will be throw
 *         if hardware access fails or if invalid data stream ID is
 *         requested.
 ******************************************************************************/
public    : Meas<config_value_t> get_sampling_rate_Msps(
                data_stream_ID_t data_stream_ID) const;
/*! @brief Measure value as it exists on hardware. Exception will be throw
 *         only if invalid data stream ID is requested.
 ******************************************************************************/
public    : bool get_samples_are_complex(
                data_stream_ID_t data_stream_ID) const;
/*! @brief Measure value as it exists on hardware. Exception will be throw
 *         if hardware access fails or if invalid data stream ID is
 *         requested.
 ******************************************************************************/
public    : Meas<gain_mode_value_t> get_gain_mode(
                data_stream_ID_t data_stream_ID) const;
/*! @brief Measure value as it exists on hardware. Exception will be throw
 *         if hardware access fails or if invalid data stream ID is
 *         requested.
 ******************************************************************************/
public    : Meas<config_value_t> get_gain_dB(
                data_stream_ID_t data_stream_ID) const;

public    : virtual void unlock_all();

public    : std::vector<gain_mode_value_t>
            get_ranges_possible_gain_mode(
                data_stream_ID_t data_stream_ID) const;

public    : void set_app_inst_name_TX0_qdac(         const char* val);
public    : void set_app_inst_name_TX0_complex_mixer(const char* val);
public    : void set_app_inst_name_TX0_cic_int(      const char* val);
public    : void set_app_inst_name_TX1_qdac(         const char* val);
public    : void set_app_inst_name_TX1_complex_mixer(const char* val);
public    : void set_app_inst_name_TX1_cic_int(      const char* val);
public    : void set_app_inst_name_RX0_qadc(         const char* val);
public    : void set_app_inst_name_RX0_complex_mixer(const char* val);
public    : void set_app_inst_name_RX0_cic_dec(      const char* val);
public    : void set_app_inst_name_RX1_qadc(         const char* val);
public    : void set_app_inst_name_RX1_complex_mixer(const char* val);
public    : void set_app_inst_name_RX1_cic_dec(      const char* val);

public    : ~RadioCtrlrNoOSTuneResamp();

/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : Meas<config_value_t> set_tuning_freq_MHz(
                data_stream_ID_t data_stream_ID,
                config_value_t   tuning_freq_MHz);
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : Meas<config_value_t> set_bandwidth_3dB_MHz(
                data_stream_ID_t         data_stream_ID,
                config_value_t bandwidth_3dB_MHz);
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : Meas<config_value_t> set_sampling_rate_Msps(
                data_stream_ID_t data_stream_ID,
                config_value_t   sampling_rate_Msps);
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : bool set_samples_are_complex(
                data_stream_ID_t data_stream_ID,
                bool             samples_are_complex);
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : Meas<gain_mode_value_t> set_gain_mode(
                data_stream_ID_t  data_stream_ID,
                gain_mode_value_t gain_mode);
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : Meas<config_value_t> set_gain_dB(
                data_stream_ID_t data_stream_ID,
                config_value_t   gain_dB);
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception will be thrown if invalid data stream ID is requested.
 *          or if hardware access fails.
 *  @return Value measured after set attempt.
 ******************************************************************************/
protected : Meas<config_value_t> set_tuning_freq_complex_mixer_MHz(
                const data_stream_ID_t data_stream_ID,
                const std::string&     inst,
                const config_value_t   tuning_freq_MHz);

/*! @brief Measure value as it exists on hardware.
 *  @param[in] data_stream_ID
 *  @param[in] inst  Application instance name of complex_mixer worker which
 *                   is associated with the data stream ID (*THIS* is why
 *                   we have a routing ID).
 ******************************************************************************/
protected : Meas<config_value_t> get_tuning_freq_complex_mixer_MHz(
              const data_stream_ID_t data_stream_ID,
              const std::string&     inst) const;

//protected :  bool lock_gain_mode(data_stream_ID_t  ds_ID,
//                                 gain_mode_value_t val);

/*! @brief Get configurator data stream ID which maps to the controller
 *         data stream ID.
 *  @param[in] ctrlr_ds_ID Controller data stream ID.
 *  @return                Configurator data stream ID.
 ******************************************************************************/
protected : data_stream_ID_t get_configurator_ds_ID(
                const data_stream_ID_t& ctrlr_ds_ID) const;

protected : void log_debug_ad9361_init(AD9361_InitParam*& init_param) const;

/*! @brief Initialize No-OS/the AD9361. Not needed to be called outside of this
 *         class for normal config lock use. Is necessary outside of this class,
 *         for example, when doing something like enabling BIST loopback on
 *         AD9361 (outside of this class) before this class performs a config
 *         lock.
 ******************************************************************************/
public    : void init();

protected : void init_AD9361_InitParam();
protected : void enforce_ensm_config();

protected : void configurator_lock_cic_dec(data_stream_ID_t ds_ID,
                const std::string& inst, configurator_t& configurator);

protected : void configurator_lock_cic_int(data_stream_ID_t ds_ID,
                const std::string& inst, configurator_t& configurator);

protected : void configurator_lock_complex_mixer(data_stream_ID_t ds_ID,
                const std::string& inst, configurator_t& configurator);

protected : void ensure_configurator_lock_tune_resamp();

/*! @brief 1. Checks if configurator will allow lock request (check only,
 *            don't actually lock configurator or hardware values yet)
 *         2. If configurator will allow lock, re-initialize AD9361 if it
 *            is necessary for the lock that is about to occur.
 *  @param[in]  config_lock_request
 *  @param[in]  configurator_copy    COPY of m_configurator.
 *  @param[out] requires_reinit      This function sets this to inidicate that
 *                                   an AD9361 re-initialization must occur.
 *  @return Boolean indicator that lock is expected to succeed.
 ******************************************************************************/
protected : bool configurator_check_and_reinit(
                const ConfigLockRequest& config_lock_request,
                configurator_t           configurator_copy);

/*! @param[in]  reql_RX1A            A lock of the SMA_RX1A data stream is
 *                                   required/pending.
 *  @param[in]  reql_RX2A            A lock of the SMA_RX2A data stream is
 *                                   required/pending.
 *  @param[in]  reql_TX1A            A lock of the SMA_TX1A data stream is
 *                                   required/pending.
 *  @param[in]  reql_TX2A            A lock of the SMA_TX2A data stream is
 *                                   required/pending.
 *  @param[in]  configurator_copy    Reference to COPY of m_configurator.
 *  @return Boolean indication of whether any initialization procedure failed
 *          (which may occur depending on current config locks)
 ******************************************************************************/
protected : bool reinit_AD9361_if_required(bool reql_RX1A, bool reql_RX2A,
                bool reql_TX1A, bool reql_TX2A,
                const configurator_t& configurator_copy);

protected : bool any_configurator_configs_locked_which_prevent_ad9361_init()
                const;

protected:
template<typename T>
const char* get_No_OS_err_str(const char* API_function_cstr,
    T API_call_return_val) const {
  std::ostringstream oss;
  oss << "No-OS API call " << API_function_cstr << "()";
  oss << " returned error: \"" << strerror(-API_call_return_val) << "\"";
  return oss.str().c_str();
}

protected : std::string get_inst_name_complex_mixer(
                const DataStreamConfigLockRequest& req) const;
protected : std::string get_inst_name_cic_dec(
                const DataStreamConfigLockRequest& req) const;
protected : std::string get_inst_name_cic_int(
                const DataStreamConfigLockRequest& req) const;

public    : void set_app_inst_name_TX_0_qdac(         const char* val);
public    : void set_app_inst_name_TX_0_complex_mixer(const char* val);
public    : void set_app_inst_name_TX_0_cic_int(      const char* val);
public    : void set_app_inst_name_TX_1_qdac(         const char* val);
public    : void set_app_inst_name_TX_1_complex_mixer(const char* val);
public    : void set_app_inst_name_TX_1_cic_int(      const char* val);
public    : void set_app_inst_name_RX_0_qadc(         const char* val);
public    : void set_app_inst_name_RX_0_complex_mixer(const char* val);
public    : void set_app_inst_name_RX_0_cic_dec(      const char* val);
public    : void set_app_inst_name_RX_1_qadc(         const char* val);
public    : void set_app_inst_name_RX_1_complex_mixer(const char* val);
public    : void set_app_inst_name_RX_1_cic_dec(      const char* val);

public    : void set_app_inst_name_ad9361_data_sub(   const char* val);

protected : void throw_if_ad9361_init_failed(const char* operation = 0) const;

}; // class RadioCtrlrNoOSTuneResamp

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrNoOSTuneResamp.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_NO_OS_TUNE_RESAMP_HH
