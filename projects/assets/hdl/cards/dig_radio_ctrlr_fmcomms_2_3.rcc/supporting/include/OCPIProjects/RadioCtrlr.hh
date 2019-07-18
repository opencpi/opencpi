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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_HH

#include <vector>  // std::vector
#include <string>  // std::string
//#include <map>     // std::map
#include "UtilValidRanges.hh" // Util::Range
#include "Meas.hh"            // Meas
#include "LogForwarder.hh"    // LogForwarder

namespace OCPIProjects {

namespace RadioCtrlr {

/// @brief Each data stream has an associated type (either RX or TX).
enum class data_stream_type_t {RX, TX};

/*! @brief We model many (all?) of the radio controller config values as
 *         doubles.
 ******************************************************************************/
typedef double config_value_t;
typedef Util::Range<config_value_t> ConfigValueRange;

typedef Util::ValidRanges<ConfigValueRange> ConfigValueRanges;

//enum class gain_mode_value_t {_auto, manual};
/// @brief intended to be "auto", "manual", or hardware-specific value
typedef std::string gain_mode_value_t;

/// @brief Each data stream has an associated ID.
typedef std::string data_stream_ID_t;

typedef std::string routing_ID_t;

/*! @brief Each config lock has an associated ID by which it can be referred to
 *         (after a lock occurs you use this ID to keep track of that lock so
 *         that you can specifically delete that lock at a later time).
 ******************************************************************************/
typedef std::string config_lock_ID_t;

typedef std::string config_key_t;

// "standard" radio controller data stream configs
const config_key_t config_key_tuning_freq_MHz    ("tuning_freq_MHz"    );
const config_key_t config_key_bandwidth_3dB_MHz  ("bandwidth_3dB_MHz"  );
const config_key_t config_key_sampling_rate_Msps ("sampling_rate_Msps" );
const config_key_t config_key_samples_are_complex("samples_are_complex");
const config_key_t config_key_gain_mode          ("gain_mode"          );
const config_key_t config_key_gain_dB            ("gain_dB"            );

/*! @brief Intended to represent requested config locks for individual data
 *         streams, which may or may not succeed.
 *  @todo / FIXME - allow analog radio controllers to include gain/gain mode
 ******************************************************************************/
class DataStreamConfigLockRequest {

/// @brief For requests by type.
protected : data_stream_type_t m_data_stream_type;
/// @brief For requests by data stream ID.
protected : data_stream_ID_t   m_data_stream_ID;
protected : routing_ID_t       m_routing_ID;

protected : config_value_t    m_tuning_freq_MHz;
protected : config_value_t    m_bandwidth_3dB_MHz;
protected : config_value_t    m_sampling_rate_Msps;
protected : bool              m_samples_are_complex;
protected : gain_mode_value_t m_gain_mode;
protected : config_value_t    m_gain_dB;

protected : config_value_t m_tolerance_tuning_freq_MHz;
protected : config_value_t m_tolerance_bandwidth_3dB_MHz;
protected : config_value_t m_tolerance_sampling_rate_Msps;
protected : config_value_t m_tolerance_gain_dB;

protected : bool m_including_data_stream_type;
protected : bool m_including_data_stream_ID;
protected : bool m_including_routing_ID;
protected : bool m_including_tuning_freq_MHz;
protected : bool m_including_bandwidth_3dB_MHz;
protected : bool m_including_sampling_rate_Msps;
protected : bool m_including_samples_are_complex;
protected : bool m_including_gain_mode;
protected : bool m_including_gain_dB;

public    : DataStreamConfigLockRequest();

/*! @brief Helps check if config lock request is malformed. A config lock
 *         request is malformed if it does not include the minimum necessary
 *         configs.
 ******************************************************************************/
//public    : void throw_if_malformed() const;

public    : data_stream_type_t get_data_stream_type() const;
public    : data_stream_ID_t   get_data_stream_ID() const;
public    : routing_ID_t       get_routing_ID() const;
public    : config_value_t     get_tuning_freq_MHz() const;
public    : config_value_t     get_bandwidth_3dB_MHz() const;
public    : config_value_t     get_sampling_rate_Msps() const;
public    : bool               get_samples_are_complex() const;
public    : gain_mode_value_t  get_gain_mode() const;
public    : config_value_t     get_gain_dB() const;

public    : config_value_t get_tolerance_tuning_freq_MHz() const;
public    : config_value_t get_tolerance_bandwidth_3dB_MHz() const;
public    : config_value_t get_tolerance_sampling_rate_Msps() const;
public    : config_value_t get_tolerance_gain_dB() const;

public    : bool get_including_data_stream_type() const;
public    : bool get_including_data_stream_ID() const;
public    : bool get_including_routing_ID() const;
public    : bool get_including_tuning_freq_MHz() const;
public    : bool get_including_bandwidth_3dB_MHz() const;
public    : bool get_including_sampling_rate_Msps() const;
public    : bool get_including_samples_are_complex() const;
public    : bool get_including_gain_mode() const;
public    : bool get_including_gain_dB() const;

/*! @brief Either this function or include_data_stream_ID()
 *         must be called before sending a
 *         DataStreamConfigLockRequest to AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock.
 ******************************************************************************/
public    : void include_data_stream_type(data_stream_type_t data_stream_type);
/*! @brief Either this function or include_data_stream_type()
 *         must be called before sending a
 *         DataStreamConfigLockRequest to AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock.
 ******************************************************************************/
public    : void include_data_stream_ID(data_stream_ID_t data_stream_ID);
/*! @brief This function must be called before sending a
 *         DataStreamConfigLockRequest to AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock.
 *  @param[in] routing_ID Must be a string in the format "RX0", "TX0", "RX1",
 *                        etc...
 ******************************************************************************/
public    : void include_routing_ID(routing_ID_t routing_ID);

/*! @brief This function must be called before sending a
 *         DataStreamConfigLockRequest to AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock.
 ******************************************************************************/
public    : void include_tuning_freq_MHz(
                config_value_t desired_tuning_freq_MHz,
                config_value_t tolerance_tuning_freq_MHz);

/*! @brief This function must be called before sending a
 *         DataStreamConfigLockRequest to AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock.
 ******************************************************************************/
public    : void include_bandwidth_3dB_MHz(
                config_value_t desired_bandwidth_3dB_MHz,
                config_value_t tolerance_bandwidth_3dB_MHz);

/*! @brief This function must be called before sending a
 *         DataStreamConfigLockRequest to DigRadioCtrlr::request_config_lock.
 ******************************************************************************/
public    : void include_sampling_rate_Msps(
                config_value_t desired_sampling_rate_Msps,
                config_value_t tolerance_sampling_rate_Msps);

/*! @brief This function must be called before sending a
 *         DataStreamConfigLockRequest to DigRadioCtrlr::request_config_lock.
 ******************************************************************************/
public    : void include_samples_are_complex(bool desired_samples_are_complex);

/*! @brief This function is optionally called before sending a
 *         DataStreamConfigLockRequest to  AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock. Call this only if request should
 *         include a desired gain mode value.
 ******************************************************************************/
public    : void include_gain_mode(gain_mode_value_t desired_gain_mode);

/*! @brief This function is optionally called before sending a
 *         DataStreamConfigLockRequest to  AnaRadioCtrlr::request_config_lock or
 *         DigRadioCtrlr::request_config_lock. Call this only if request should
 *         include desired manual gain value.
 ******************************************************************************/
public    : void include_gain_dB(
                config_value_t desired_gain_dB,
                config_value_t tolerance_gain_dB);

protected : void throw_for_invalid_get_call(const char* config) const;

}; // class DataStreamConfigLockRequest

/*! @brief Intended to represent requested controller config locks.
 *         Locks may or may not succeed.
 ******************************************************************************/
struct ConfigLockRequest {
  std::vector<DataStreamConfigLockRequest> m_data_streams;
  //std::map<std::string, config_value_t> m_configs;

}; // struct ConfigLockRequest

/*! @brief Intended to represent an existing (already locked) lock of a group
 *         of radio controller configs for an individual data stream.
 ******************************************************************************/
struct DataStreamConfigLock {

  /// @brief Each existing lock is always associated with a data stream ID.
  data_stream_ID_t  m_data_stream_ID;
  //routing_ID_t       m_routing_ID; /// @todo /FIXME - add this member and implement corresponding functionality?

  config_value_t    m_tuning_freq_MHz;
  config_value_t    m_bandwidth_3dB_MHz;
  config_value_t    m_sampling_rate_Msps;
  bool              m_samples_are_complex;
  gain_mode_value_t m_gain_mode;
  config_value_t    m_gain_dB;

  bool m_including_gain_mode;
  bool m_including_gain_dB;

}; // struct DataStreamConfigLock

/*! @brief Intended to represent an existing (already locked) lock of a group
 *         of digital radio controller configs.
 ******************************************************************************/
struct ConfigLock {
  config_lock_ID_t                  m_config_lock_ID;
  std::vector<DataStreamConfigLock> m_data_streams;
  //std::map<std::string, config_value_t> m_configs;

}; // struct ConfigLock

/*! @brief Provides an API for controlling/locking analog configs of
 *         a radio. When requesting config locks, a Configurator object is
 *         queried for valid ranges before hardware actuation is performed.
 ******************************************************************************/
template<class log_callback_t, class configurator_t>
class AnaRadioCtrlr : public LogForwarder<log_callback_t> {

protected : using LogForwarder<log_callback_t>::log_info;
protected : using LogForwarder<log_callback_t>::log_debug;
protected : using LogForwarder<log_callback_t>::log_trace;
protected : using LogForwarder<log_callback_t>::log_warn;
protected : using LogForwarder<log_callback_t>::log_error;

protected : const char* m_descriptor;

// child class is expected to have a member derived from Configurator
protected : configurator_t& m_configurator;

protected : std::vector<ConfigLock> m_config_locks;

protected : std::vector<data_stream_ID_t> m_data_stream_IDs;

public    : AnaRadioCtrlr(const char* descriptor, configurator_t& configurator);

public    : const std::string&             get_descriptor() const;

public    : const std::vector<ConfigLock>& get_config_locks() const;

/// @brief Determine whether the data stream is powered on and fully active.
public    : virtual bool get_data_stream_is_enabled(
                data_stream_ID_t data_stream_ID) const = 0;

/// @brief Measure value as it exists on hardware.
public    : virtual Meas<config_value_t> get_tuning_freq_MHz(
                data_stream_ID_t data_stream_ID) const = 0;
/// @brief Measure value as it exists on hardware.
public    : virtual Meas<config_value_t> get_bandwidth_3dB_MHz(
                data_stream_ID_t data_stream_ID) const = 0;

/*! @brief  Get current ranges for which a lock is expected to succeed (but not
 *          guaranteed to succeed). Note the ranges can change any time any
 *          other config is locked.
 *  @return Current ranges for which a lock is expected to succeed.
 ******************************************************************************/
public    : ConfigValueRanges get_ranges_possible_tuning_freq_MHz(
                data_stream_ID_t data_stream_ID) const;

/*! @brief  Get current ranges for which a lock is expected to succeed (but not
 *          guaranteed to suceed). Note the ranges can change any time any
 *          other config is locked.
 *  @return Current ranges for which a lock is expected to succeed.
 ******************************************************************************/
public    : ConfigValueRanges get_ranges_possible_bandwidth_3dB_MHz(
                data_stream_ID_t data_stream_ID) const;

/*! @brief  Get current ranges for which a lock is expected to succeed (but not
 *          guaranteed to suceed). Note the ranges can change any time any
 *          other config is locked.
 *  @return Current ranges for which a lock is expected to succeed.
 ******************************************************************************/
public    : virtual std::vector<gain_mode_value_t>
            get_ranges_possible_gain_mode(
                data_stream_ID_t data_stream_ID) const;

/*! @brief  Get current ranges for which a lock is expected to succeed (but not
 *          guaranteed to suceed). Note the ranges can change any time any
 *          other config is locked.
 *  @return Current ranges for which a lock is expected to succeed.
 ******************************************************************************/
public    : ConfigValueRanges get_ranges_possible_gain_dB(
                data_stream_ID_t data_stream_ID) const;

public    : void log_all_possible_config_values();

public    : virtual bool request_config_lock(
                config_lock_ID_t         config_lock_ID,
                const ConfigLockRequest& config_lock_request);

public    : virtual void unlock_config_lock(config_lock_ID_t config_lock_ID);
public    : virtual void unlock_all();

public    : void throw_if_data_stream_lock_request_malformed(
                const DataStreamConfigLockRequest&
                data_stream_config_lock_request) const;

/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *  @return Value measured after set attempt.
 *  @todo / FIXME - for increased functionality, make public and throw
 *          exception if value is locked?
 ******************************************************************************/
protected : virtual Meas<config_value_t> set_tuning_freq_MHz(
                data_stream_ID_t data_stream_ID,
                config_value_t   tuning_freq_MHz) = 0;
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *  @return Value measured after set attempt.
 *  @todo / FIXME - for increased functionality, make public and throw
 *          exception if value is locked?
 ******************************************************************************/
protected : virtual Meas<config_value_t> set_bandwidth_3dB_MHz(
                data_stream_ID_t data_stream_ID,
                config_value_t   bandwidth_3dB_MHz) = 0;
/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] val   Desired value to lock to.
 *  @param[in] tol   If the configurator lock succeeds and the read back
 *                   on-hardware value is within val +/- tol, the lock will be
 *                   considered successful.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : bool lock_tuning_freq_MHz(data_stream_ID_t ds_ID,
                                      config_value_t   val,
                                      config_value_t   tol);
/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] val   Desired value to lock to.
 *  @param[in] tol   If the configurator lock succeeds and the read back
 *                   on-hardware value is within val +/- tol, the lock will be
 *                   considered successful.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : bool lock_bandwidth_3dB_MHz(data_stream_ID_t ds_ID,
                                        config_value_t   val,
                                        config_value_t   tol);
/// @brief Unlocks configurator lock (no hardware action is performed).
protected : void unlock_tuning_freq_MHz(  data_stream_ID_t data_stream_ID);
/// @brief Unlocks configurator lock (no hardware action is performed).
protected : void unlock_bandwidth_3dB_MHz(data_stream_ID_t data_stream_ID);

/*! @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] cfg_key  Config key string.
 ******************************************************************************/
protected : void unlock_config(data_stream_ID_t ds_ID,
                               config_key_t     cfg_key);

/*! @brief Performs the minimum config locks required per data stream
 *         for an AnaRadioCtrlr.
 ******************************************************************************/
protected : virtual bool do_min_data_stream_config_locks(
                data_stream_ID_t data_stream_ID,
                const DataStreamConfigLockRequest&
                data_stream_config_lock_request);

protected : bool config_val_is_within_tolerance(
                config_value_t              expected_val,
                config_value_t              tolerance,
                const Meas<config_value_t>& meas) const;

public    : void throw_if_data_stream_disabled_for_read(
                const data_stream_ID_t& ds_ID,
                const char* config_description) const;
public    : void throw_if_data_stream_disabled_for_write(
                const data_stream_ID_t& ds_ID,
                const char* config_description) const;

/*! @param[in] did_lock Configurator lock success indicator.
 *  @param[in] is       Read-from hardware value is within requested tolerance.
 *  @param[in] ds_ID    Data stream ID for which a desired lock was attempted.
 *  @param[in] val      Value for which a desired lock was attempted.
 *  @param[in] tol      Value tolerance for which a desired lock was attempted.
 *  @param[in] cfg_key  Config key string.
 *  @param[in] unit     Value unit string.
 *  @param[in] meas     Pointer to Meas object containing measured (read from
 *                      hardware) value. Specifying this is optional.
 ******************************************************************************/
protected : void log_info_config_lock(bool did_lock, bool is,
                const data_stream_ID_t& ds_ID,
                config_value_t val, config_value_t tol,
                config_key_t cfg_key, const char* unit,
                const Meas<config_value_t>* meas = 0) const;

protected :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_info(const char* msg, ...) const;

protected :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_debug(const char* msg, ...) const;

protected :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_trace(const char* msg, ...) const;

protected :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_warn(const char* msg, ...) const;

protected :
#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
void log_error(const char* msg, ...) const;

}; // class AnaRadioCtrlr

/*! @brief Provides an API for controlling/locking analog and digital configs of
 *         a radio. When requesting config locks, a Configurator object is
 *         queried for valid ranges before hardware actuation is performed.
 ******************************************************************************/
template<class log_callback_t, class configurator_t>
class DigRadioCtrlr : public AnaRadioCtrlr<log_callback_t, configurator_t> {

protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::m_config_locks;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::config_val_is_within_tolerance;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::log_info;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::log_debug;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::log_trace;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::log_warn;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::log_error;
protected : using AnaRadioCtrlr<log_callback_t, configurator_t>::log_info_config_lock;

protected : std::vector<bool> m_ranges_possible_samples_are_complex;

public    : DigRadioCtrlr(const char* descriptor, configurator_t& configurator);

/// @brief Measure value as it exists on hardware.
public    : virtual Meas<config_value_t> get_sampling_rate_Msps(
                data_stream_ID_t data_stream_ID) const = 0;
/// @brief Retrieve value as it exists on hardware.
public    : virtual bool get_samples_are_complex(
                data_stream_ID_t data_stream_ID) const = 0;
/*! @brief Measure value as it exists on hardware.
 *         If hardware does not support a gain mode setting, the expectation
 *         is that an exception will be thrown.
 ******************************************************************************/
public    : virtual Meas<gain_mode_value_t> get_gain_mode(
                data_stream_ID_t data_stream_ID) const = 0;
/*! @brief Measure value as it exists on hardware (should throw exception if
 *         gain mode is auto)
 ******************************************************************************/
public    : virtual Meas<config_value_t> get_gain_dB(
                data_stream_ID_t data_stream_ID) const = 0;

/*! @brief  Get current ranges for which a lock is expected to succeed (but not
 *          guaranteed to suceed). Note the ranges can change any time any
 *          other config is locked.
 *  @return Current ranges for which a lock is expected to succeed.
 ******************************************************************************/
public    : ConfigValueRanges get_ranges_possible_sampling_rate_Msps(
                data_stream_ID_t data_stream_ID) const;

/*! @brief  Get current ranges for which a lock is expected to succeed (but not
 *          guaranteed to suceed). Note the ranges can change any time any
 *          other config is locked.
 *  @return Current ranges for which a lock is expected to succeed.
 ******************************************************************************/
public    : std::vector<bool> get_ranges_possible_samples_are_complex(
                data_stream_ID_t data_stream_ID) const;

public    : virtual bool request_config_lock(
                config_lock_ID_t         config_lock_ID,
                const ConfigLockRequest& config_lock_request);

public    : virtual void unlock_config_lock(config_lock_ID_t config_lock_ID);

/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *  @return Value measured after set attempt.
 *  @todo / FIXME - for increased functionality, make public and throw
 *          exception if value is locked?
 ******************************************************************************/
protected : virtual Meas<config_value_t> set_sampling_rate_Msps(
                data_stream_ID_t data_stream_ID,
                config_value_t   sampling_rate_Msps) = 0;
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *  @return Value measured after set attempt.
 *  @todo / FIXME - for increased functionality, make public and throw
 *          exception if value is locked?
 ******************************************************************************/
protected : virtual bool set_samples_are_complex(
                data_stream_ID_t data_stream_ID,
                bool             samples_are_complex) = 0;
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          If hardware does not support a gain mode setting, the expectation
 *          is that an exception will be thrown.
 *  @return Value measured after set attempt.
 *  @todo / FIXME - for increased functionality, make public and throw
 *          exception if value is locked?
 ******************************************************************************/
protected : virtual Meas<gain_mode_value_t> set_gain_mode(
                data_stream_ID_t  data_stream_ID,
                gain_mode_value_t gain_mode) = 0;
/*! @brief  Attempt to set on-hardware value with no guarantee of success.
 *          Exception should be thrown if if the gain mode for the
 *          requested data stream is auto.
 *  @return Value measured after set attempt.
 *  @todo / FIXME - for increased functionality, make public and throw
 *          exception if value is locked?
 ******************************************************************************/
protected : virtual Meas<config_value_t> set_gain_dB(
                data_stream_ID_t data_stream_ID,
                config_value_t   gain_dB) = 0;

/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] val   Desired value to lock to.
 *  @param[in] tol   If the configurator lock succeeds and the read back
 *                   on-hardware value is within val +/- tol, the lock will be
 *                   considered successful.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : bool lock_sampling_rate_Msps(data_stream_ID_t ds_ID,
                                         config_value_t   val,
                                         config_value_t   tol);
/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] val   Desired value to lock to.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : bool lock_samples_are_complex(data_stream_ID_t ds_ID,
                                          bool             val);
/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] val   Desired value to lock to.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : virtual bool lock_gain_mode(data_stream_ID_t  ds_ID,
                                        gain_mode_value_t val);
/*! @brief     Requests configurator lock, and if that succeeds, attempt to set
 *             on-hardware value to desired value.
 *  @param[in] ds_ID Data stream ID for which to apply lock.
 *  @param[in] val   Desired value to lock to.
 *  @param[in] tol   If the configurator lock succeeds and the read back
 *                   on-hardware value is within val +/- tol, the lock will be
 *                   considered successful.
 *  @return    Boolean indicator of success.
 ******************************************************************************/
protected : bool lock_gain_dB(data_stream_ID_t ds_ID,
                              config_value_t   val,
                              config_value_t   tol);

/// @brief Unlocks configurator lock (no hardware action is performed).
protected : void unlock_sampling_rate_Msps (data_stream_ID_t data_stream_ID);
/// @brief Unlocks configurator lock (no hardware action is performed).
protected : void unlock_samples_are_complex(data_stream_ID_t data_stream_ID);
/*! @brief Unlocks configurator lock (no hardware action is performed).
 *         If hardware does not support a gain mode setting, the expectation
 *         is that an exception will be thrown.
 ******************************************************************************/
protected : void unlock_gain_mode          (data_stream_ID_t data_stream_ID);
/*! @brief Unlocks configurator lock (no hardware action is performed).
 *         If hardware does not support a manual gain setting, the expectation
 *         is that an exception will be thrown.
 ******************************************************************************/
protected : void unlock_gain_dB            (data_stream_ID_t data_stream_ID);

/*! @brief Performs the minimum config locks required per data stream
 *         for a DigRadioCtrlr.
 ******************************************************************************/
protected : virtual bool do_min_data_stream_config_locks(
                data_stream_ID_t data_stream_ID,
                const DataStreamConfigLockRequest&
                data_stream_config_lock_request);

/*! @param[in] did_lock Configurator lock success indicator.
 *  @param[in] ds_ID    Data stream ID for which a desired lock was attempted.
 *  @param[in] val      Value for which a desired lock was attempted.
 *  @param[in] cfg_key  Config key string.
 *  @param[in] meas     Boolean indicator of measured (read from hardware)
 *                      value. Specifying this is optional.
 ******************************************************************************/
protected : void log_info_config_lock(bool did_lock,
                const data_stream_ID_t& ds_ID, gain_mode_value_t val,
                config_key_t cfg_key,
                const Meas<gain_mode_value_t>* meas = 0) const;
/*! @param[in] did_lock Configurator lock success indicator.
 *  @param[in] ds_ID    Data stream ID for which a desired lock was attempted.
 *  @param[in] val      Value for which a desired lock was attempted.
 *  @param[in] cfg_key  Config key string.
 *  @param[in] meas     Boolean indicator of measured (read from hardware)
 *                      value. Specifying this is optional.
 ******************************************************************************/
protected : void log_info_config_lock(bool did_lock,
                const data_stream_ID_t& ds_ID, config_value_t val,
                config_key_t cfg_key, const bool* meas = 0) const;

/*! @brief Log DataStreamConfigLockRequest values w/ log level info.
 *  @param[in] pre = C string to prepend to all logs. Leave as default value of
 *                   0 to not prepend anything.
 ******************************************************************************/
protected : void log_info_ds_cfg_lock_req_vals(
                const DataStreamConfigLockRequest& dsreq,
                const config_lock_ID_t& config_lock_ID) const;

protected : void throw_if_data_stream_lock_request_malformed(
                const DataStreamConfigLockRequest&
                data_stream_config_lock_request) const;

}; // class DigRadioCtrlr

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlr.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_HH
