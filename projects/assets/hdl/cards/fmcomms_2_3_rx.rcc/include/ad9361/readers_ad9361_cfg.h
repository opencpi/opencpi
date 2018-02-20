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

#ifndef _READERS_AD9361_CFG_H
#define _READERS_AD9361_CFG_H

/*! @file readers_ad9361_bbpll.h
 *  @brief Provides functions for reading in-situ configuration setting
 *         value from an operating AD9361 IC using an OpenCPI
 *         application.
 ******************************************************************************/
 
#include "OcpiApi.hh" // OCPI::API namespace
#include <sstream>    // std::ostringstream
#include <string>     // std::string
#include "worker_prop_parsers_ad9361_config_proxy.h" // parse()
#include "ad9361_common.h" // AD9361_duplex_mode_t, AD9361_one_rx_one_tx_mode_use_rx_num_t

/*! @brief Get the in-situ value
 *         of the
 *         duplex mode setting
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
const char* get_AD9361_duplex_mode(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    AD9361_duplex_mode_t& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "ad9361_rf_phy", vstr);

  char* ret;
  struct ad9361_config_proxy_ad9361_rf_phy ad9361_rf_phy;

  ret = (char*) parse(vstr.c_str(), ad9361_rf_phy);
  if(ret != 0) { return ret; }

  val = ad9361_rf_phy.pdata.fdd ? AD9361_duplex_mode_t::FDD : AD9361_duplex_mode_t::TDD;

  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         use_extclk setting
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
const char* get_AD9361_use_extclk(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    bool& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "ad9361_rf_phy", vstr);

  char* ret;
  struct ad9361_config_proxy_ad9361_rf_phy ad9361_rf_phy;

  ret = (char*) parse(vstr.c_str(), ad9361_rf_phy);
  if(ret != 0) { return ret; }

  val = ad9361_rf_phy.pdata.use_extclk;

  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         Tx Channel Swap setting
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
const char* get_AD9361_Tx_Channel_Swap(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_bool_t& val)
{
  std::string vstr;
  OCPI::API::Property p(app, app_inst_name_proxy, "Tx_Channel_Swap");

  val = p.getBoolValue();

  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         Rx Channel Swap setting
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
const char* get_AD9361_Rx_Channel_Swap(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_bool_t& val)
{
  std::string vstr;
  OCPI::API::Property p(app, app_inst_name_proxy, "Rx_Channel_Swap");

  val = p.getBoolValue();

  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         one_rx_one_tx_mode_use_rx_num setting
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
const char* get_AD9361_one_rx_one_tx_mode_use_rx_num(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    AD9361_one_rx_one_tx_mode_use_rx_num_t& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "ad9361_rf_phy", vstr);

  char* ret;
  struct ad9361_config_proxy_ad9361_rf_phy ad9361_rf_phy;

  ret = (char*) parse(vstr.c_str(), ad9361_rf_phy);
  if(ret != 0) { return ret; }

  val = (ad9361_rf_phy.pdata.rx1tx1_mode_use_rx_num == 1) ? AD9361_one_rx_one_tx_mode_use_rx_num_t::R1 : AD9361_one_rx_one_tx_mode_use_rx_num_t::R2;

  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         rx_rf_port_input setting
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
const char* get_AD9361_rx_rf_port_input(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    AD9361_rx_rf_port_input_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "rx_rf_port_input");
  ocpi_ulong_t rx_rf_port_input = p.getULongValue();

  switch(rx_rf_port_input)
  {
    case 0:  val = AD9361_rx_rf_port_input_t::A_BALANCED; break;
    case 1:  val = AD9361_rx_rf_port_input_t::B_BALANCED; break;
    case 2:  val = AD9361_rx_rf_port_input_t::C_BALANCED; break;
    case 3:  val = AD9361_rx_rf_port_input_t::A_N;        break;
    case 4:  val = AD9361_rx_rf_port_input_t::A_P;        break;
    case 5:  val = AD9361_rx_rf_port_input_t::B_N;        break;
    case 6:  val = AD9361_rx_rf_port_input_t::B_P;        break;
    case 7:  val = AD9361_rx_rf_port_input_t::C_N;        break;
    case 8:  val = AD9361_rx_rf_port_input_t::C_P;        break;
    default:
      std::ostringstream err_oss;
      err_oss << "Invalid value read read ";
      err_oss << "from 'ad9361_config_proxy' worker's";
      err_oss << "'rx_rf_port_input' property 'rx_rf_port_input': ";
      err_oss << rx_rf_port_input;
      return err_oss.str().c_str();
  }

  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         RX RF port which corresponds to the timing diagram R1 channel
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
const char* get_AD9361_RX_RF_port_for_timing_diagram_R1(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    AD9361_RX_port_t& val)
{
  AD9361_rx_rf_port_input_t AD9361_rx_rf_port_input;
  
  const char* inst = app_inst_name_proxy;
  const char* err = get_AD9361_rx_rf_port_input(app, inst, AD9361_rx_rf_port_input);
  if(err != 0) { return err; }

  if((AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::A_BALANCED) or
     (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::A_N) or 
     (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::A_P))
  {
    val = AD9361_RX_port_t::RX1A;
  }
  else if((AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::B_BALANCED) or
          (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::B_N) or 
          (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::B_P))
  {
    val = AD9361_RX_port_t::RX1B;
  }
  else // ((AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::C_BALANCED) or
       //  (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::C_N) or
       //  (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::C_P))
  {
    val = AD9361_RX_port_t::RX1C;
  }
  
  return 0;
}

/*! @brief Get the in-situ value
 *         of the
 *         RX RF port which corresponds to the timing diagram R2 channel
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
const char* get_AD9361_RX_RF_port_for_timing_diagram_R2(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    AD9361_RX_port_t& val)
{
  AD9361_rx_rf_port_input_t AD9361_rx_rf_port_input;
  
  const char* inst = app_inst_name_proxy;
  const char* err = get_AD9361_rx_rf_port_input(app, inst, AD9361_rx_rf_port_input);
  if(err != 0) { return err; }

  if((AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::A_BALANCED) or
     (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::A_N) or 
     (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::A_P))
  {
    val = AD9361_RX_port_t::RX2A;
  }
  else if((AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::B_BALANCED) or
          (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::B_N) or 
          (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::B_P))
  {
    val = AD9361_RX_port_t::RX2B;
  }
  else // ((AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::C_BALANCED) or
       //  (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::C_N) or
       //  (AD9361_rx_rf_port_input == AD9361_rx_rf_port_input_t::C_P))
  {
    val = AD9361_RX_port_t::RX2C;
  }
  
  return 0;
}

/*! @brief Get the in-situ value with double floating point precision
 *         of the
 *         FB_CLK_Delay in nanoseconds
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
const char* get_AD9361_FB_CLK_Delay_ns(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "FB_CLK_Delay");

  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 6:
  // "The typical delay is approximately 0.3 ns/LSB."
  ocpi_ushort_t FB_CLK_Delay = p.getUShortValue();
  val = ((double)FB_CLK_Delay) * 0.3;

  return 0;
}

/*! @brief Get the in-situ value with double floating point precision
 *         of the
 *         TX_Data_Delay in nanoseconds
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
const char* get_AD9361_Tx_Data_Delay_ns(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "Tx_Data_Delay");

  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 6:
  // "The typical delay is approximately 0.3 ns/LSB."
  ocpi_ushort_t Tx_Data_Delay = p.getUShortValue();
  val = ((double)Tx_Data_Delay) * 0.3;

  return 0;
}



#endif // _READERS_AD9361_CFG_H
