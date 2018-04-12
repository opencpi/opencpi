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

#ifndef _WRITERS_AD9361_RX_GAIN_H
#define _WRITERS_AD9361_RX_GAIN_H

/*! @file
 *  @brief Provides functions for writing in-situ Receive (RX) gain-related
 *         values to an operating AD9361 IC using an OpenCPI application.
 ******************************************************************************/

#include "OcpiApi.hh" // OCPI::API namespace
#include "ocpi_component_prop_type_helpers.h" // ocpi_long_t type
#include "worker_prop_parsers_ad9361_config_proxy.h" // parse()

/*! @brief Set the nominal in-situ value with exact precision
 *         of the
 *         Rx gain for the RX1 channel in dB
 *         to an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[in]  val                 Value to assign.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* set_AD9361_gain_RX1_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    const ocpi_long_t& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "rx_rf_gain", vstr);

  char* ret;
  ad9361_config_proxy_rx_rf_gain_t rx_rf_gain;

  ret = (char*) parse(vstr.c_str(), rx_rf_gain);
  if(ret != 0) { return ret; }

  rx_rf_gain[0] = val; // 0 corresponds to RX1

  std::string rx_rf_gain_str = to_string(rx_rf_gain);
  app.setProperty(app_inst_name_proxy, "rx_rf_gain", rx_rf_gain_str.c_str());

  return 0;
}

/*! @brief Set the nominal in-situ value with exact precision
 *         of the
 *         Rx gain for the RX2 channel in dB
 *         to an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[in]  val                 Value to assign.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* set_AD9361_gain_RX2_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    const ocpi_long_t& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "rx_rf_gain", vstr);

  char* ret;
  ad9361_config_proxy_rx_rf_gain_t rx_rf_gain;

  ret = (char*) parse(vstr.c_str(), rx_rf_gain);
  if(ret != 0) { return ret; }

  rx_rf_gain[1] = val; // 1 corresponds to RX2

  std::string rx_rf_gain_str = to_string(rx_rf_gain);
  app.setProperty(app_inst_name_proxy, "rx_rf_gain", rx_rf_gain_str.c_str());

  return 0;
}

/*! @brief Set the nominal in-situ setting
 *         of the
 *         Rx gain control mode
 *         to an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[in]  val                 Value to assign.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* set_AD9361_rx_gain_control_mode(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    const ad9361_config_proxy_rx_gain_control_mode_t& val)
{
  std::string val_str = to_string(val);
  
  const char* inst = app_inst_name_proxy;
  app.setProperty(inst, "rx_gain_control_mode", val_str.c_str());

  return 0;
}

#endif // _WRITERS_AD9361_RX_GAIN_H
