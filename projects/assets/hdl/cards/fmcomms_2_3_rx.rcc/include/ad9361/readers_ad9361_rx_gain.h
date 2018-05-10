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

#ifndef _READERS_AD9361_RX_GAIN_H
#define _READERS_AD9361_RX_GAIN_H

/*! @file
 *  @brief Provides functions for reading in-situ Receive (RX) gain-related
 *         values from an operating AD9361 IC using an OpenCPI application.
 ******************************************************************************/

#include "ocpi_component_prop_type_helpers.h" // ocpi_long_t, ocpi_ulonglong_t types
#include "OcpiApi.hh"                  // OCPI::API namespace

/*! @brief Get the nominal in-situ value with exact precision
 *         of the
 *         Rx gain in dB
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
const char* get_AD9361_rx_gain_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_long_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "rx_rf_gain");
  val = p.getLongValue();

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         Rx gain in dB
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
const char* get_AD9361_rx_gain_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  char* ret;
  const char* inst = app_inst_name_proxy;

  ocpi_long_t val_precast;
  ret = (char*) get_AD9361_rx_gain_dB(app, inst, val_precast);
  val = (double) val_precast;

  return ret;
}

/*! @brief Get the nominal in-situ value with exact precision
 *         of the
 *         current minimum allowable Rx gain in dB
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
const char* get_AD9361_rx_gain_min_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    int32_t& val)
{
  // I *think* this is the value that is equivalent to calling
  // clk_get_rate(phy, phy->ref_clk_scale[RX_RFPLL]) inside No-OS
  // (which is what the RX gain tables appear to be based off of)
  OCPI::API::Property p(app, app_inst_name_proxy, "rx_lo_freq");
  ocpi_ulonglong_t f = p.getULongLongValue();

  if(f <= 1.3e9) // see ad9361.c ad9361_gt_tableindex()
  {
    val = 1; // see ad9361.c ad9361_init_gain_tables()
    return 0;
  }
  if(f <= 4e9) // see ad9361.c ad9361_gt_tableindex()
  {
    val = -3; // see ad9361.c ad9361_init_gain_tables()
    return 0;
  }
  // 4e9 < Rx_RFPLL_freq_Hz <= 6e9 // see ad9361.c ad9361_gt_tableindex()
  val = -10; // see ad9361.c ad9361_init_gain_tables()
  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         current minimum allowable Rx gain in dB
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
const char* get_AD9361_rx_gain_min_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double & val)
{
  char* ret;
  const char* inst = app_inst_name_proxy;

  int32_t val_precast;
  ret = (char*) get_AD9361_rx_gain_min_dB(app, inst, val_precast);
  val = (double) val_precast;
  
  return ret;
}

/*! @brief Get the nominal in-situ value with exact precision
 *         of the
 *         current maximum allowable Rx gain in dB
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
const char* get_AD9361_rx_gain_max_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    int32_t& val)
{
  // I *think* this is the value that is equivalent to calling
  // clk_get_rate(phy, phy->ref_clk_scale[RX_RFPLL]) inside No-OS
  // (which is what the RX gain tables appear to be based off of)
  OCPI::API::Property p(app, app_inst_name_proxy, "rx_lo_freq");
  ocpi_ulonglong_t f = p.getULongLongValue();

  if(f <= 1.3e9) // see ad9361.c ad9361_gt_tableindex()
  {
    val = 77; // see ad9361.c ad9361_init_gain_tables()
    return 0;
  }
  if(f <= 4e9) // see ad9361.c ad9361_gt_tableindex()
  {
    val = 71; // see ad9361.c ad9361_init_gain_tables()
    return 0;
  }
  // 4e9 < Rx_RFPLL_freq_Hz <= 6e9 // see ad9361.c ad9361_gt_tableindex()
  val = 62; // see ad9361.c ad9361_init_gain_tables()
  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         current maximum allowable Rx gain in dB
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
const char* get_AD9361_rx_gain_max_dB(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double & val)
{
  char* ret;
  const char* inst = app_inst_name_proxy;

  int32_t val_precast;
  ret = (char*) get_AD9361_rx_gain_max_dB(app, inst, val_precast);
  val = (double) val_precast;
  
  return ret;
}

#endif // _READERS_AD9361_RX_GAIN_H
