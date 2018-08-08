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

#ifndef _READERS_AD9361_BB_RX_FILTERS_DIGITAL_H
#define _READERS_AD9361_BB_RX_FILTERS_DIGITAL_H

/*! @file
 *  @brief Provides functions for reading in-situ RX digital filter/decimator
 *         values from an operating AD9361 IC using an OpenCPI application.
 *
 * \verbatim
 
    RX_SAMPL_FREQ..corresponds to the overall effective ADC sampling
                   rate (CLKRF_FREQ) divided by the RX FIR decimation factor.
                   The unit is in complex samples per second.
    CLKRF_FREQ.....corresponds to the overall effective ADC sampling
                   rate in complex samples per second. This rate is
                   equivalent to the rate of the clock signal which drives the
                   AD9361 DATA_CLK_P pin.

    x1,x2,x3,x4 below refers to the multiplication of the clock rate

                                     AD9361 IC
   +---------------------------------------------------------------------------+
   |                            "effective" ADC                                |
   |        /-----------------------------------------+               RX       |
   |       /                 +---+    +---+    +---+  |       +---+  data      |
   |      /            +-----|HB3|<---|HB2|<---|HB1|<-|<------|FIR|<-------    |
   |     /    /-----+  |     |   |    |   |    |   |  |       |   |            |
   |    /    /  ADC |<-+     |   |    |   |    |   |  |       |   |            |
   | <-+<---+       |     +--|> <|----|> <|----|> <|--|<------|> <|........    |
   |    \    \      |-+---+  +---+ R2 +---+ R1 +---+  | CLKRF +---+(RX_SAMPL   |
   |     \    \-----+ |ADC    x1   FREQ x1  FREQ x1   | FREQ   x1   FREQ)      |
   |      \           |FREQ   x2        x2       x2   |        x2              |
   |       \          |       x3                      |        x4              |
   |        \-----------------------------------------+                        |
   |                  /\                                                       |
   |                  | ADC_FREQ                                               |
   |                                                                           |
   +---------------------------------------------------------------------------+
 
   \endverbatim
 *
 ******************************************************************************/

#include <string>     // std::string
#include <cstdint>    // uint8_t type
#include "OcpiApi.hh" // OCPI::API namespace

/*! @brief Get the in-situ value with exact precision of the
 *         Receiver Half-Band filter 3's decimation factor
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
const char* get_AD9361_RHB3_decimation_factor(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    uint8_t& val)
{
  std::string enum_str;
  app.getProperty(app_inst_name_proxy, "RHB3_Enable_and_Decimation", enum_str);

  if(enum_str == "Decimate_by_1_no_filtering")
  {
    val = 1;
  }
  else if(enum_str == "Decimate_by_2_half_band_filter")
  {
    val = 2;
  }
  else if(enum_str == "Decimate_by_3_and_filter")
  {
    val = 3;
  }
  else
  {
    std::string err;
    err = "Invalid value read for ad9361_config_proxy.rcc ";
    err += "RHB3_Enable_and_Interp property: " + enum_str;
    return err.c_str();
  }

  return 0;
}

/*! @brief Get the in-situ value with exact precision of the
 *         Receive Half-Band filter 2's decimation factor
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
const char* get_AD9361_RHB2_decimation_factor(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    uint8_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "RHB2_Enable");
  val = p.getBoolValue() ? 2 : 1;

  return 0;
}

/*! @brief Get the in-situ value with exact precision of the
 *         Receive Half-Band filter 1's decimation factor
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
const char* get_AD9361_RHB1_decimation_factor(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    uint8_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "RHB1_Enable");
  val = p.getBoolValue() ? 2 : 1;

  return 0;
}

#endif // _READERS_AD9361_BB_RX_FILTERS_DIGITAL_H
