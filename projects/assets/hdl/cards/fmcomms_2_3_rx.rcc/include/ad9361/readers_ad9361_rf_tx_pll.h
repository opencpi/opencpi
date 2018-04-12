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

/*! @file readers_ad9361_tx_rfpll.h
 *  @brief Provides functions for reading in-situ Transmitter (TX) Radio
 *         Frequency (RF) Phase-Locked Loop (RFPLL) values from an operating
 *         AD9361 IC using an OpenCPI application. See
 *         AD9361_Reference_Manual_UG-570.pdf
 *         Figure 4. PLL Synthesizer Block Diagram
 ******************************************************************************/

#include "OcpiApi.hh" // OCPI::API namespace
#include "ad9361.h"   // RFPLL_MODULUS macro (this is a ADI No-OS header)
#include "ocpi_component_prop_type_helpers.h" // ocpi_... types

const char* get_AD9361_Tx_RFPLL_input_F_REF(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_ulong_t& val)
{
  std::string vstr;
  app.getProperty(app_inst_name_proxy, "ad9361_rf_phy", vstr);

  char* ret;
  struct ad9361_rf_phy_ad9361_config_proxy ad9361_rf_phy;

  ret = (char*) parse(vstr.c_str(), ad9361_rf_phy);
  if(ret != 0) { return ret; }

  val = ad9361_rf_phy.clk_refin.rate;

  return 0;
}

const char* get_AD9361_Tx_RFPLL_ref_divider(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_float_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "Tx_Ref_divider");
  val = p.getFloatValue();

  return 0;
}

const char* get_AD9361_Tx_RFPLL_N_Integer(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_ushort_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "tx_vco_n_integer");
  val = p.getUShortValue();

  return 0;
}

const char* get_AD9361_Tx_RFPLL_N_Fractional(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    ocpi_ulong_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "tx_vco_n_fractional");
  val = p.getULongValue();

  return 0;
}

const char* get_AD9361_Tx_RFPLL_external_div_2_enable(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
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
    return err.c_str();
  }

  return 0;
}

const char* get_AD9361_Tx_RFPLL_VCO_Divider(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
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
    return err.c_str();
  }

  return 0;
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
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* get_AD9361_Tx_RFPLL_LO_freq_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_Tx_RFPLL_input_F_REF;
  double d_Tx_RFPLL_ref_divider;
  double d_Tx_RFPLL_N_Integer;
  double d_Tx_RFPLL_N_Fractional;
  double d_Tx_RFPLL_VCO_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    bool          Tx_RFPLL_external_div_2_enable;
    ocpi_ulong_t  Tx_RFPLL_input_F_REF;
    ocpi_float_t  Tx_RFPLL_ref_divider;
    ocpi_ushort_t Tx_RFPLL_N_Integer;
    ocpi_ulong_t  Tx_RFPLL_N_Fractional;
    uint8_t       Tx_RFPLL_VCO_Divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_Tx_RFPLL_external_div_2_enable(app, inst, Tx_RFPLL_external_div_2_enable);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_RFPLL_input_F_REF( app, inst, Tx_RFPLL_input_F_REF );
    if(ret != 0) { return ret; }

    if(Tx_RFPLL_external_div_2_enable)
    {
      val = ((double)Tx_RFPLL_input_F_REF) / 2.;
      return 0;
    }

    ret = (char*) get_AD9361_Tx_RFPLL_ref_divider( app, inst, Tx_RFPLL_ref_divider );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_RFPLL_N_Integer(   app, inst, Tx_RFPLL_N_Integer   );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_RFPLL_N_Fractional(app, inst, Tx_RFPLL_N_Fractional);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_RFPLL_VCO_Divider( app, inst, Tx_RFPLL_VCO_Divider );
    if(ret != 0) { return ret; }

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

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 TX RF LO frequency step size above and/or below the current
 *         LO frequency in Hz
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
const char* get_AD9361_Tx_RFPLL_LO_freq_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_Tx_RFPLL_input_F_REF;
  double d_Tx_RFPLL_ref_divider;
  double d_Tx_RFPLL_VCO_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    bool         Tx_RFPLL_external_div_2_enable;
    uint32_t     Tx_RFPLL_input_F_REF;
    ocpi_float_t Tx_RFPLL_ref_divider;
    uint8_t      Tx_RFPLL_VCO_Divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_Tx_RFPLL_external_div_2_enable(app, inst, Tx_RFPLL_external_div_2_enable);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_RFPLL_input_F_REF( app, inst, Tx_RFPLL_input_F_REF );
    if(ret != 0) { return ret; }

    if(Tx_RFPLL_external_div_2_enable)
    {
      // in this case, the Tx PLL is fixed, so we set the step size to infinity
      val = std::numeric_limits<double>::infinity();
      return 0;
    }

    ret = (char*) get_AD9361_Tx_RFPLL_ref_divider(app, inst, Tx_RFPLL_ref_divider);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_Tx_RFPLL_VCO_Divider(app, inst, Tx_RFPLL_VCO_Divider);
    if(ret != 0) { return ret; }

    d_Tx_RFPLL_input_F_REF  = (double) Tx_RFPLL_input_F_REF;
    d_Tx_RFPLL_ref_divider  = (double) Tx_RFPLL_ref_divider;
    d_Tx_RFPLL_VCO_Divider  = (double) Tx_RFPLL_VCO_Divider;
  }

  // theoretical step size based upon register precision (*not necessarily*
  // what No-OS/ad9361_config_proxy.rcc implements)
  double x = d_Tx_RFPLL_input_F_REF;
  x *= d_Tx_RFPLL_ref_divider; // why multiply (*) not divide? (I suspect ADI's term "divider" is a misnomer)
  x /= RFPLL_MODULUS;
  x /= d_Tx_RFPLL_VCO_Divider;
  
  val = x;

  return 0;
}

#endif // _READERS_AD9361_TX_RFPLL_H
