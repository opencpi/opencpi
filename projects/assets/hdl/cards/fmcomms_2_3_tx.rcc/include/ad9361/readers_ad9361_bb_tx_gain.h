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

#ifndef _READERS_AD9361_TX_GAIN_H
#define _READERS_AD9361_TX_GAIN_H

/*! @file
 *  @brief Provides functions for reading in-situ Transmit (TX) gain-related
 *         values from an operating AD9361 IC using an OpenCPI application.
 ******************************************************************************/

#include "worker_prop_parsers_ad9361_config_proxy.h" // ad9361_config_proxy_tx_attenuation_t type, parse()
#include "OcpiApi.hh"      // OA namespace

namespace OA = OCPI::API;

/*! @brief Get the in-situ value with exact precision of the
 *         Tx attenuation in mdB for the TX1 channel
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 ******************************************************************************/
void get_AD9361_tx_attenuation_TX1_mdB(
    OA::Application& app, const char* app_inst_name_proxy,
    OA::ULong& val)
{
  std::string str;
  app.getProperty(app_inst_name_proxy, "tx_attenuation", str);

  ad9361_config_proxy_tx_attenuation_t tx_attenuation;

  parse(str.c_str(), tx_attenuation);

  val = tx_attenuation[0]; // 0 corresponds to TX1
}

/*! @brief Get the in-situ value with exact precision of the
 *         Tx attenuation in mdB for the TX2 channel
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 ******************************************************************************/
void get_AD9361_tx_attenuation_TX2_mdB(
    OA::Application& app, const char* app_inst_name_proxy,
    OA::ULong& val)
{
  std::string str;
  app.getProperty(app_inst_name_proxy, "tx_attenuation", str);

  ad9361_config_proxy_tx_attenuation_t tx_attenuation;

  parse(str.c_str(), tx_attenuation);

  val = tx_attenuation[1]; // 1 corresponds to TX2
}

/*! @brief Get the in-situ value with double floating point precision of the
 *         Tx attenuation in mdB for the TX1 channel
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 ******************************************************************************/
void get_AD9361_tx_attenuation_TX1_mdB(
    OA::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  const char* inst = app_inst_name_proxy;

  OA::ULong val_precast;
  get_AD9361_tx_attenuation_TX1_mdB(app, inst, val_precast);
  val = (double) val_precast;
}

/*! @brief Get the in-situ value with double floating point precision of the
 *         Tx attenuation in mdB for the TX2 channel
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 ******************************************************************************/
void get_AD9361_tx_attenuation_TX2_mdB(
    OA::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  const char* inst = app_inst_name_proxy;

  OA::ULong val_precast;
  get_AD9361_tx_attenuation_TX2_mdB(app, inst, val_precast);
  val = (double) val_precast;
}

#endif // _READERS_AD9361_TX_GAIN_H
