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

#ifndef _NO_OS_PLATFORM_OPENCPI_SLAVE_AD9361_CONFIG_H
#define _NO_OS_PLATFORM_OPENCPI_SLAVE_AD9361_CONFIG_H

#include <cstdint> // int32_t, etc.
#include <cstdlib> // strtol()
#include <OcpiApi.hh>

extern "C" {
#include "ad9361_api.h" // AD9361_InitParam
}

namespace OA = OCPI::API;

/* @brief The AD9361 register set determines which channel mode is used (1R1T,
 * 1R2T, 2R1T, or 1R2T). This mode eventually determines which timing diagram
 * the AD9361 is expecting for the TX data path pins. This function
 * tells the FPGA bitstream which channel mode should be assumed when
 * generating the TX data path signals.
 ***************************************************************************/
template<typename T>
void slave_ad9361_config_set_FPGA_channel_config(T& slave) {
  uint8_t general_tx_enable_filter_ctrl =
      slave.get_general_tx_enable_filter_ctrl();
  bool two_t = TX_CHANNEL_ENABLE(TX_1 | TX_2) ==
      (general_tx_enable_filter_ctrl & 0xc0);
  uint8_t general_rx_enable_filter_ctrl =
      slave.get_general_rx_enable_filter_ctrl();
  bool two_r = RX_CHANNEL_ENABLE(RX_1 | RX_2) ==
      (general_rx_enable_filter_ctrl & 0xc0);
  slave.set_config_is_two_r(two_r);
  slave.set_config_is_two_t(two_t);
}

//class enum ad9361_config_rx_frame_usage_t           {enable,toggle};
//class enum ad9361_config_data_bus_index_direction_t {normal,reverse};
//class enum ad9361_config_data_rate_config_t         {SDR,DDR};

/*! @brief ad9361_config worker's props whose purpose is to read back
 *         the static configuration of the FPGA bitstream - reference the OWD.
 ******************************************************************************/
struct ad9361_config_config_t {
  OA::Bool                                           qadc1_is_present;
  OA::Bool                                           qdac1_is_present;
  Ad9361_configWorkerTypes::Rx_frame_usage           rx_frame_usage;
  Ad9361_configWorkerTypes::Data_bus_index_direction data_bus_index_direction;
  OA::Bool                                           data_clk_is_inverted;
  OA::Bool                                           rx_frame_is_inverted;
  OA::Bool                                           LVDS;
  OA::Bool                                           single_port;
  OA::Bool                                           swap_ports;
  OA::Bool                                           half_duplex;
  Ad9361_configWorkerTypes::Data_rate_config         data_rate_config;
};

/*! @brief ad9361_data_sub worker's props whose purpose is to read back
 *         the static configuration of the FPGA bitstream - reference the OWD.
 ******************************************************************************/
struct ad9361_data_sub_config_t {
  OA::UShort DATA_CLK_Delay;
  OA::UShort RX_Data_Delay;
  OA::UShort FB_CLK_Delay;
  OA::UShort TX_Data_Delay;
};

/*! @brief Purpose is to read back the static configuration of the FPGA
 *         bitstream.
 ******************************************************************************/
struct ad9361_bitstream_config_t : ad9361_config_config_t,
                                   ad9361_data_sub_config_t {
};

/*! @param[in]  slave Reference to ad9361_config slave interface object.
 *  @param[in]  app   Reference to OpenCPI applicaiton object.
 *  @param[in]  inst  OpenCPI application instance name of ad9361_data_sub.
 *  @param[out] cfg   Reference to AD9361 bitstream configuration object.
 ******************************************************************************/
template<typename T>
void ad9361_bitstream_get_config(
    OA::Application& app, T& slave, const char * inst,
    ad9361_bitstream_config_t& cfg) {

  // don't care about qadc0_is_present
  cfg.qadc1_is_present         = slave.get_qadc1_is_present();
  // don't care about qadc1_is_present
  cfg.qdac1_is_present         = slave.get_qdac1_is_present();
  cfg.rx_frame_usage           = slave.get_rx_frame_usage();
  cfg.data_bus_index_direction = slave.get_data_bus_index_direction();
  cfg.data_clk_is_inverted     = slave.get_data_clk_is_inverted();
  cfg.rx_frame_is_inverted     = slave.get_rx_frame_is_inverted();
  cfg.LVDS                     = slave.get_LVDS();
  cfg.single_port              = slave.get_single_port();
  cfg.swap_ports               = slave.get_swap_ports();
  cfg.half_duplex              = slave.get_half_duplex();
  cfg.data_rate_config         = slave.get_data_rate_config();

  std::string DATA_CLK_Delay_str;
  app.getProperty(inst, "DATA_CLK_Delay", DATA_CLK_Delay_str);
  cfg.DATA_CLK_Delay = strtol(DATA_CLK_Delay_str.c_str(), NULL, 0);

  std::string RX_Data_Delay_str;
  app.getProperty(inst, "RX_Data_Delay", RX_Data_Delay_str);
  cfg.RX_Data_Delay = strtol(RX_Data_Delay_str.c_str(), NULL, 0);

  std::string FB_CLK_Delay_str;
  app.getProperty(inst, "FB_CLK_Delay", FB_CLK_Delay_str);
  cfg.FB_CLK_Delay = strtol(FB_CLK_Delay_str.c_str(), NULL, 0);

  std::string TX_Data_Delay_str;
  app.getProperty(inst, "TX_Data_Delay", TX_Data_Delay_str);
  cfg.TX_Data_Delay = strtol(TX_Data_Delay_str.c_str(), NULL, 0);
}

void slave_ad9361_config_apply_config_to_AD9361_InitParam(
    const ad9361_bitstream_config_t& config,
    AD9361_InitParam& init) {
  bool is_2R1T_or_1R2T_or_2R2T = config.qadc1_is_present or config.qdac1_is_present;

  bool rx_frame_usage_is_toggle;
  switch(config.rx_frame_usage) {
    case Ad9361_configWorkerTypes::RX_FRAME_USAGE_TOGGLE:
      rx_frame_usage_is_toggle = true;
      break;
    default:
      rx_frame_usage_is_toggle = false;
      break;
  }

  bool data_bus_idx_dir_inv;
  switch(config.data_bus_index_direction) {
    case Ad9361_configWorkerTypes::DATA_BUS_INDEX_DIRECTION_NORMAL:
      data_bus_idx_dir_inv = true;
    default:
      data_bus_idx_dir_inv = false;
  }

  bool mode_is_SDR;
  switch(config.data_rate_config) {
    case Ad9361_configWorkerTypes::DATA_RATE_CONFIG_SDR:
      mode_is_SDR = true;
    default:
      mode_is_SDR = false;
  }

  init.rx_frame_pulse_mode_enable = rx_frame_usage_is_toggle ? 1 : 0;
  init.invert_data_bus_enable = data_bus_idx_dir_inv ? 1:0;
  init.invert_data_clk_enable = config.data_clk_is_inverted ? 1 : 0;
  init.invert_rx_frame_enable = config.rx_frame_is_inverted ? 1 : 0;
  if(config.LVDS)
  {
    init.lvds_rx_onchip_termination_enable = 1;
    init.lvds_mode_enable                  = 1;

    // AD9361_Reference_Manual_UG-570.pdf (Rev. A):
    // "The following bits are not supported in LVDS mode:
    // * Swap Ports-In LVDS mode, P0 is Tx and P1 is Rx. This configuration cannot be changed.
    // * Single Port Mode-Both ports are enabled in LVDS mode.
    // * FDD Full Port-Not supported in LVDS.
    // * FDD Alt Word Order-Not supported in LVDS.
    // * FDD Swap Bits-Not supported in LVDS."
    init.swap_ports_enable            = 0;
    init.single_port_mode_enable      = 0;
    init.full_port_enable             = 0;
    init.fdd_alt_word_order_enable    = 0; ///@TODO/FIXME read this value from FPGA?
    init.half_duplex_mode_enable      = 0;
    init.single_data_rate_enable      = 0;
    init.full_duplex_swap_bits_enable = 0; ///@TODO / FIXME read this value from FPGA?
  }
  else { // mode is CMOS
    init.lvds_rx_onchip_termination_enable = 0;
    init.lvds_mode_enable                  = 0;

    init.swap_ports_enable         = config.swap_ports ? 1 : 0;
    init.single_port_mode_enable   = config.single_port ? 1 : 0;
    init.fdd_alt_word_order_enable = 0; ///@TODO/FIXME read this value from FPGA?
    init.full_port_enable          = (!config.half_duplex) and (!config.single_port);
    init.half_duplex_mode_enable   = config.half_duplex ? 1 : 0;
    init.single_data_rate_enable   = mode_is_SDR ? 1 : 0;
    init.full_duplex_swap_bits_enable = 0; ///@TODO / FIXME read this value from FPGA?
  }
  init.two_rx_two_tx_mode_enable = is_2R1T_or_1R2T_or_2R2T ? 1 : 0;

  init.rx_data_clock_delay = config.DATA_CLK_Delay;
  init.rx_data_delay       = config.RX_Data_Delay;
  init.tx_fb_clock_delay   = config.FB_CLK_Delay;
  init.tx_data_delay       = config.TX_Data_Delay;
}

#endif // _NO_OS_PLATFORM_OPENCPI_SLAVE_AD9361_CONFIG_H
