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

#ifndef _READERS_AD9361_RX_FILTERS_ANALOG
#define _READERS_AD9361_RX_FILTERS_ANALOG

#include <cstdint>    // uint16_t
#include <cstdlib>    // strtol()
#include "OcpiApi.hh" // OCPI::API namespace

/*! @brief Get the in-situ value with exact precision of the
 *         Rx BaseBand Filter Tune Divide
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
const char* get_AD9361_Rx_BBF_Tune_Divide(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    uint16_t& val)
{
  std::string enum_str;
  app.getProperty(app_inst_name_proxy, "Rx_BBF_Tune_Divide", enum_str);

  // strtol() documentation - "If no valid conversion could be performed, a zero
  // value is returned (0L)."
  long int tmp = strtol(enum_str.c_str(), NULL, 0);

  // note that 0 is not a valid possible value for Rx_BBF_Tune_Divider
  if(tmp != 0L)
  {
    val = tmp;
  }
  else
  {
    std::string err;
    err = "Invalid value read for ad9361_config_proxy.rcc ";
    err += "Rx_BBF_Tune_Divide property: " + enum_str;
    return err.c_str();
  }
  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         Rx BBBW in Hz
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
const char* get_AD9361_Rx_BBBW_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  //! @todo TODO/FIXME - this calculation may not be 100% accurate - figure out which of the multiple formulas for Rx BBBW are accurate (see AD9361 registers 0x1F8, 0x1FB, 0x1FC)
  double d_BBPLL_FREQ_Hz;
  double d_Rx_BBF_Tune_Divide;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double   BBPLL_FREQ_Hz;
    uint16_t Rx_BBF_Tune_Divide;

    char* ret;

    const char* inst = app_inst_name_proxy;
    ret = (char*) get_AD9361_BBPLL_FREQ_Hz(     app, inst, BBPLL_FREQ_Hz     );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Rx_BBF_Tune_Divide(app, inst, Rx_BBF_Tune_Divide);
    if(ret != 0) { return ret; }

    d_BBPLL_FREQ_Hz      = (double) BBPLL_FREQ_Hz;
    d_Rx_BBF_Tune_Divide = (double) Rx_BBF_Tune_Divide;
  }

  // AD9361_Reference_Manual_UG-570.pdf p. 9
  // "The Rx filter... is normally calibrated to 1.4x the baseband channel
  // bandwidth (BBBW)."
  // BBBW_ACTUAL,MHz = BBPLL_MHz * ln(2) / (2.8 * pi * Divider)
  const double PI = 3.141592653589793;
  val = d_BBPLL_FREQ_Hz*std::log(2.)/(1.4*2.*PI*(double)d_Rx_BBF_Tune_Divide);

  return 0;
}

/*! @brief Get the nominal in-situ value with No-OS point precision
 *         of the
 *         rx_rf_bandwidth in Hz
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
const char* get_AD9361_rx_rf_bandwidth_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_ulong_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "rx_rf_bandwidth");
  val = p.getULongValue();

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         rx_rf_bandwidth in Hz
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
const char* get_AD9361_rx_rf_bandwidth_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  char* ret;
  const char* inst = app_inst_name_proxy;

  ocpi_ulong_t val_precast;
  ret = (char*) get_AD9361_rx_rf_bandwidth_Hz(app, inst, val_precast);
  val = (double) val_precast;
  return ret;
}

#endif // _READERS_AD9361_RX_FILTERS_ANALOG
