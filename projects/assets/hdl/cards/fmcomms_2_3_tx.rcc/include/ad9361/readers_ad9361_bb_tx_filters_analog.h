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

#ifndef _READERS_AD9361_TX_FILTERS_ANALOG
#define _READERS_AD9361_TX_FILTERS_ANALOG

#include <cmath>      // std::log
#include <string>     //std::string
#include "OcpiApi.hh" // OCPI::API::Application class

/*! @brief Get the in-situ value with exact precision of the
 *         Tx BaseBand Filter Tune Divider
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_Tx_BBF_Tune_Divider(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    uint16_t& val)
{
  std::string enum_str;
  app.getProperty(app_inst_name_proxy, "Tx_BBF_Tune_Divider", enum_str);

  // strtol() documentation - "If no valid conversion could be performed, a zero
  // value is returned (0L)."
  long int tmp = strtol(enum_str.c_str(), NULL, 0);

  // note that 0 is not a valid possible value for Tx_BBF_Tune_Divider
  if(tmp != 0L)
  {
    val = tmp;
  }
  else
  {
    std::string err;
    err = "Invalid value read for ad9361_config_proxy.rcc ";
    err += "Tx_BBF_Tune_Divider property: " + enum_str;
    return err.c_str();
  }
  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the 
 *         Tx BBBW in Hz
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_Tx_BBBW_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_BBPLL_FREQ_Hz;
  double d_Tx_BBF_Tune_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double   BBPLL_FREQ_Hz;
    uint16_t Tx_BBF_Tune_Divider;

    char* ret;

    const char* inst = app_inst_name_proxy;
    ret = (char*) get_AD9361_BBPLL_FREQ_Hz(      app, inst, BBPLL_FREQ_Hz      );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_BBF_Tune_Divider(app, inst, Tx_BBF_Tune_Divider);
    if(ret != 0) { return ret; }

    d_BBPLL_FREQ_Hz      = (double) BBPLL_FREQ_Hz;
    d_Tx_BBF_Tune_Divider = (double) Tx_BBF_Tune_Divider;
  }

  // AD9361_Reference_Manual_UG-570.pdf p. 10
  // BBBW_ACTUAL,MHz = BBPLL_MHz * ln(2) / (3.2 * pi * Divider)
  const double PI = 3.141592653589793;
  val = d_BBPLL_FREQ_Hz*std::log(2.)/(1.6*2.*PI*(double)d_Tx_BBF_Tune_Divider);

  return 0;
}

//! @todo TODO/FIXME - report *EXACT* nominal 3dB cutoff freq
  
const char* get_AD9361_tx_filter_complex_bandwidth_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double Tx_BBBW_Hz;
  const char* inst = app_inst_name_proxy;
  const char* err = get_AD9361_Tx_BBBW_Hz(app, inst, Tx_BBBW_Hz);

  // AD9361_Reference_Manual_UG-570.pdf p. 10 "The baseband Tx analog filter
  // calibration tunes the cutoff frequency of the third-order Butterworth
  // Tx anti-imaging filter. The TX filter ... is normally calibrated to
  // 1.6x the BBBW. Note that BBBW is half the complex bandwidth..."
  val = 1.6*(Tx_BBBW_Hz*2);

  return err;
}

/*! @brief Calculate AD9361 Tx BBBW step size down (i.e. towards 0) in Hz with double floating
 *         point precision. The following precision is assumed for dependent
 *         parameters (note that each of these parameters are converted to
 *         double floats before performing calculation):
 *          * double   BBPLL_freq
 *          * uint16_t Tx_BBF_Tune_Divider
 *
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_Tx_BBBW_step_down(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_BBPLL_freq;
  double d_Tx_BBF_Tune_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double   BBPLL_freq;
    uint16_t Tx_BBF_Tune_Divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_BBPLL_FREQ_Hz(      app, inst, BBPLL_freq         );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_BBF_Tune_Divider(app, inst, Tx_BBF_Tune_Divider);
    if(ret != 0) { return ret; }

    d_BBPLL_freq          = (double) BBPLL_freq;
    d_Tx_BBF_Tune_Divider = (double) Tx_BBF_Tune_Divider;
  }

  // AD9361_Reference_Manual_UG-570.pdf p. 10
  // BBBW_ACTUAL,MHz = BBPLL_MHz * ln(2) / (3.2 * pi * Divider)
  // 
  // BBBW_step_down = (BBPLL_MHz * ln(2) / (3.2 * pi * (Divider  ))) - (BBPLL_MHz * ln(2) / (3.2 * pi * (Divider+1)))
  //                = BBPLL_MHz * ln(2) / (3.2 * pi ) * ((1/Divider) - (1/(Divider+1)))
  //                = BBPLL_MHz * ln(2) / (3.2 * pi ) * (1/Divider/(Divider+1))
  const double PI = 3.141592653589793;
  val = d_BBPLL_freq*std::log(2.)/(1.6*2.*PI) * (1/d_Tx_BBF_Tune_Divider/(d_Tx_BBF_Tune_Divider+1));

  return 0;
}

/*! @brief Calculate AD9361 Tx BBBW step size up (i.e. towards inf) in Hz with double floating
 *         point precision. The following precision is assumed for dependent
 *         parameters (note that each of these parameters are converted to
 *         double floats before performing calculation):
 *          * double   BBPLL_freq
 *          * uint16_t Tx_BBF_Tune_Divider
 *
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_Tx_BBBW_step_up(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_BBPLL_freq;
  double d_Tx_BBF_Tune_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double   BBPLL_freq;
    uint16_t Tx_BBF_Tune_Divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_BBPLL_FREQ_Hz(      app, inst, BBPLL_freq         );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_BBF_Tune_Divider(app, inst, Tx_BBF_Tune_Divider);
    if(ret != 0) { return ret; }

    d_BBPLL_freq          = (double) BBPLL_freq;
    d_Tx_BBF_Tune_Divider = (double) Tx_BBF_Tune_Divider;
  }

  // AD9361_Reference_Manual_UG-570.pdf p. 10
  // BBBW_ACTUAL,MHz = BBPLL_MHz * ln(2) / (3.2 * pi * Divider)
  // 
  // BBBW_step_up   = (BBPLL_MHz * ln(2) / (3.2 * pi * (Divider-1))) - (BBPLL_MHz * ln(2) / (3.2 * pi * (Divider  )))
  //                = BBPLL_MHz * ln(2) / (3.2 * pi ) * ((1/Divider) - (1/(Divider-1)))
  //                = BBPLL_MHz * ln(2) / (3.2 * pi ) * (1/Divider/(Divider-1))
  const double PI = 3.141592653589793;
  val = d_BBPLL_freq*std::log(2.)/(1.6*2.*PI) * (1/d_Tx_BBF_Tune_Divider/(d_Tx_BBF_Tune_Divider-1));

  return 0;
}

/*! @brief Get the nominal in-situ value with No-OS point precision
 *         of the
 *         tx_rf_bandwidth in Hz
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_tx_rf_bandwidth_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_ulong_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "tx_rf_bandwidth");
  val = p.getULongValue();

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         tx_rf_bandwidth in Hz
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_tx_rf_bandwidth_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  char* ret;
  const char* inst = app_inst_name_proxy;

  ocpi_ulong_t val_precast;
  ret = (char*) get_AD9361_tx_rf_bandwidth_Hz(app, inst, val_precast);
  val = (double) val_precast;
  return ret;
}


#endif // _READERS_AD9361_TX_FILTERS_ANALOG
