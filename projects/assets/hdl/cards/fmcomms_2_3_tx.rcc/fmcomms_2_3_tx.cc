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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Nov 13 13:16:35 2017 EST
 * BASED ON THE FILE: fmcomms_2_3_tx.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the fmcomms_2_3_tx worker in C++
 */

#include "fmcomms_2_3_tx-worker.hh"

#include <map>     // std::map
#include <cmath>   // round(), std::log
#include <cstdlib> // strtol(), strtod()
#include <sstream> // std::ostringstream
#include <string>  // std::string
#include <iomanip> // std::setprecision()
#include <cstdint> // uint32_t, uint64_t
#include <utility> // std::pair

#include "ad9361.h"   // for RFPLL_MODULUS
#include "OcpiApi.hh" // OCPI::API::Application
#include "ocpi_component_prop_type_helpers.h" // ocpi_*_t types
#include "ad9361_common.h" // AD9361_duplex_mode_t
#include "worker_prop_parsers_ad9361_config_proxy.h" // AD9361_InitParam_ad9361_config_proxy, ad9361_config_proxy_tx_attenuation_t, parse()
#include "readers_ad9361_cfg.h" // get_AD9361_duplex_mode(), get_AD9361_use_extclk()
#include "readers_ad9361_bb_pll.h" // get_AD9361_BBPLL_FREQ_Hz()
#include "readers_ad9361_bb_tx_gain.h"   // get_AD9361_tx_attenuation_mdB()
#include "readers_ad9361_rf_tx_pll.h" // get_AD9361_Tx_RFPLL_LO_freq_Hz()
#include "readers_ad9361_bb_tx_dac.h" // get_AD9361_CLKTF_FREQ_Hz()
#include "readers_ad9361_bb_tx_filters_analog.h" // get_AD9361_tx_filter_complex_bandwidth_Hz()
#include "writers_ad9361_tx_gain.h"   // set_AD9361_tx_attenuation_mdB()
#include "writers_ad9361_rf_tx_pll.h" // set_AD9361_Tx_RFPLL_LO_freq_Hz()
#include "writers_ad9361_bb_tx_dac.h" // set_AD9361_CLKTF_FREQ_Hz()
#include "writers_ad9361_bb_tx_filters_analog.h" // set_AD9361_bb_tx_filter_complex_bandwidth_Hz()

#define PROP_STR_RF_GAIN_DB              "rf_gain_dB"
#define PROP_STR_BB_GAIN_DB              "bb_gain_dB"
#define PROP_STR_FREQUENCY_MHZ           "frequency_MHz"
#define PROP_STR_SAMPLE_RATE_MHZ         "sample_rate_MHz"
#define PROP_STR_RF_CUTOFF_FREQUENCY_MHZ "rf_cutoff_frequency_MHz"
#define PROP_STR_BB_CUTOFF_FREQUENCY_MHZ "bb_cutoff_frequency_MHz"

#define LOG_WRITTEN_START() \
  log_trace("start of %s_written()", prop.c_str())

#define LOG_WRITTEN_END() \
  log_trace("end   of %s_written()", prop.c_str())

#define LOG_READ_START() \
  log_trace("start of %s_read()", prop.c_str())

#define LOG_READ_END() \
  log_trace("end   of %s_read()", prop.c_str())

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Fmcomms_2_3_txWorkerTypes;

class Fmcomms_2_3_txWorker : public Fmcomms_2_3_txWorkerBase {

private:

  const std::string m_worker_str;

  const double m_No_OS_API_precision_tx_sampling_freq;
  const double m_No_OS_API_precision_tx_rf_bandwidth;

  bool m_app_inst_name_ad9361_config_proxy_initialized;
  bool m_app_inst_name_ad9361_data_sub_initialized;
  bool m_config_initialized;

  // property writes may be delayed until after initial OWD props are
  // initialized, the purpose of this vector is to queue up pending writes to
  // allow them to be executed in the same original order they would have been
  // executed in had they not been delayed (why do we care about execution
  // order? - because some property's allowable values are dependent on the
  // current value of other properties, it's the old "batch processing of
  // frontend resources" problem)
  std::vector<std::string> m_pending_prop_write_queue;

  // first is string representation of this worker's prop name, second is
  // pointer to this worker's prop
  std::map<std::string, double*> m_map_val;

  // first is string representation of this worker's min/max/step prop name
  // whose value is expected to always be that of a parameter, second is the
  // expected value
  std::map<std::string, double> m_map_fixedval;

public:

  Fmcomms_2_3_txWorker() : Fmcomms_2_3_txWorkerBase(),
      m_worker_str("fmcomms_2_3_tx.rcc"),
      m_No_OS_API_precision_tx_sampling_freq(1.),
      m_No_OS_API_precision_tx_rf_bandwidth(1.),
      m_app_inst_name_ad9361_config_proxy_initialized(false),
      m_app_inst_name_ad9361_data_sub_initialized(false),
      m_config_initialized(false)
  {
    // rf_gain_dB is not fixed
    m_map_fixedval["rf_gain_max_dB"              ] = FMCOMMS_2_3_TX_RF_GAIN_MAX_DB_P;
    m_map_fixedval["rf_gain_min_dB"              ] = FMCOMMS_2_3_TX_RF_GAIN_MIN_DB_P;
    m_map_fixedval["rf_gain_step_dB"             ] = FMCOMMS_2_3_TX_RF_GAIN_STEP_DB_P;

    m_map_fixedval[PROP_STR_BB_GAIN_DB           ] = FMCOMMS_2_3_TX_NOT_SUPPORTED_P;
    m_map_fixedval["bb_gain_max_dB"              ] = FMCOMMS_2_3_TX_BB_GAIN_MAX_DB_P;
    m_map_fixedval["bb_gain_min_dB"              ] = FMCOMMS_2_3_TX_BB_GAIN_MIN_DB_P;
    m_map_fixedval["bb_gain_step_dB"             ] = FMCOMMS_2_3_TX_BB_GAIN_STEP_DB_P;

    // frequency_MHz is not fixed
    m_map_fixedval["frequency_max_MHz"           ] = FMCOMMS_2_3_TX_FREQUENCY_MAX_MHZ_P;
    m_map_fixedval["frequency_min_MHz"           ] = FMCOMMS_2_3_TX_FREQUENCY_MIN_MHZ_P;
    // frequency_step_MHz is not fixed

    // sample_rate_MHz is not fixed
    m_map_fixedval["sample_rate_max_MHz"         ] = FMCOMMS_2_3_TX_SAMPLE_RATE_MAX_MHZ_P;
    m_map_fixedval["sample_rate_min_MHz"         ] = FMCOMMS_2_3_TX_SAMPLE_RATE_MIN_MHZ_P;
    // sample_rate_step_MHz is not fixed

    m_map_fixedval[PROP_STR_RF_CUTOFF_FREQUENCY_MHZ] = FMCOMMS_2_3_TX_NOT_SUPPORTED_P;
    m_map_fixedval["rf_cutoff_frequency_max_MHz" ] = FMCOMMS_2_3_TX_RF_CUTOFF_FREQUENCY_MAX_MHZ_P;
    m_map_fixedval["rf_cutoff_frequency_min_MHz" ] = FMCOMMS_2_3_TX_RF_CUTOFF_FREQUENCY_MIN_MHZ_P;
    m_map_fixedval["rf_cutoff_frequency_step_MHz"] = FMCOMMS_2_3_TX_RF_CUTOFF_FREQUENCY_STEP_MHZ_P;

    // bb_cutoff_frequency_MHz is not fixed
    m_map_fixedval["bb_cutoff_frequency_max_MHz" ] = FMCOMMS_2_3_TX_BB_CUTOFF_FREQUENCY_MAX_MHZ_P;
    m_map_fixedval["bb_cutoff_frequency_min_MHz" ] = FMCOMMS_2_3_TX_BB_CUTOFF_FREQUENCY_MIN_MHZ_P;
    // bb_cutoff_frequency_step_MHz is not fixed

    m_map_val["rf_gain_max_dB"              ] = &m_properties.rf_gain_max_dB;
    m_map_val["rf_gain_min_dB"              ] = &m_properties.rf_gain_min_dB;
    m_map_val["rf_gain_step_dB"             ] = &m_properties.rf_gain_step_dB;
    m_map_val["bb_gain_max_dB"              ] = &m_properties.bb_gain_max_dB;
    m_map_val["bb_gain_min_dB"              ] = &m_properties.bb_gain_min_dB;
    m_map_val["bb_gain_step_dB"             ] = &m_properties.bb_gain_step_dB;
    m_map_val["frequency_max_MHz"           ] = &m_properties.frequency_max_MHz;
    m_map_val["frequency_min_MHz"           ] = &m_properties.frequency_min_MHz;
    m_map_val["sample_rate_max_MHz"         ] = &m_properties.sample_rate_max_MHz;
    m_map_val["sample_rate_min_MHz"         ] = &m_properties.sample_rate_min_MHz;
    m_map_val["rf_cutoff_frequency_max_MHz" ] = &m_properties.rf_cutoff_frequency_max_MHz;
    m_map_val["rf_cutoff_frequency_min_MHz" ] = &m_properties.rf_cutoff_frequency_min_MHz;
    m_map_val["rf_cutoff_frequency_step_MHz"] = &m_properties.rf_cutoff_frequency_step_MHz;
    m_map_val["bb_cutoff_frequency_max_MHz" ] = &m_properties.bb_cutoff_frequency_max_MHz;
    m_map_val["bb_cutoff_frequency_min_MHz" ] = &m_properties.bb_cutoff_frequency_min_MHz;

    m_map_val[PROP_STR_RF_GAIN_DB             ] = &m_properties.rf_gain_dB;
    m_map_val[PROP_STR_BB_GAIN_DB             ] = (double*) &m_properties.bb_gain_dB;
    m_map_val[PROP_STR_FREQUENCY_MHZ          ] = &m_properties.frequency_MHz;
    m_map_val[PROP_STR_SAMPLE_RATE_MHZ        ] = &m_properties.sample_rate_MHz;
    m_map_val[PROP_STR_RF_CUTOFF_FREQUENCY_MHZ] = (double*) &m_properties.rf_cutoff_frequency_MHz;
    m_map_val[PROP_STR_BB_CUTOFF_FREQUENCY_MHZ] = &m_properties.bb_cutoff_frequency_MHz;
  }

private:

  void log_info(const char* msg, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 2, 3)))
#endif
  {
    if(!m_properties.enable_log_info) { return; }
    printf("INFO : %s worker: ",m_worker_str.c_str());
    va_list arg;
    va_start(arg, msg);
    vprintf(msg, arg);
    va_end(arg);
    printf("\n");
  };

  void log_debug(const char* msg, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 2, 3)))
#endif
  {
    if(!m_properties.enable_log_debug) { return; }
    printf("DEBUG: %s worker: ",m_worker_str.c_str());
    va_list arg;
    va_start(arg, msg);
    vprintf(msg, arg);
    va_end(arg);
    printf("\n");
  };

  void log_trace(const char* msg, ...)
#ifdef __GNUC__
  __attribute__((format(printf, 2, 3)))
#endif
  {
    if(!m_properties.enable_log_trace) { return; }
    printf("TRACE: %s worker: ",m_worker_str.c_str());
    va_list arg;
    va_start(arg, msg);
    vprintf(msg, arg);
    va_end(arg);
    printf("\n");
  };

  std::string strip_unit(std::string prop) {
    return prop.substr(prop.find_last_of("_")+1);
  } ///
  //
  void disable_TX_FIR_filter() {
    OCPI::API::Application& app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    std::string DISABLE_str;
    app.getProperty(inst, "DISABLE",       DISABLE_str);
    app.setProperty(inst, "tx_fir_en_dis", DISABLE_str.c_str());
  }

  void set_ad9361_init(ad9361_config_proxy_ad9361_init& ad9361_init)
  {
    std::string ad9361_init_str = to_string(ad9361_init);
    OCPI::API::Application& app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;

    // save property values which may change during write to ad9361_init

    OCPI::API::Property p_rx_rf_bandwidth(app, inst, "rx_rf_bandwidth" );
    OCPI::API::Property p_rx_lo_freq(     app, inst, "rx_lo_freq"      );
    // we do not set ad9361_config_proxy's rx_sampling_freq because this worker's sample_rate_MHz's value will affect both the ad9361_config_proxy's tx_sampling_freq and rx_sampling_freq properties
    std::string str_rx_gain_control_mode;
    app.getProperty(inst, "rx_gain_control_mode", str_rx_gain_control_mode);
    std::string str_rx_rf_gain;
    app.getProperty(inst, "rx_rf_gain", str_rx_rf_gain);

    ocpi_ulong_t     rx_rf_bandwidth = p_rx_rf_bandwidth.getULongValue();
    ocpi_ulonglong_t rx_lo_freq      = p_rx_lo_freq.getULongLongValue();
    // we do not set ad9361_config_proxy's rx_sampling_freq because this worker's sample_rate_MHz's value will affect both the ad9361_config_proxy's tx_sampling_freq and rx_sampling_freq properties
    ad9361_config_proxy_rx_rf_gain_t rx_rf_gain;
    parse(str_rx_rf_gain.c_str(), rx_rf_gain); //! @todo TODO/FIXME -  ignoring return value, idk if this is the right thing to to...

    // write ad9361_init

    app.setProperty(inst, "ad9361_init", ad9361_init_str.c_str());

    // re-apply saved properties property values which may have changed during
    // write to ad9361_init

    p_rx_rf_bandwidth.setULongValue( rx_rf_bandwidth );
    p_rx_lo_freq.setULongLongValue(  rx_lo_freq      );
    // we do not set ad9361_config_proxy's rx_sampling_freq because this worker's sample_rate_MHz's value will affect both the ad9361_config_proxy's tx_sampling_freq and rx_sampling_freq properties
    app.setProperty(inst, "rx_gain_control_mode", str_rx_gain_control_mode.c_str());
    std::string rx_rf_gain_str = to_string(rx_rf_gain);
    app.setProperty(inst, "rx_rf_gain", rx_rf_gain_str.c_str());
  }

  /* @brief calc_difference_between_desired_and_in_situ_after_hw_write
   ****************************************************************************/
  const char* calc_difference_between_desired_and_in_situ_after_hw_write(
      const std::string& prop,
      const std::pair<double, std::string>& desired,
      std::pair<double, std::string>& in_situ_after_hw_write,
      double& diff) {
    const std::string unit_desired = strip_unit(prop);
    convert_value(in_situ_after_hw_write, unit_desired);

    const double& val_desired          =                desired.first;
    double& val_in_situ_after_hw_write = in_situ_after_hw_write.first;

    diff = val_in_situ_after_hw_write - val_desired;
    return 0;
  }

  void do_log_info_desired_vs_in_situ(const std::string& prop,
      const std::pair<double, std::string>& desired,
      std::pair<double, std::string>& in_situ_after_hw_write) {

    double diff;
    const char* err = calc_difference_between_desired_and_in_situ_after_hw_write(prop, desired, in_situ_after_hw_write, diff);
    if(err == 0)
    {
      log_info("attempted property write: prop=%s,\tdesired value = %.15f %s,\tcurrent in-situ nominal value = %.15f %s,\t(difference=%.15f %s)", prop.c_str(), desired.first, desired.second.c_str(), in_situ_after_hw_write.first, in_situ_after_hw_write.second.c_str(), diff, desired.second.c_str());
    }
    else
    {
      log_info("attempted property write: prop=%s,\tdesired value=%.15f,\terror determining in-situ values: %s", prop.c_str(), desired.first, err);
    }
  }

  RCCResult
  error_if_fixed_prop_val_is_not_prop_fixedval(const std::string& prop)
  {
    double diff = ((*(m_map_val.at(prop))) - m_map_fixedval.at(prop));

    // ideally the only acceptable difference would be 0. but we have to give
    // some wiggle room due to double precision rounding 0.00000000000001 was
    // arbitrarily chosen to be "very little wiggle room"
    if(std::abs(diff) > 0.00000000000001)
    {
      RCCResult res = setError("attempted to overwrite fixed property value of %s (%.19f) with a value of %.19f", prop.c_str(), m_map_fixedval.at(prop), *(m_map_val.at(prop)));
      return res;
    }
    return RCC_OK;
  }

  RCCResult
  error_if_step_prop_val_is_not_prop_fixedval(const std::string& prop)
  {
    double diff = ((*(m_map_val.at(prop))) - m_map_fixedval.at(prop));

    // ideally the only acceptable difference would be 0. but we have to give
    // some wiggle room due to double precision rounding 0.00000000000001 was
    // arbitrarily chosen to be "very little wiggle room"
    if(std::abs(diff) > 0.00000000000001)
    {
      RCCResult res = setError("attempted to overwrite fixed property value of %s (%.19f) with a value of %.19f", prop.c_str(), m_map_fixedval.at(prop), *(m_map_val.at(prop)));
      return res;
    }
    return RCC_OK;
  }

  RCCResult
  error_if_min_prop_val_is_not_prop_fixedval(const std::string& prop)
  {
    double diff = ((*(m_map_val.at(prop))) - m_map_fixedval.at(prop));

    // ideally the only acceptable difference would be 0. but we have to give
    // some wiggle room due to double precision rounding 0.00000000000001 was
    // arbitrarily chosen to be "very little wiggle room"
    if(std::abs(diff) > 0.00000000000001)
    {
      RCCResult res = setError("attempted to overwrite fixed property value of %s (%.19f) with a value of %.19f", prop.c_str(), m_map_fixedval.at(prop), *(m_map_val.at(prop)));
      return res;
    }
    if((*(m_map_val.at(prop))) < m_map_fixedval.at(prop))
    {
      // increase number just a tad to make it in bounds
      *(m_map_val.at(prop)) = m_map_fixedval.at(prop);
    }
    return RCC_OK;
  }

  RCCResult
  error_if_max_prop_val_is_not_prop_fixedval(const std::string& prop)
  {
    double diff = ((*(m_map_val.at(prop))) - m_map_fixedval.at(prop));

    // ideally the only acceptable difference would be 0. but we have to give
    // some wiggle room due to double precision rounding 0.00000000000001 was
    // arbitrarily chosen to be "very little wiggle room"
    if(std::abs(diff) > 0.00000000000001)
    {
      RCCResult res = setError("attempted to overwrite fixed property value of %s (%.19f) with a value of %.19f", prop.c_str(), m_map_fixedval.at(prop), *(m_map_val.at(prop)));
      return res;
    }
    if((*(m_map_val.at(prop))) > m_map_fixedval.at(prop))
    {
      // decrease number just a tad to make it in bounds
      *(m_map_val.at(prop)) = m_map_fixedval.at(prop);
    }
    return RCC_OK;
  }

  const char* get_min_allowed_val(const std::string prop, double& min) {
    if(prop == PROP_STR_RF_GAIN_DB)
    {
      min =   FMCOMMS_2_3_TX_RF_GAIN_MIN_DB_P;
    }
    else if(prop == PROP_STR_BB_GAIN_DB)
    {
      min =   FMCOMMS_2_3_TX_BB_GAIN_MIN_DB_P;
    }
    else if(prop == PROP_STR_FREQUENCY_MHZ)
    {
      min =   FMCOMMS_2_3_TX_FREQUENCY_MIN_MHZ_P;
    }
    else if(prop == PROP_STR_SAMPLE_RATE_MHZ)
    {
      min =   FMCOMMS_2_3_TX_SAMPLE_RATE_MIN_MHZ_P;
    }
    else if(prop == PROP_STR_RF_CUTOFF_FREQUENCY_MHZ)
    {
      min =   FMCOMMS_2_3_TX_RF_CUTOFF_FREQUENCY_MIN_MHZ_P;
    }
    else if(prop == PROP_STR_BB_CUTOFF_FREQUENCY_MHZ)
    {
      min =   FMCOMMS_2_3_TX_BB_CUTOFF_FREQUENCY_MIN_MHZ_P;
    }
    else
    {
      std::string err = "requested min for invalid property named " + prop;
      return err.c_str();
    }
    return 0;
  }

  const char* get_max_allowed_val(const std::string prop, double& max) {
    if(prop == PROP_STR_RF_GAIN_DB)
    {
      max =   FMCOMMS_2_3_TX_RF_GAIN_MAX_DB_P;
    }
    else if(prop == PROP_STR_BB_GAIN_DB)
    {
      max =   FMCOMMS_2_3_TX_BB_GAIN_MAX_DB_P;
    }
    else if(prop == PROP_STR_FREQUENCY_MHZ)
    {
      max =   FMCOMMS_2_3_TX_FREQUENCY_MAX_MHZ_P;
    }
    else if(prop == PROP_STR_SAMPLE_RATE_MHZ)
    {
      max =   FMCOMMS_2_3_TX_SAMPLE_RATE_MAX_MHZ_P;
    }
    else if(prop == PROP_STR_RF_CUTOFF_FREQUENCY_MHZ)
    {
      max =   FMCOMMS_2_3_TX_RF_CUTOFF_FREQUENCY_MAX_MHZ_P;
    }
    else if(prop == PROP_STR_BB_CUTOFF_FREQUENCY_MHZ)
    {
      max =   FMCOMMS_2_3_TX_BB_CUTOFF_FREQUENCY_MAX_MHZ_P;
    }
    else
    {
      std::string err = "requested max for invalid property named " + prop;
      return err.c_str();
    }
    return 0;
  }

  /*! @brief E.g., if prop is "rf_gain_dB", return RCC_OK if
   *         m_properties_rf_gain_min_dB <= m_properties.rf_gain_dB <= 
   *         m_properties_rf_gain_max_dB, and return setError(...) otherwise.
   ****************************************************************************/
  RCCResult check_bounds(const std::string& prop, double desired_val) {
    double min, max;
    const char* errmin = get_min_allowed_val(prop, min);
    if(errmin != 0) { return setError(errmin); }
    const char* errmax = get_max_allowed_val(prop, max);
    if(errmax != 0) { return setError(errmax); }
    const bool too_low  = (desired_val < min);
    const bool too_high = (desired_val > max);
    if(too_high || too_low)
    {
      std::ostringstream oss;
      oss << std::setprecision(17) << prop << " too " << (too_high ? "high" : "low") << " (\"";
      oss << desired_val << "\") can only be in the range [ ";
      oss << std::setprecision(17) << min << " to ";
      oss  << std::setprecision(17) << max << " ]";
      
      return setError(oss.str().c_str());
    }
    return RCC_OK;
  }

  void get_adjusted_val_to_be_applied_to_hw(
      const std::string& prop, double val_desired,
      std::pair<ocpi_ulonglong_t, std::string>& adjusted) {
    if(prop.compare(PROP_STR_FREQUENCY_MHZ) == 0)
    {
      /*uint32_t Tx_RFPLL_input_F_REF;
      OCPI::API::Application& app = getApplication();
      const char* err1 = get_in_situ_Tx_RFPLL_input_F_REF(app, Tx_RFPLL_input_F_REF);
      if(err1 != 0) { LOG_WRITTEN_END(); return setError(err1); }
      float assumed_constant_value_for_ref_divider; // I'm not 100% sure that this is constant for all TX RF LO freqs, but it has only ever been observed to have the same value (of 2)
      const char* err2 = get_in_situ_Tx_RFPLL_ref_divider( app, assumed_constant_value_for_ref_divider );
      if(err2 != 0) { LOG_WRITTEN_END(); return setError(err2); }
      double step_size_Hz = ((double)Tx_RFPLL_input_F_REF) * ((double)assumed_constant_value_for_ref_divider); // why * and not / ???
      step_size_Hz /= RFPLL_MODULUS;
      step_size_Hz /= 2.;
      if(m_properties.frequency_MHz <= 3000.) { step_size_Hz /= 2.; }
      if(m_properties.frequency_MHz <= 1500. - Tx_RFPLL_input_F_REF*2./RFPLL_MODULUS/4.) { step_size_Hz /= 2.; } // - Tx... is for a weird edge case with No-OS/AD9361
      if(m_properties.frequency_MHz <=750.  ) { step_size_Hz /= 2.; }
      if(m_properties.frequency_MHz <= 375.  ) { step_size_Hz /= 2.; }
      if(m_properties.frequency_MHz <= 187.5 ) { step_size_Hz /= 2.; }
      if(m_properties.frequency_MHz <= 93.75 ) { step_size_Hz /= 2.; }
      log_info("step_size_Hz=%.15f", step_size_Hz);
      double val_num_Hz_steps = (*(m_map_val.at(prop)) * 1e6)/ step_size_Hz;
      log_info("val_num_Hz_steps=%.15f", val_num_Hz_steps);
      // note that this effectively enforces m_No_OS_API_precision_tx_lo_freq
      double closest_stepped_freq_Hz = std::round(val_num_Hz_steps)*step_size_Hz;
      log_info("closest_stepped_freq_Hz=%15f", closest_stepped_freq_Hz);
      uint64_t val_adjusted_Hz = (uint64_t) std::ceil(closest_stepped_freq_Hz); // ceil because No-OS does floor?
      //val_adjusted_Hz=750e6 + 1; // debugging (forcing value)
      */

      uint64_t val = (uint64_t) std::round(val_desired*1e6);
      adjusted = std::make_pair(val, "Hz");
    }
    else
    {
      std::string err = "invalid property: " + prop;
      throw err.c_str();
    }
    log_info("attempted property write: prop=%s,\tdesired value=%.15f,\tvalue to be written (after potential adjustment due to No-OS API precision limitations) is %lu %s", prop.c_str(), val_desired, adjusted.first, adjusted.second.c_str());
  }

  /*void get_adjusted_val_to_be_applied_to_hw(
      const std::string& prop, double val_desired,
      std::pair<ocpi_float_t, std::string>& adjusted) {
    if(prop.compare(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ) == 0)
    {
      // AD9361_Reference_Manual_UG-570.pdf p. 10 "The baseband Tx analog filter
      // calibration tunes the cutoff frequency of the third-order Butterworth
      // Tx anti-imaging filter. The TX filter ... is normally calibrated to
      // 1.6x the BBBW. Note that BBBW is half the complex bandwidth..."
      double BBBW_Hz = (val_desired / 1.6) * 1e6;

      try{
        OCPI::API::Application& app = getApplication();
        double BBPLL_FREQ_Hz;
        const char* err = get_AD9361_BBPLL_FREQ_Hz(app, m_properties.app_inst_name_ad9361_config_proxy, BBPLL_FREQ_Hz);
        if(err == 0)
        {
          log_debug("BBPLL_FREQ_Hz=%.15f", BBPLL_FREQ_Hz);

          // AD9361_Reference_Manual_UG-570.pdf p. 10 The TX filter ... is
          // normally calibrated to 1.6x the BBBW
          double BBBW_DESIRED_MHz = val_desired / 1.6;
          log_debug("BBBW_DESIRED_MHz=%.15f", BBBW_DESIRED_MHz);

          // AD9361_Reference_Manual_UG-570.pdf p. 10 formula (6)
          double BBPLL_MHz = BBPLL_FREQ_Hz / 1e6;
          const double PI = 3.141592653589793;
          double divider_desired = BBPLL_MHz * std::log(2.) / BBBW_DESIRED_MHz / 1.6 / 2. / PI;
          log_debug("divider_desired=%.15f", divider_desired);

          double lo_divider = floor(divider_desired);
          double hi_divider = ceil(divider_desired);

          // AD9361_Reference_Manual_UG-570.pdf p. 10 formula (7)
          double lo_BBBW_ACTUAL_MHz = BBPLL_MHz * std::log(2.) / 1.6 / 2. / PI / lo_divider;
          double hi_BBBW_ACTUAL_MHz = BBPLL_MHz * std::log(2.) / 1.6 / 2. / PI / hi_divider;
          log_debug("lo_BBBW_ACTUAL_MHz=%.15f", lo_BBBW_ACTUAL_MHz);
          log_debug("hi_BBBW_ACTUAL_MHz=%.15f", hi_BBBW_ACTUAL_MHz);

          double lo_diff = std::abs(BBBW_DESIRED_MHz - lo_BBBW_ACTUAL_MHz);
          double hi_diff = std::abs(BBBW_DESIRED_MHz - hi_BBBW_ACTUAL_MHz);
          log_debug("lo_diff=%.15f", lo_diff);
          log_debug("hi_diff=%.15f", hi_diff);

          double adjusted_divider = (lo_diff < hi_diff) ? lo_divider : hi_divider;
          //double adjusted_BBBW_DESIRED_MHz = (lo_diff < hi_diff) ? lo_BBBW_ACTUAL_MHz : hi_BBBW_ACTUAL_MHz;
          double adjusted_BBBW_DESIRED_MHz = BBPLL_MHz * std::log(2.) / 1.6 / 2. / PI / (adjusted_divider-0.8); // 0.8 is needed because No-OS rounding is so coarse in some cases we need to bump the divider down by a bit so No-OS's "ceiling" function (DIV_ROUND_UP) won't overflow to adjusted_divider+1
          adjusted_BBBW_DESIRED_MHz += 0.000010; // just to ensure don't accidentally bump up to next divider ceiling

          // set val_desired to BB (real, not complex) cutoff frequency with least diff
          log_debug("old desired BBBW_Hz=%.15f", BBBW_Hz);
          BBBW_Hz = (adjusted_BBBW_DESIRED_MHz * 1e6);
          log_debug("new desired BBBW_Hz=%.15f", BBBW_Hz);
        }
      }
      catch(const std::exception& e) {}

      // round to nearest 0.5 (No-OS expects uint32_t, but
      // the value later gets multiplied by 2 when converted to
      // complex bandwidth before being passed to
      // set_in_situ_tx_complex_bandwidth_Hz()
      size_t num_halves = round(BBBW_Hz / 0.5);

      float val = ((float)(num_halves)) * 0.5;
      adjusted = std::make_pair(val, "Hz");
      log_info("attempted property write: prop=%s,\tdesired value=%.15f, value to be written (after potential conversions 1: to BBBW_Hz and 2: precision adjustment due to No-OS API precision limitations) is %7f %s", prop.c_str(), val_desired, adjusted.first, adjusted.second.c_str());
    }
  }*/

  void get_adjusted_val_to_be_applied_to_hw(
      const std::string& prop, double val_desired,
      std::pair<ocpi_ulong_t, std::string>& adjusted) {
    if(prop.compare(PROP_STR_SAMPLE_RATE_MHZ) == 0)
    {
      ocpi_ulong_t val = (ocpi_ulong_t) round(val_desired*1e6);
      adjusted = std::make_pair(val, "Hz");
    }
    else if(prop.compare(PROP_STR_RF_GAIN_DB) == 0)
    {
      // value applied to hardware via the ad9361_config_proxy's ulong
      // tx_attenuation property write (which performs a No-OS API call to
      // set_ad9361_tx_attenuation()) which requires tx attenuation to be
      // specified in mdB

      // round tx rf gain in dB to nearest step size
      double desired_nsteps = val_desired / FMCOMMS_2_3_TX_RF_GAIN_STEP_DB_P;
      double desired_nsteps_rounded = round(desired_nsteps);
      double x = desired_nsteps_rounded * FMCOMMS_2_3_TX_RF_GAIN_STEP_DB_P;
      double d_tx_rf_gain_dB_rounded = x;

      // convert tx rf gain in dB to tx rf gain in mdB
      double d_tx_rf_gain_mdB = d_tx_rf_gain_dB_rounded * 1000.;

      // convert tx rf gain in mdB to tx attenuation in mdB
      double d_tx_attenuation_mdB = -d_tx_rf_gain_mdB;

      // convert to ulong property type
      ocpi_ulong_t tx_attenuation_mdB = (ocpi_ulong_t) round(d_tx_attenuation_mdB);

      adjusted = std::make_pair(tx_attenuation_mdB, "mdB");
    }
    else if(prop.compare(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ) == 0)
    {
      //! @todo TODO/FIXME - calc value to write to tx_rf_bandwidth that will result in actual nominal 3dB freq instead of assuming No-OS/ad9361_config_proxy's 'tx_rf_bandwidth' corresponds to the 3dB freq
      ocpi_ulong_t val = (ocpi_ulong_t) round(val_desired*1e6); // MHz to Hz
      adjusted = std::make_pair(val, "Hz");
    }
    else
    {
      std::string err = "invalid property: " + prop;
      throw err.c_str();
    }
    log_info("attempted property write: prop%s,\tdesired value=%.15f,\tvalue to be written (after potential adjustment due to No-OS API precision limitations) is %u %s", prop.c_str(), val_desired, adjusted.first, adjusted.second.c_str());
  }

  RCCResult set_in_situ_val(const std::string& prop,
      ocpi_ulonglong_t adjusted_val_to_be_applied_to_hw) {
    OCPI::API::Application& app = getApplication();
    ocpi_ulonglong_t& val_adjusted = adjusted_val_to_be_applied_to_hw;
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    if(prop.compare(PROP_STR_FREQUENCY_MHZ) == 0)
    {
      const char* err = set_AD9361_Tx_RFPLL_LO_freq_Hz(app, inst, val_adjusted);
      if(err != 0) { return setError(err); }
    }
    else
    {
      return setError("invalid property: %s", prop.c_str());
    }
    return RCC_OK;
  }

  RCCResult set_in_situ_val(const std::string& prop,
      ocpi_ulong_t adjusted_val_to_be_applied_to_hw) {
    OCPI::API::Application& app = getApplication();
    ocpi_ulong_t& val_adjusted = adjusted_val_to_be_applied_to_hw;
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    if(prop.compare(PROP_STR_SAMPLE_RATE_MHZ) == 0)
    {
      const char* err = set_AD9361_CLKTF_FREQ_Hz(app, inst, val_adjusted);
      if(err != 0) { return setError(err); }
    }
    else if(prop.compare(PROP_STR_RF_GAIN_DB) == 0)
    {
      //! @todo TODO / FIXME - fix this hackish channel handling
      const char* err = set_AD9361_tx_attenuation_TX1_mdB(app, inst, val_adjusted);
      if(err != 0) { return setError(err); }
    }
    else if(prop.compare(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ) == 0)
    {
      const char* err = set_AD9361_tx_rf_bandwidth_Hz(app, inst, val_adjusted);
      if(err != 0) { return setError(err); }
    }
    else
    {
      return setError("invalid property: %s", prop.c_str());
    }
    return RCC_OK;
  }

  /*RCCResult set_in_situ_val(const std::string& prop,
      float adjusted_val_to_be_applied_to_hw) {
    OCPI::API::Application& app = getApplication();
    float& val_adjusted = adjusted_val_to_be_applied_to_hw;
    if(prop.compare(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ) == 0)
    {
      // val_adjusted is expected to be in (EXACT) multiples of 0.5
      uint32_t c_bw_Hz = (uint32_t) round(val_adjusted * 2.); // complex bw = cutoff freq * 2
      const char* err = set_AD9361_bb_tx_filter_complex_bandwidth_Hz(app, m_properties.app_inst_name_ad9361_config_proxy, c_bw_Hz);
      if(err != 0) { return setError(err); }
    }
    else
    {
      return setError("invalid property: %s", prop.c_str());
    }
    return RCC_OK;
  }*/

  const char* get_in_situ_val(const std::string& prop,
      std::pair<double, std::string>& in_situ_after_hw_write) {
    OCPI::API::Application& app = getApplication();
    double val;
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    if(prop.compare(PROP_STR_RF_GAIN_DB) == 0)
    {
      double atten;
      //! @todo TODO/FIXME - attenuation[0] assumes 1R1T
      const char* err = get_AD9361_tx_attenuation_TX1_mdB(app, inst, atten);
      val = -atten; // gain = -attenuation
      in_situ_after_hw_write = std::make_pair(val, "mdB");
      return err;
    }
    else if(prop.compare(PROP_STR_FREQUENCY_MHZ) == 0)
    {
      const char* err = get_AD9361_Tx_RFPLL_LO_freq_Hz(app, inst, val);
      in_situ_after_hw_write = std::make_pair(val, "Hz");
      return err;
    }
    else if(prop.compare(PROP_STR_SAMPLE_RATE_MHZ) == 0)
    {
      const char* err = get_AD9361_CLKTF_FREQ_Hz(app, inst, val);
      in_situ_after_hw_write = std::make_pair(val, "Hz");
      return err;
    }
    else if(prop.compare(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ) == 0)
    {
      /*if(m_properties.enable_log_debug)
      {
        double BBPLL_FREQ_Hz;
        const char* err = get_AD9361_BBPLL_FREQ_Hz(app, m_properties.app_inst_name_ad9361_config_proxy, BBPLL_FREQ_Hz);
        if(err == 0) { log_debug("BBPLL_FREQ=%.15f Hz", BBPLL_FREQ_Hz); }
      }
      double c_bw_Hz;
      const char* err = get_AD9361_tx_filter_complex_bandwidth_Hz(app, m_properties.app_inst_name_ad9361_config_proxy, c_bw_Hz);

      // bb_cutoff_frequency is also half the complex bandwidth
      val = c_bw_Hz / 2.;

      in_situ_after_hw_write = std::make_pair(val, "Hz");
      return err;*/
      const char* err = get_AD9361_tx_rf_bandwidth_Hz(app, inst, val);
      in_situ_after_hw_write = std::make_pair(val, "Hz");
      return err;
    }
    else
    {
      return "invalid property read";
    }
    return 0;
  }

  void convert_value(std::pair<double, std::string>& value, std::string unit_desired)
  {
    std::string& unit = value.second;
    double factor = 1.;
    if    ((unit         == "dB") &&
           (unit_desired == "mdB"))
    {
      factor = 1000.;
    }
    else if((unit         == "mdB") &&
            (unit_desired == "dB"))
    {
      factor = 0.001;
    }
    else if((unit         == "MHz") &&
            (unit_desired == "Hz"))
    {
      factor = 1e6;
    }
    else if((unit         == "Hz") &&
            (unit_desired == "MHz"))
    {
      factor = 0.000001;
    }
    else if(not (((unit         == "Hz" ) &&
                  (unit_desired == "Hz" )) or
                 ((unit         == "MHz") &&
                  (unit_desired == "MHz")) or
                 ((unit         == "dB" ) &&
                  (unit_desired == "dB" )) or
                 ((unit         == "mdB") &&
                  (unit_desired == "mdB"))))
    {
      std::string err;
      err = "calculation for unit combo not supported: ";
      err += unit         + ", " + unit_desired;
      throw err.c_str();
    }
    value.first *= factor;
    unit         = unit_desired;
  }

  void dequeue_prop_writes_dependant_upon_app_inst_strings() {
    bool all_entries_processed = false;
    while(not all_entries_processed)
    {
      all_entries_processed = true;
      auto it = m_pending_prop_write_queue.begin();
      while(it != m_pending_prop_write_queue.end())
      {
        if(*it == "config")
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          config_written();
          break;
        }
        it++;
      }
    }
  }

  // this is necessary because the first write to this worker's init
  // property causes a write to the ad9361_config_proxy.rcc worker's
  // ad9361_init property, which re-initializes the AD9361 which overrides
  // all the desired initial values written to rf_gain_dB, etc. The reason
  // this happens is apparently the OpenCPI framework performs all OCS
  // initializations before the OWD ones (init is an OWD property, and
  // rf_gain_dB, etc are OCS properties). To avoid this undesirable
  // behavior , we skip all the initial writes to rf_gain_dB, etc, and
  // finally perform them here now that init property write is complete.
  // We *really* want not only the init property, but also the LO_source
  // property to be initialized before performing the calculations in
  // rf_gain_dB_written(), etc.
  void dequeue_prop_writes_dependant_upon_config() {
    bool all_entries_processed = false;
    while(not all_entries_processed)
    {
      all_entries_processed = true;
      auto it = m_pending_prop_write_queue.begin();
      while(it != m_pending_prop_write_queue.end())
      {
        if(     *it == PROP_STR_RF_GAIN_DB)
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          rf_gain_dB_written();
          break;
        }
        else if(*it == PROP_STR_BB_GAIN_DB)
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          bb_gain_dB_written();
          break;
        }
        else if(*it == PROP_STR_FREQUENCY_MHZ)
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          frequency_MHz_written();
          break;
        }
        else if(*it == PROP_STR_SAMPLE_RATE_MHZ)
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          sample_rate_MHz_written();
          break;
        }
        else if(*it == PROP_STR_RF_CUTOFF_FREQUENCY_MHZ)
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          rf_cutoff_frequency_MHz_written();
          break;
        }
        else if(*it == PROP_STR_BB_CUTOFF_FREQUENCY_MHZ)
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          bb_cutoff_frequency_MHz_written();
          break;
        }
        else if(*it == "LO_source")
        {
          all_entries_processed = false;
          m_pending_prop_write_queue.erase(it);
          LO_source_written();
          break;
        }
        it++;
      }
    }
  }

  template<typename T>
  RCCResult do_written(const std::string& prop, bool& previously_skipped) {
    LOG_WRITTEN_START();
    
    // 1. We must allow setting of the property during the "initialized" state
    //    in order to allow the default values to be applied.
    // 2. Otherwise, we only want to allow any operations when in the
    //    "operating" state.
    /*if(not (isOperating() or isInitialized()))
    {
      LOG_WRITTEN_END();
      return RCC_OK;
    }*/ //! @todo TODO/FIXME - bug is causing isOperating() to erroneously return false
    if(not m_config_initialized)
    {
      previously_skipped = true;

      log_debug("skipping %s property initialization because 'config' property has not yet been initialized", prop.c_str());
      m_pending_prop_write_queue.push_back(prop);

      LOG_WRITTEN_END();
      return RCC_OK;
    }
    if(previously_skipped)
    {
      log_debug("executing previously skipped %s property initialization because 'init' and 'LO_source' properties have been initialized", prop.c_str());
      previously_skipped = false;
    }

    const std::string unit_desired = strip_unit(prop);

    // first is value, second is value's unit description
    std::pair<double, std::string> desired;
    std::pair<T,      std::string> adjusted_to_be_applied_to_hw;
    std::pair<double, std::string> in_situ_after_hw_write;

    double& val_desired                      = desired.first;
    T&      val_adjusted_to_be_applied_to_hw = adjusted_to_be_applied_to_hw.first;

    desired = std::make_pair(*(m_map_val.at(prop)), unit_desired);

    // ensure attempted write of desired value is valid, exit otherwise
    RCCResult res = check_bounds(prop, val_desired);
    if(res != RCC_OK) { LOG_WRITTEN_END(); return res; }

    // apply step size value adjustment necessary for No-OS API call which
    // ad9361_config_proxy.rcc performs
    get_adjusted_val_to_be_applied_to_hw(prop, val_desired, adjusted_to_be_applied_to_hw);

    // apply property value to hardware
    res = set_in_situ_val(prop, val_adjusted_to_be_applied_to_hw);
    if(res != RCC_OK) { LOG_WRITTEN_END(); return res; }

    // for final logging
    if(m_properties.enable_log_info)
    {
      std::string unit;
      const char* err = get_in_situ_val(prop, in_situ_after_hw_write);
      if(err != 0) { return setError(err); }
      do_log_info_desired_vs_in_situ(prop, desired, in_situ_after_hw_write);
    }

    LOG_WRITTEN_END();
    return RCC_OK;
  }

  RCCResult do_written_no_hw(const std::string& prop, bool& previously_skipped) {
    LOG_WRITTEN_START();
    
    // 1. We must allow setting of this property during the "initialized" state
    //    in order to allow the default values to be applied.
    // 2. Otherwise, we only want to allow any operations when in the
    //    "operating" state.
    /*if(not (isOperating() or isInitialized()))
    {
      LOG_WRITTEN_END();
      return RCC_OK;
    }*/ //! @todo TODO/FIXME - bug is causing isOperating() to erroneously return false
    if(not m_config_initialized)
    {
      previously_skipped = true;
      log_debug("skipping %s property initialization because 'config' property has not yet been initialized", prop.c_str());
      LOG_WRITTEN_END();
      return RCC_OK;
    }
    if(previously_skipped)
    {
      log_debug("executing previously skipped %s property initialization because 'config' property has been initialized", prop.c_str());
      previously_skipped = false;
    }

    double val_desired = *(m_map_val.at(prop));

    // ensure attempted write of desired value is valid, exit otherwise
    RCCResult res = check_bounds(prop, val_desired);
    if(res != RCC_OK) { LOG_WRITTEN_END(); return res; }

    LOG_WRITTEN_END();
    return RCC_OK;
  }

  RCCResult do_read(const std::string& prop) {
    LOG_READ_START();
    const std::string unit_desired = strip_unit(prop);
    
    std::pair<double, std::string> in_situ;
    double& val_in_situ = in_situ.first;

    const char* err = get_in_situ_val(prop, in_situ);
    if(err != 0) { return setError(err); }

    convert_value(in_situ, unit_desired);
    *(m_map_val.at(prop)) = val_in_situ;

    log_info("current in-situ nominal %s = %.15f", prop.c_str(), val_in_situ);

    LOG_READ_END();
    return RCC_OK;
  }

  const char* calc_worst_case_Tx_RFPLL_freq_step_for_current_F_REF_and_ref_divider(
      OCPI::API::Application& app, const char* app_inst_name_proxy,
      double& val)
  {
    double d_Tx_RFPLL_input_F_REF;
    double d_Tx_RFPLL_ref_divider;

    { // restrict scope so we don't accidentally use non-double values
      // for later calculation
      bool         Tx_RFPLL_external_div_2_enable;
      ocpi_ulong_t Tx_RFPLL_input_F_REF;
      ocpi_float_t Tx_RFPLL_ref_divider;

      char* err;
      const char*& inst = app_inst_name_proxy;

      err = (char*) get_AD9361_Tx_RFPLL_external_div_2_enable(app, inst, Tx_RFPLL_external_div_2_enable);
      if(err != 0) { return err; }

      if(Tx_RFPLL_external_div_2_enable)
      {
        // in this case, the Tx PLL is fixed, so we set the step size to infinity
        val = std::numeric_limits<double>::infinity();
        return 0;
      }

      err = (char*) get_AD9361_Tx_RFPLL_input_F_REF( app, inst, Tx_RFPLL_input_F_REF );
      if(err != 0) { return err; }
      err = (char*) get_AD9361_Tx_RFPLL_ref_divider(app, inst, Tx_RFPLL_ref_divider);
      if(err != 0) { return err; }

      d_Tx_RFPLL_input_F_REF  = (double) Tx_RFPLL_input_F_REF;
      d_Tx_RFPLL_ref_divider  = (double) Tx_RFPLL_ref_divider;
    }

    double x = d_Tx_RFPLL_input_F_REF;
    x *= d_Tx_RFPLL_ref_divider; // why * and not / ???
    x /= RFPLL_MODULUS;

    // min VCO divider gives us the worst case step
    double d_Tx_RFPLL_VCO_Divider_min = 2.;
    x /= d_Tx_RFPLL_VCO_Divider_min;
    
    val = x;

    return 0;
  }

  void get_FPGA_bitstream_FB_CLK_Delay(ocpi_ushort_t& FB_CLK_Delay) {
    OCPI::API::Application &app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_data_sub;

    std::string FB_CLK_Delay_str;
    app.getProperty(inst, "FB_CLK_Delay", FB_CLK_Delay_str);
    FB_CLK_Delay = strtol(FB_CLK_Delay_str.c_str(), NULL, 0);
  }

  void get_FPGA_bitstream_TX_Data_Delay(ocpi_ushort_t& TX_Data_Delay) {
    OCPI::API::Application &app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_data_sub;

    std::string TX_Data_Delay_str;
    app.getProperty(inst, "TX_Data_Delay", TX_Data_Delay_str);
    TX_Data_Delay = strtol(TX_Data_Delay_str.c_str(), NULL, 0);
  }

  std::string get_INT_LO_str() {
    std::string INT_LO_str;
    OCPI::API::Application &app = getApplication();
    app.getProperty(m_properties.app_inst_name_ad9361_config_proxy, "INT_LO", INT_LO_str);
    return INT_LO_str;
  }

  std::string get_EXT_LO_str() {
    std::string EXT_LO_str;
    OCPI::API::Application &app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;

    app.getProperty(inst, "EXT_LO", EXT_LO_str);

    return EXT_LO_str;
  }

  std::string get_TX_1_str() {
    std::string TX_1_str;
    OCPI::API::Application &app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;

    app.getProperty(inst, "TX_1", TX_1_str);

    return TX_1_str;
  }

  std::string get_TX_2_str() {
    std::string TX_2_str;
    OCPI::API::Application &app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;

    app.getProperty(inst, "TX_2", TX_2_str);

    return TX_2_str;
  }

  // notification that rf_gain_dB property has been written
  RCCResult rf_gain_dB_written() {
    const std::string prop = PROP_STR_RF_GAIN_DB;
    static bool previously_skipped = false;
    RCCResult res = do_written<ocpi_ulong_t>(prop, previously_skipped);
    return res;
  }
  // notification that rf_gain_dB property will be read
  RCCResult rf_gain_dB_read() {
    const std::string prop = PROP_STR_RF_GAIN_DB;
    RCCResult res = do_read(prop);
    return res;
  }
  // notification that rf_gain_max_dB property has been written
  RCCResult rf_gain_max_dB_written() {
    const std::string prop("rf_gain_max_dB");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_max_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that rf_gain_min_dB property has been written
  RCCResult rf_gain_min_dB_written() {
    const std::string prop("rf_gain_min_dB");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_min_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that rf_gain_step_dB property has been written
  RCCResult rf_gain_step_dB_written() {
    const std::string prop("rf_gain_step_dB");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_step_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that bb_gain_dB property has been written
  RCCResult bb_gain_dB_written() {
    const std::string prop(PROP_STR_BB_GAIN_DB);
    static bool previously_skipped = false;
    RCCResult res = do_written_no_hw(prop, previously_skipped);
    return res;
  }
  // notification that bb_gain_max_dB property has been written
  RCCResult bb_gain_max_dB_written() {
    const std::string prop("bb_gain_max_dB");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_max_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that bb_gain_min_dB property has been written
  RCCResult bb_gain_min_dB_written() {
    const std::string prop("bb_gain_min_dB");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_min_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that bb_gain_step_dB property has been written
  RCCResult bb_gain_step_dB_written() {
    const std::string prop("bb_gain_step_dB");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_step_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that frequency_MHz property has been written
  RCCResult frequency_MHz_written() {
    const std::string prop = PROP_STR_FREQUENCY_MHZ;
    static bool previously_skipped = false;
    RCCResult res = do_written<ocpi_ulonglong_t>(prop, previously_skipped);
    return res;
  }
  // notification that frequency_MHz property will be read
  RCCResult frequency_MHz_read() {
    const std::string prop = PROP_STR_FREQUENCY_MHZ;
    RCCResult res = do_read(prop);
    return res;
  }
  // notification that frequency_max_MHz property has been written
  RCCResult frequency_max_MHz_written() {
    const std::string prop("frequency_max_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_max_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that frequency_min_MHz property has been written
  RCCResult frequency_min_MHz_written() {
    const std::string prop("frequency_min_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_min_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that frequency_step_MHz property has been written
  RCCResult frequency_step_MHz_written() {
    // we would call error_if_prop_val_is_not_prop_fixedval() if there was a
    // default value for this property, but there isn't so writes to this
    // property should NEVER be supported
    return setError("writes to frequency_step_MHz are invalid");
  }
  // notification that frequency_step_MHz property will be read
  RCCResult frequency_step_MHz_read() {
    log_trace("start of frequency_step_MHz_read()");
    double step_Hz;

    // Note that for the sample rate calculation, it was decided to treat
    // CLKTF_FREQ as the effective AD9361 sample rate, and not the TX_SAMPL_FREQ
    OCPI::API::Application& app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;

    //const char* r = get_in_situ_Tx_RFPLL_freq_step(app, step_Hz);
    //if(r != 0) { return setError(r); }
    //log_info("current in-situ nominal TX RF LO step size = %.15f Hz", step_Hz);
    
    // we use this function instead of get_in_situ_Tx_RFPLL_freq_step()
    // because, after observation of how No-OS choses divider values, No-OS's
    // precision at lower TX RF LO frequencies is nowhere near the theoretical
    // precision of the AD9361 dividers :( - it was observed that every step
    // size was at or below the theoretical worst case value
    const char* r = calc_worst_case_Tx_RFPLL_freq_step_for_current_F_REF_and_ref_divider(app, inst, step_Hz);
    if(r != 0) { return setError(r); }
    log_info("using worst-case in-situ nominal TX RF LO step size = %.15f Hz", step_Hz);

    m_properties.frequency_step_MHz = step_Hz/1e6;

    log_trace("end   of frequency_step_MHz_read()");
    return RCC_OK;
  }
  // notification that sample_rate_MHz property has been written
  RCCResult sample_rate_MHz_written() {
    const std::string prop(PROP_STR_SAMPLE_RATE_MHZ);
    static bool previously_skipped = false;
    RCCResult res = do_written<ocpi_ulong_t>(prop, previously_skipped);
    return res;
  }
  // notification that sample_rate_MHz property will be read
  RCCResult sample_rate_MHz_read() {
    const std::string prop(PROP_STR_SAMPLE_RATE_MHZ);
    RCCResult res = do_read(prop);
    return res;
  }
  // notification that sample_rate_max_MHz property has been written
  RCCResult sample_rate_max_MHz_written() {
    const std::string prop("sample_rate_max_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_max_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that sample_rate_min_MHz property has been written
  RCCResult sample_rate_min_MHz_written() {
    const std::string prop("sample_rate_min_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_min_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that sample_rate_step_MHz property has been written
  RCCResult sample_rate_step_MHz_written() {
    // we would call error_if_prop_val_is_not_prop_fixedval() if there was a
    // default value for this property, but there isn't so writes to this
    // property should NEVER be supported
    return setError("writes to sample_rate_step_MHz are invalid");
  }
  // notification that sample_rate_step_MHz property will be read
  RCCResult sample_rate_step_MHz_read() {
    log_trace("start of sample_rate_step_MHz_read()");
    double step_Hz;

    // Note that for the sample rate calculation, it was decided to treat
    // CLKTF_FREQ as the "effective" AD9361 sample rate, and not the
    // TX_SAMPL_FREQ
    OCPI::API::Application& app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    const char* r = get_AD9361_CLKTF_FREQ_step_Hz(app, inst, step_Hz);
    if(r != 0) { return setError(r); }

    if(step_Hz < m_No_OS_API_precision_tx_sampling_freq) 
    {
      log_debug("current in-situ nominal TX sample rate step size = %.15f Hz (not achievable due to No-OS API)", step_Hz);
      step_Hz = m_No_OS_API_precision_tx_sampling_freq;
      log_info("current in-situ nominal TX sample rate step size = %.15f Hz (limited by precision of No-OS API, which is less precise than the precision of the AD9361 register set limitations)", step_Hz);
    }
    else
    {
      log_info("current in-situ nominal TX sample rate step size = %.15f Hz (limited by precision of AD9361 register set)", step_Hz);
    }
    m_properties.sample_rate_step_MHz = step_Hz/1e6;

    log_trace("end   of sample_rate_step_MHz_read()");
    return RCC_OK;
  }
  // notification that rf_cutoff_frequency_MHz property has been written
  RCCResult rf_cutoff_frequency_MHz_written() {
    const std::string prop(PROP_STR_RF_CUTOFF_FREQUENCY_MHZ);
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_fixed_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that rf_cutoff_frequency_max_MHz property has been written
  RCCResult rf_cutoff_frequency_max_MHz_written() {
    const std::string prop("rf_cutoff_frequency_max_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_max_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that rf_cutoff_frequency_min_MHz property has been written
  RCCResult rf_cutoff_frequency_min_MHz_written() {
    const std::string prop("rf_cutoff_frequency_min_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_min_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that rf_cutoff_frequency_step_MHz property has been written
  RCCResult rf_cutoff_frequency_step_MHz_written() {
    const std::string prop("rf_cutoff_frequency_step_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_step_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that bb_cutoff_frequency_MHz property has been written
  RCCResult bb_cutoff_frequency_MHz_written() {
    const std::string prop(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ);
    static bool previously_skipped = false;
    //RCCResult res = do_written<float>(prop, previously_skipped);
    RCCResult res = do_written<ocpi_ulong_t>(prop, previously_skipped);
    return res;
  }
  // notification that bb_cutoff_frequency_MHz property will be read
  RCCResult bb_cutoff_frequency_MHz_read() {
    const std::string prop(PROP_STR_BB_CUTOFF_FREQUENCY_MHZ);
    RCCResult res = do_read(prop);
    return res;
  }
  // notification that bb_cutoff_frequency_max_MHz property has been written
  RCCResult bb_cutoff_frequency_max_MHz_written() {
    const std::string prop("bb_cutoff_frequency_max_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_max_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that bb_cutoff_frequency_min_MHz property has been written
  RCCResult bb_cutoff_frequency_min_MHz_written() {
    const std::string prop("bb_cutoff_frequency_min_MHz");
    // because this property has a default value, we need to support the initial
    // write of the default value that occurs during initialization, however,
    // because it is writable we need to call
    // error_if_prop_val_is_not_prop_fixedval() to ensure end users never write
    // an invalid value
    RCCResult res = error_if_min_prop_val_is_not_prop_fixedval(prop);
    return res;
  }
  // notification that bb_cutoff_frequency_step_MHz property has been written
  RCCResult bb_cutoff_frequency_step_MHz_written() {
    // we would call error_if_prop_val_is_not_prop_fixedval() if there was a
    // default value for this property, but there isn't so writes to this
    // property should NEVER be supported
    return setError("writes to bb_cutoff_frequency_step_MHz are invalid");
  }
  // notification that bb_cutoff_frequency_step_MHz property will be read
  RCCResult bb_cutoff_frequency_step_MHz_read() {
    //! @todo TODO/FIXME - implement this calculation
    // just set to -1 for now
    m_properties.bb_cutoff_frequency_step_MHz = FMCOMMS_2_3_TX_NOT_SUPPORTED_P;

    return RCC_OK;
  }
  // notification that app_inst_name_ad9361_config_proxy property has been written
  RCCResult app_inst_name_ad9361_config_proxy_written() {
    m_app_inst_name_ad9361_config_proxy_initialized = true;

    if(m_app_inst_name_ad9361_config_proxy_initialized and
       m_app_inst_name_ad9361_data_sub_initialized)
    {
      dequeue_prop_writes_dependant_upon_app_inst_strings();
    }

    return RCC_OK;
  }
  // notification that app_inst_name_ad9361_data_sub property has been written
  RCCResult app_inst_name_ad9361_data_sub_written() {
    m_app_inst_name_ad9361_data_sub_initialized = true;

    if(m_app_inst_name_ad9361_config_proxy_initialized and
       m_app_inst_name_ad9361_data_sub_initialized)
    {
      dequeue_prop_writes_dependant_upon_app_inst_strings();
    }

    return RCC_OK;
  }//
  // notification that config property has been written
  RCCResult config_written() {
    log_trace("start of config_written()");
    const std::string prop("config");

    static bool previously_skipped = false;

    if(not (m_app_inst_name_ad9361_config_proxy_initialized and
            m_app_inst_name_ad9361_data_sub_initialized))
    {
      previously_skipped = true;

      log_debug("skipping %s property initialization because app_inst_name* properties have not yet been initialized", prop.c_str());
      m_pending_prop_write_queue.push_back(prop);

      log_trace("end   of config_written()");
      return RCC_OK;
    }
    if(previously_skipped)
    {
      log_debug("executing previously skipped %s property initialization because app_inst_name* properties have been initialized", prop.c_str());
      previously_skipped = false;
    }

    // write desired values to ad9361_config_proxy's ad9361_init function,
    // but only change the values of the struct members we care about so as to
    // avoid potentially stepping on anyone else's toes
    
    std::string pstr;
    try
    {
      OCPI::API::Application &app = getApplication();
      app.getProperty(m_properties.app_inst_name_ad9361_config_proxy, "ad9361_init", pstr);
    }
    catch(const std::exception& e)
    {
      std::string err;
      err = "Exception when reading ad9361_config_proxy property: ";
      err += e.what();
      return setError(err.c_str());
    }

    ad9361_config_proxy_ad9361_init ad9361_init;
    const char* retc = parse(pstr.c_str(), ad9361_init);
    if(retc != 0) { return setError(retc); }

    ad9361_init.reference_clk_rate =
        round(m_properties.config.reference_clk_rate_Hz); // will be later applied

    uint8_t FDD_enable = (m_properties.config.duplex_mode == CONFIG_DUPLEX_MODE_TDD)
                         ? 0 : 1;
    ad9361_init.frequency_division_duplex_mode_enable = FDD_enable; // will be later applied

    if(m_properties.config.are_using_REF_CLK_SMA)
    {
      ad9361_init.xo_disable_use_ext_refclk_enable = 1; // will be later applied
    }
    else
    {
      ad9361_init.xo_disable_use_ext_refclk_enable = 0; // will be later applied
    }

    // no I/Q swap for TX channel(s)
    ad9361_init.pp_tx_swap_enable = (ocpi_uchar_t) 0;

    // See Table 1: Channel Connectivity in AD9361 DAC Sub Component Data Sheet
    //
    if(m_properties.config.SMA_channel == CONFIG_SMA_CHANNEL_TX1A)
    {
      // (this is what performs no channel swap)
      ad9361_init.tx_channel_swap_enable = 0; // will be later applied
      // (this is what ensures desired channel TX_1 is enabled)
      std::string TX_1_str = get_TX_1_str();
      long int TX_1_i = strtol(TX_1_str.c_str(), NULL, 0);
      ad9361_init.one_rx_one_tx_mode_use_tx_num = (uint8_t) (TX_1_i & 0xff); // will be later applied
    }
    else // i.e. (m_properties.config.SMA_channel == CONFIG_SMA_CHANNEL_TX2A)
    {
      // (this is what performs channel swap)
      ad9361_init.tx_channel_swap_enable = 0; // will be later applied
      // (this is what ensures desired channel TX_2 is enabled)
      std::string TX_2_str = get_TX_2_str();
      long int TX_2_i = strtol(TX_2_str.c_str(), NULL, 0);
      ad9361_init.one_rx_one_tx_mode_use_tx_num = (uint8_t) (TX_2_i & 0xff); // will be later applied
    }

    // Because our design approach was to build the FPGA bitstream based on
    // assumed values for the on-AD9361 delays, those delays are built into
    // the bitstream metadata (as parameter properties for the
    // ad9361_data_sub.hdl worker). Here is where those assumptions are forced
    // to be true by reading the assumed values from the bitstream's metadata
    // (and values are later applied via the ad9361_config_proxy.rcc worker's
    // ad9361_init property).


    ocpi_ushort_t                     FB_CLK_Delay;
    get_FPGA_bitstream_FB_CLK_Delay(  FB_CLK_Delay  );
    ad9361_init.tx_fb_clock_delay   = FB_CLK_Delay;   // will be later applied

    ocpi_ushort_t                     TX_Data_Delay;
    get_FPGA_bitstream_TX_Data_Delay( TX_Data_Delay );
    ad9361_init.tx_data_delay       = TX_Data_Delay;  // will be later applied

    // values are finally applied
    try
    {
      set_ad9361_init(ad9361_init);
    }
    catch(const std::exception& e)
    {
      std::string err;
      err = "Exception when setting ad9361_config_proxy property: ";
      err += e.what();

      log_trace("end   of config_written()");
      return setError(err.c_str());
    }
    m_config_initialized = true;

    //! @todo TODO/FIXME - remove disabling of rx FIR and add FIR support paradigm
    disable_TX_FIR_filter();

    OCPI::API::Application& app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    std::string DISABLE_str;
    app.getProperty(inst, "DISABLE",               DISABLE_str);
    app.setProperty(inst, "en_state_machine_mode", DISABLE_str.c_str());

    dequeue_prop_writes_dependant_upon_config();

    log_trace("end   of config_written()");
    return RCC_OK;
  }
  // notification that config property will be read
  RCCResult config_read() {

    OCPI::API::Application& app = getApplication();
    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;

    // m_properties.config.reference_clk_rate_Hz is assumed to be what we set it
    // to, we normally would read the ad9361_config_proxy.rcc workers'
    // pdata.clk_refin.rate property but we refrain since the value initially
    // set to this worker's init.reference_clk_rate_Hz property has more
    // precision

    AD9361_duplex_mode_t mode;
    const char* err1 = get_AD9361_duplex_mode(app, inst, mode);
    if(err1 != 0) { return setError(err1); }
    m_properties.config.duplex_mode = (mode == AD9361_duplex_mode_t::FDD) ? CONFIG_DUPLEX_MODE_FDD : CONFIG_DUPLEX_MODE_FDD;

    bool use_extclk;
    const char* err2 = get_AD9361_use_extclk(app, inst, use_extclk);
    if(err2 != 0) { return setError(err2); }
    m_properties.config.are_using_REF_CLK_SMA = use_extclk;

    return RCC_OK;
  }
  // notification that LO_source property has been written
  RCCResult LO_source_written() {
    log_trace("start of LO_source_written()");
    const std::string prop("LO_source");

    static bool previously_skipped = false;

    if(not m_config_initialized)
    {
      previously_skipped = true;

      m_pending_prop_write_queue.push_back(prop);

      log_trace("end of LO_source_written()");
      return RCC_OK;
    }
    if(previously_skipped)
    {
      log_debug("executing previously skipped %s property initialization because app_inst_name* properties have been initialized", prop.c_str());
      previously_skipped = false;
    }

    std::string INT_LO_str = get_INT_LO_str();
    std::string EXT_LO_str = get_EXT_LO_str();
    const std::string valstr = (m_properties.LO_source == LO_SOURCE_INTERNAL)
                               ? INT_LO_str : EXT_LO_str;

    OCPI::API::Application& app = getApplication();

    const char* inst = m_properties.app_inst_name_ad9361_config_proxy;
    app.setProperty(inst, "tx_lo_int_ext", valstr.c_str());

    log_trace("end of LO_source_written()");
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE; // change this as needed for this worker to do something useful
    // return RCC_ADVANCE; when all inputs/outputs should be advanced each time "run" is called.
    // return RCC_ADVANCE_DONE; when all inputs/outputs should be advanced, and there is nothing more to do.
    // return RCC_DONE; when there is nothing more to do, and inputs/outputs do not need to be advanced.
  }
};

FMCOMMS_2_3_TX_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
FMCOMMS_2_3_TX_END_INFO
