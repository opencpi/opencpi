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

#ifndef _READERS_AD9361_TX_RFPLL_H
#define _READERS_AD9361_TX_RFPLL_H

/*! @file
 *  @brief Provides functions for reading in-situ Receiver (TX) Radio
 *         Frequency (RF) Phase-Locked Loop (RFPLL) values from an operating
 *         AD9361 IC using an OpenCPI application. See
 *         AD9361_Reference_Manual_UG-570.pdf
 *         Figure 4. PLL Synthesizer Block Diagram
 ******************************************************************************/

#include "OcpiApi.hh" // OA namespace
#include "ad9361.h"   // RFPLL_MODULUS macro (this is a ADI No-OS header)

namespace OA = OCPI::API;

void get_AD9361_Tx_RFPLL_input_F_REF(
    OA::Application& app, const char* app_inst_name_proxy,
    OA::ULong& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "ad9361_rf_phy", vstr);

  struct ad9361_config_proxy_ad9361_rf_phy ad9361_rf_phy;

  parse(vstr.c_str(), ad9361_rf_phy);

  val = ad9361_rf_phy.clk_refin.rate;
}

void get_AD9361_Tx_RFPLL_ref_divider(
    OA::Application& app, const char* app_inst_name_proxy,
    OA::Float& val)
{
  OA::Property p(app, app_inst_name_proxy, "Tx_Ref_divider");
  val = p.getFloatValue();
}

void get_AD9361_Tx_RFPLL_N_Integer(
    OA::Application& app, const char* app_inst_name_proxy,
    OA::UShort& val)
{
  OA::Property p(app, app_inst_name_proxy, "tx_vco_n_integer");
  val = p.getUShortValue();
}

void get_AD9361_Tx_RFPLL_N_Fractional(
    OA::Application& app, const char* app_inst_name_proxy,
    OA::ULong& val)
{
  OA::Property p(app, app_inst_name_proxy, "tx_vco_n_fractional");
  val = p.getULongValue();
}

void get_AD9361_Tx_RFPLL_external_div_2_enable(
    OA::Application& app, const char* app_inst_name_proxy,
    bool& val)
{
  std::string enum_str;
  app.getProperty(app_inst_name_proxy, "tx_vco_divider", enum_str);
  if((enum_str == "2")  ||
     (enum_str == "4")  ||
     (enum_str == "8")  ||
     (enum_str == "16") ||
     (enum_str == "32") ||
     (enum_str == "64") ||
     (enum_str == "128"))
  {
    val = false;
  }
  else if(enum_str == "external_2")
  {
    val = true;
  }
  else
  {
    std::string err;
    err = "Invalid value read for ad9361_config_proxy.rcc ";
    err += "tx_vco_divider property: " + enum_str;
    throw err;
  }
}

void get_AD9361_Tx_RFPLL_VCO_Divider(
    OA::Application& app, const char* app_inst_name_proxy,
    uint8_t& val)
{
  std::string enum_str;
  app.getProperty(app_inst_name_proxy, "tx_vco_divider", enum_str);
  if((enum_str == "2")  ||
     (enum_str == "4")  ||
     (enum_str == "8")  ||
     (enum_str == "16") ||
     (enum_str == "32") ||
     (enum_str == "64") ||
     (enum_str == "128"))
  {
    val = (uint8_t) (strtol(enum_str.c_str(), NULL, 0) & 0xff);
  }
  else if(enum_str == "external_2")
  {
    val = 0;
  }
  else
  {
    std::string err;
    err = "Invalid value read for ad9361_config_proxy.rcc ";
    err += "tx_vco_divider property: " + enum_str;
    throw err;
  }
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 TX RF LO frequency in Hz
 *         from an operating AD9361 IC controlled by the specified OpenCPI
 *         application instance of the ad9361_config_proxy.rcc worker.
 *
 *  @param[in]  app                 OpenCPI application reference
 *  @param[in]  app_inst_name_proxy OpenCPI application instance name of the
 *                                  OpenCPI ad9361_config_proxy.rcc worker
 *  @param[out] val                 Retrieved value.
 ******************************************************************************/
void get_AD9361_Tx_RFPLL_LO_freq_Hz(
    OA::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_Tx_RFPLL_input_F_REF;
  double d_Tx_RFPLL_ref_divider;
  double d_Tx_RFPLL_N_Integer;
  double d_Tx_RFPLL_N_Fractional;
  double d_Tx_RFPLL_VCO_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    bool       Tx_RFPLL_external_div_2_enable;
    OA::ULong  Tx_RFPLL_input_F_REF;
    OA::Float  Tx_RFPLL_ref_divider;
    OA::UShort Tx_RFPLL_N_Integer;
    OA::ULong  Tx_RFPLL_N_Fractional;
    uint8_t    Tx_RFPLL_VCO_Divider;

    const char* inst = app_inst_name_proxy;

    get_AD9361_Tx_RFPLL_external_div_2_enable(app, inst, Tx_RFPLL_external_div_2_enable);
    get_AD9361_Tx_RFPLL_input_F_REF( app, inst, Tx_RFPLL_input_F_REF );

    if(Tx_RFPLL_external_div_2_enable)
    {
      val = ((double)Tx_RFPLL_input_F_REF) / 2.;
      return;
    }

    get_AD9361_Tx_RFPLL_ref_divider( app, inst, Tx_RFPLL_ref_divider );
    get_AD9361_Tx_RFPLL_N_Integer(   app, inst, Tx_RFPLL_N_Integer   );
    get_AD9361_Tx_RFPLL_N_Fractional(app, inst, Tx_RFPLL_N_Fractional);
    get_AD9361_Tx_RFPLL_VCO_Divider( app, inst, Tx_RFPLL_VCO_Divider );

    d_Tx_RFPLL_input_F_REF  = (double) Tx_RFPLL_input_F_REF;
    d_Tx_RFPLL_ref_divider  = (double) Tx_RFPLL_ref_divider;
    d_Tx_RFPLL_N_Integer    = (double) Tx_RFPLL_N_Integer;
    d_Tx_RFPLL_N_Fractional = (double) Tx_RFPLL_N_Fractional;
    d_Tx_RFPLL_VCO_Divider  = (double) Tx_RFPLL_VCO_Divider;
  }

  //AD9361_Reference_Manual_UG-570.pdf Figure 4. PLL Synthesizer Block Diagram
  //(calculating the "FREF = 10MHz TO 80MHz" signal)
  double x = d_Tx_RFPLL_input_F_REF;
  // why multiply (*) not divide? (I suspect ADI's term "divider" is a misnomer)
  x *= d_Tx_RFPLL_ref_divider;

  //AD9361_Reference_Manual_UG-570.pdf Figure 4. PLL Synthesizer Block Diagram
  //(calculating the "TO VCO DIVIDER BLOCK" signal)
  // this calculation is similar to what's done in Analog Device's No-OS's
  // ad9361.c's ad9361_calc_rfpll_int_freq() function, except WE use floating
  // point and don't round
  x *= (d_Tx_RFPLL_N_Integer + (d_Tx_RFPLL_N_Fractional/RFPLL_MODULUS));
  //log_debug("d_Tx_RFPLL_N_Integer=%.15f", d_Tx_RFPLL_N_Integer);
  //log_debug("d_Tx_RFPLL_N_Fractional=%.15f", d_Tx_RFPLL_N_Fractional);

  //AD9361_Reference_Manual_UG-570.pdf Figure 4. VCO Divider
  //(calculating the "LO" signal which is the  output of the MUX)
  x /= d_Tx_RFPLL_VCO_Divider;
  //log_debug("d_Tx_RFPLL_VCO_Divider=%.15f", d_Tx_RFPLL_VCO_Divider);

  val = x;
}

#endif // _READERS_AD9361_TX_RFPLL_H
