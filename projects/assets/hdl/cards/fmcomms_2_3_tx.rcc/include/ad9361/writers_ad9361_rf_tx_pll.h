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

#ifndef _WRITERS_AD9361_RF_TX_PLL_H
#define _WRITERS_AD9361_RF_TX_PLL_H

/*! @file
 *  @brief Provides functions for writing in-situ Transceiver (TX) Radio
 *         Frequency (RF) Phase-Locked Loop (RFPLL) values to an operating
 *         AD9361 IC using an OpenCPI application.
 ******************************************************************************/

/*! @brief Set the nominal in-situ value with No-OS precision
 *         of the
 *         AD9361 TX RF LO frequency in Hz
 *         to an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[in]  val                 Value to assign. Note that the precision
 *                                  is the same as the corresponding OpenCPI
 *                                  property which is the same as the
 *                                  underlying No-OS API call.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* set_AD9361_Tx_RFPLL_LO_freq_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    const ocpi_ulonglong_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "tx_lo_freq");
  p.setULongLongValue(val);

  return 0;
}

#endif // _WRITERS_AD9361_RF_TX_PLL_H
