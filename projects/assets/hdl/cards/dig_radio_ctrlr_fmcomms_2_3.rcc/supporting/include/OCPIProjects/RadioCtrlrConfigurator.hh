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

#ifndef _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_HH
#define _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_HH

#include <map>             // std::map
#include <vector>          // std::vector
#include "RadioCtrlr.hh"   // config_value_t,
                           // config_key_t, etc...
#include "LogForwarder.hh"
#include "UtilLockRConstrConfig.hh"

namespace OCPIProjects {

namespace RadioCtrlr {

typedef Util::LockRConstrConfig<ConfigValueRange> LockRConstrConfig;

/*! @brief A radio's lockable, value range-constrained configs
 *         (std::map of LockRConstrConfig).
 ******************************************************************************/
class RadioConfiguratorDataStreamBase {

protected : data_stream_type_t                        m_type;
public    : std::map<config_key_t, LockRConstrConfig> m_configs;
public    : RadioConfiguratorDataStreamBase(data_stream_type_t type);

public    : data_stream_type_t get_type() const;

public    : bool operator==(const RadioConfiguratorDataStreamBase& rhs) const;

};

/*! @brief An analog radio configurator's data stream. The minimum configs are:
 *         * tuning_freq_MHz,
 *         * bandwidth_3dB_MHz.
 ******************************************************************************/
class AnaRadioConfiguratorDataStream : public RadioConfiguratorDataStreamBase {

public    : AnaRadioConfiguratorDataStream(data_stream_type_t type);

};

/*! @brief A digital radio data stream is modelled as a dictionary of lockable,
 *         value range-constrained configs. The minimum configs for a data
 *         stream are:
 *         * tuning_freq_MHz,
 *         * bandwidth_3dB_MHz,
 *         * sampling_rate_MHz.
 ******************************************************************************/
class DigRadioConfiguratorDataStream : public AnaRadioConfiguratorDataStream {

public    : DigRadioConfiguratorDataStream(data_stream_type_t type);

};

/*! @brief Expands a basic digital radio data stream to include the following
 *         configs:
 *         * gain_mode,
 *         * gain_dB.
 ******************************************************************************/
class DigRadioConfiguratorDataStreamWithGain :
    public DigRadioConfiguratorDataStream {

public    : DigRadioConfiguratorDataStreamWithGain(data_stream_type_t type);

};

/*! @brief Software-only emulator of a hardware configuration environment which
 *         provides an API for locking/unlocking range-constrained configs.
 *         Each config has a constraint which is a valid range represented
 *         by an object of the ValidRanges type.
 ******************************************************************************/
template<class data_stream_t,
         class log_callback_t>
class Configurator : public LogForwarder<log_callback_t> {

protected : typedef std::map<data_stream_ID_t, data_stream_t> data_streams_t;

protected : using LogForwarder<log_callback_t>::log_info;
protected : using LogForwarder<log_callback_t>::log_debug;
protected : using LogForwarder<log_callback_t>::log_trace;
protected : using LogForwarder<log_callback_t>::log_warn;
protected : using LogForwarder<log_callback_t>::log_error;
protected : using LogForwarder<log_callback_t>::m_callback_log_info;
protected : using LogForwarder<log_callback_t>::m_callback_log_debug;
protected : using LogForwarder<log_callback_t>::m_callback_log_trace;
protected : using LogForwarder<log_callback_t>::m_callback_log_warn;
protected : using LogForwarder<log_callback_t>::m_callback_log_error;

/// @brief these contain data stream-specific configs
protected : data_streams_t m_data_streams;

/// @brief these are global (data stream-agnostic) configs
protected : std::map<config_key_t, LockRConstrConfig> m_configs;

private   : bool m_impose_constraints_first_run_did_occur;

public    : Configurator(data_stream_t    data_stream_0,
                         data_stream_ID_t data_stream_0_key);

/*! @brief  Lock data stream-specific config to the specified value.
 *  @return True if lock was successful. False if lock was unsuccessful.
 ******************************************************************************/
public    : bool lock_config(data_stream_ID_t data_stream_key,
                             config_key_t     config_key,
                             config_value_t   config_val,
                             config_value_t   config_val_tolerance=0);
/*! @brief  Lock global (data stream-agnostic) config to the specified value.
 *  @return True if lock was successful. False if lock was unsuccessful.
 ******************************************************************************/
public    : bool lock_config(config_key_t   config_key,
                             config_value_t config_val,
                             config_value_t config_val_tolerance=0);

/// @brief Unlock data stream-specific config.
public    : void unlock_config(data_stream_ID_t data_stream_key,
                               config_key_t     config_key);
/// @brief Unlock global (data stream-agnostic) config.
public    : void unlock_config(config_key_t config_key);
/// @brief Unlock all configs.
public    : void unlock_all();

/*! @param[in] data_stream_key Key specifier for the desired data stream.
 *  @param[in] config_key      Key specifier for config which exists in the
 *                             data stream specified by data_stream_key
 *                             config.
 *  @return    Min valid value for data stream-specific config.
 ******************************************************************************/
public    : config_value_t get_config_min_valid_value(
                data_stream_ID_t data_stream_key,
                config_key_t     config_key);

/*! @param[in] data_stream_key Key specifier for the desired data stream.
 *  @param[in] config_key      Key specifier for config which exists in the
 *                             data stream specified by data_stream_key
 *                             config.
 *  @return    Max valid value for data stream-specific config.
 ******************************************************************************/
public    : config_value_t get_config_max_valid_value(
                data_stream_ID_t data_stream_key,
                config_key_t     config_key);

/*! @param[in] config_key Key specifier for global (data stream-agnostic)
 *                        config.
 *  @return    Min valid value for global (data stream-agnostic) config.
 ******************************************************************************/
public    : config_value_t get_config_min_valid_value(
                config_key_t config_key);

/*! @param[in] config_key Key specifier for global (data stream-agnostic)
 *                        config.
 *  @return    Max valid value for global (data stream-agnostic) config.
 ******************************************************************************/
public    : config_value_t get_config_max_valid_value(
                config_key_t config_key);

public    : bool get_config_is_locked(data_stream_ID_t data_stream_key,
                                      config_key_t     config_key);

public    : bool get_config_is_locked(config_key_t config_key);

public    : void log_all_possible_config_values(bool do_info = true,
                                                bool do_debug = false);

public    : void find_data_streams_of_type(data_stream_type_t type,
                std::vector<data_stream_ID_t>& data_streams) const;

public    : const ConfigValueRanges& get_ranges_possible(
                data_stream_ID_t data_stream_key,
                config_key_t     config_key);

public    : const ConfigValueRanges& get_ranges_possible(
                config_key_t config_key);

/*! @brief Child classes must define the constraints which are applied during
 *         a single pass of constraints imposition.
 ******************************************************************************/
protected : virtual void impose_constraints_single_pass() = 0;

protected : void ensure_impose_constraints_first_run_did_occur();

protected : void throw_if_any_possible_ranges_are_empty(
                const char* calling_func_c_str = 0) const;

protected : bool lock_config(LockRConstrConfig& config,
                             config_value_t     config_val,
                             config_value_t     config_val_tolerance=0);

/*! @brief Max supported constraint dependency depth. This value was
 *         arbitrarily chosen, but should be sufficient
 *         for child classes which don't have insanely complicated
 *         constraints.
 ******************************************************************************/
protected : const static int max_constraint_dependency_depth = 32768;

/*! @brief Once something changes, multiple passes to
 *         impose_constraints_single_pass()
 *         might need to occur to capture all of the ripple effects. This
 *         function continues to call constraints_definition() (up to
 *         Configurator::max_num_contraint_passes
 *         times) until the ripple effects have resolve, i.e. until the
 *         config allowable ranges are no longer changing in value.
 ******************************************************************************/
protected : void impose_constraints();

protected : config_value_t get_config_min_valid_value(
                const LockRConstrConfig& config);

protected : config_value_t get_config_max_valid_value(
                const LockRConstrConfig& config);

protected : LockRConstrConfig& get_config(data_stream_ID_t data_stream_key,
                                          config_key_t     config_key);

protected : const LockRConstrConfig& get_config(
                data_stream_ID_t data_stream_key,
                config_key_t     config_key) const;

protected : LockRConstrConfig&       get_config(config_key_t config_key);
protected : const LockRConstrConfig& get_config(config_key_t config_key) const;

protected : enum class constraint_op_t {multiply, divide, plus, minus};

/*! @brief This is just a helper function which is made available for
 *         inherited classes to use if desired. It is a common constraint.
 ******************************************************************************/
protected : bool constrain_all_XY_such_that_X_equals_Y(
                LockRConstrConfig& cfg_X, LockRConstrConfig& cfg_Y) const;

protected : bool constrain_X_to_function_of_A_and_B(
                LockRConstrConfig&       cfg_X,
                const LockRConstrConfig& cfg_A,
                const LockRConstrConfig& cfg_B,
                constraint_op_t          op) const;

/*! @brief Impose constraint:
 *         ranges of config X = ranges of config A / ranges of config B.
 *         This is just a helper function which is made available for
 *         inherited classes to use if desired. It is a common constraint.
 ******************************************************************************/
protected : bool constrain_X_to_A_divided_by_B(
                LockRConstrConfig&       cfg_X,
                const LockRConstrConfig& cfg_A,
                const LockRConstrConfig& cfg_B) const;

/*! @brief Impose constraint:
 *         ranges of config X = ranges of config A * ranges of config B.
 *         This is just a helper function which is made available for
 *         inherited classes to use if desired. It is a common constraint.
 ******************************************************************************/
protected : bool constrain_all_XAB_such_that_X_equals_A_multiplied_by_B(
                LockRConstrConfig& cfg_X,
                LockRConstrConfig& cfg_A, LockRConstrConfig& cfg_B) const;

/*! @brief Impose constraint:
 *         ranges of config X = ranges of config A / ranges of config B.
 *         This is just a helper function which is made available for
 *         inherited classes to use if desired. It is a common constraint.
 ******************************************************************************/
protected : bool constrain_all_XAB_such_that_X_equals_A_divided_by_B(
                LockRConstrConfig& cfg_X,
                LockRConstrConfig& cfg_A, LockRConstrConfig& cfg_B) const;

/*! @brief Impose constraint:
 *         ranges of config X = ranges of config A + ranges of config B.
 *         This is just a helper function which is made available for
 *         inherited classes to use if desired. It is a common constraint.
 ******************************************************************************/
protected : bool constrain_all_XAB_such_that_X_equals_A_plus_B(
                LockRConstrConfig& cfg_X,
                LockRConstrConfig& cfg_A, LockRConstrConfig& cfg_B) const;

/*! @brief Impose constraint:
 *         ranges of config X = ranges of config A / ranges of config B.
 *         This is just a helper function which is made available for
 *         inherited classes to use if desired. It is a common constraint.
 ******************************************************************************/
protected : bool constrain_X_to_A_multiplied_by_B(
                LockRConstrConfig&       cfg_X,
                const LockRConstrConfig& cfg_A,
                const LockRConstrConfig& cfg_B) const;

/*! @brief  Impose constraint:
 *          Y(X) <= X
 *          i.e.  ranges of config Y <= ranges of config X.
 *  @return Boolean indication that any of the ranges changed.
 ******************************************************************************/
protected : bool constrain_Y_less_than_or_equal_to_X(
                LockRConstrConfig& cfg_Y,
                LockRConstrConfig& cfg_X) const;

/*! @brief  Impose constraint:
 *          Y = c, where c is a constant
 *  @return Boolean indication that any of the ranges changed.
 ******************************************************************************/
protected : bool constrain_Y_equals_constant(
                LockRConstrConfig& cfg_Y,
                config_value_t constant) const;

protected : bool limit_Y_to_less_than_or_equal_to_X(
                LockRConstrConfig&       cfg_Y,
                const LockRConstrConfig& cfg_X) const;

protected : bool limit_Y_to_greater_than_or_equal_to_X(
                LockRConstrConfig&       cfg_Y,
                const LockRConstrConfig& cfg_X) const;


}; // class Configurator

} // namespace RadioCtrlr

} // namespace OCPIProjects

#include "RadioCtrlrConfigurator.cc"

#endif // _OCPI_PROJECTS_RADIO_CTRLR_CONFIGURATOR_HH
