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

#ifndef _READERS_AD9361_TX_DAC_H
#define _READERS_AD9361_TX_DAC_H

/*! @file
 *  @brief Provides functions for reading in-situ DAC-related values
 *         from an operating AD9361 IC using an OpenCPI application.
 *
 * \verbatim
 
    TX_SAMPL_FREQ..corresponds to the overall effective DAC sampling
                   rate divided by the TX FIR interpolation rate in
                   complex samples per second. (I *think* this is
                   assumed to be the TX_FB_CLK pin's clock rate, the AD9361 
                   probably synchronizes at some point between
                   the TX_FB_CLK domain and the clocks derived from
                   the DAC_FREQ clock domain.)
    CLKTF_FREQ.....corresponds to the overall effective DAC sampling
                   rate in complex samples per second.

                                     AD9361 IC
   +---------------------------------------------------------------------------+
   |                            "effective" DAC                                |
   |    TX               +----------------------------------------\            |
   |   data  +---+       |  +---+    +---+    +---+                \           |
   | ------->|FIR|------>|->|HB1|--->|HB2|--->|HB3|----+            \          |
   |         |   |       |  |   |    |   |    |   |    |  +-----\    \         |
   |         |   |       |  |   |    |   |    |   |    +->| DAC  \    \        |
   | ........|> <|------>|--|> <|----|> <|----|> <|-+     |       +--->+->     |
   |(TX_SAMPL+---+ CLKTF |  +---+ T1 +---+ T2 +---+ +---+-|>     /    /        |
   | FREQ)    x1   FREQ  |   x1   FREQ x1  FREQ x1  DAC | +-----/    /         |
   |          x2         |   x2        x2       x2  FREQ|           /          |
   |          x4         |                      x3      |          /           |
   |                     +----------------------------------------/            |
   |                                                    /\                     |
   |                                                    | DAC_FREQ             |
   |                                                    |                      |
   |                                                  +---+ DAC                |
   |                                                  |   | divider            |
   |                                                  +---+ /1, /2             |
   |                                                    /\                     |
   |                                                    |                      |
   |                                               -----+ ADC_FREQ             |
   |                                                                           |
   +---------------------------------------------------------------------------+
 
   CLKTF_FREQ formula:
   DAC_FREQ = BBPLL_freq / DAC_divider
   CLKTF_FREQ = 1/HB1/HB2/HB3*DAC_FREQ

   \endverbatim
 *
 ******************************************************************************/

#include <cstdint>    // uint8_t, uint32_t types
#include "OcpiApi.hh" // OCPI::API namespace
//#include "ad9361.h"   // BBPLL_MODULUS macro (this is a ADI No-OS header)
#include "ocpi_component_prop_type_helpers.h" // ocpi_float_t
#include "readers_ad9361_bb_rx_adc.h"             // get_AD9361_ADC_FREQ_Hz()
#include "readers_ad9361_bb_tx_filters_digital.h" // get_AD9361_THB...() functions

/*! @brief Get the in-situ value with exact precision of the
 *         DAC divider
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
const char* get_AD9361_DAC_divider(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    uint8_t& val)
{
  OCPI::API::Property p(app, app_inst_name_proxy, "DAC_Clk_div2");
  val = p.getBoolValue() ? 2 : 1;

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 DAC frequency in Hz
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
const char* get_AD9361_DAC_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_ADC_FREQ_Hz;
  double d_DAC_divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double  ADC_FREQ_Hz;
    uint8_t DAC_divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_ADC_FREQ_Hz(app, inst, ADC_FREQ_Hz);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_DAC_divider(app, inst, DAC_divider);
    if(ret != 0) { return ret; }

    d_ADC_FREQ_Hz = (double) ADC_FREQ_Hz;
    d_DAC_divider = (double) DAC_divider;
  }

  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 7:
  // "When clear, the DAC clock rate equals the ADC clock rate.
  // When set, the DAC clock equals 1/2 of the ADC rate."
  val = d_ADC_FREQ_Hz / d_DAC_divider;

  //log_debug("calculated DAC_FREQ = %.15f Hz", val);

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         DAC_FREQ step size above or below the current DAC_FREQ in Hz
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
const char* get_AD9361_DAC_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_ADC_FREQ_step_Hz;
  double d_DAC_divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double  ADC_FREQ_step_Hz;
    uint8_t DAC_divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_ADC_FREQ_step_Hz(app, inst, ADC_FREQ_step_Hz);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_DAC_divider(     app, inst, DAC_divider     );
    if(ret != 0) { return ret; }

    d_ADC_FREQ_step_Hz = (double) ADC_FREQ_step_Hz;
    d_DAC_divider      = (double) DAC_divider;
  }

  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 7:
  // "When clear, the DAC clock rate equals the ADC clock rate.
  // When set, the DAC clock equals 1/2 of the ADC rate."
  val = d_ADC_FREQ_step_Hz / d_DAC_divider;

  return 0;
}

const char* get_AD9361_T2_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_DAC_FREQ_Hz;
  double d_THB3_interpolation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double DAC_FREQ_Hz;
    uint8_t THB3_interpolation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_DAC_FREQ_Hz(              app, inst, DAC_FREQ_Hz              );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_THB3_interpolation_factor(app, inst, THB3_interpolation_factor);
    if(ret != 0) { return ret; }

    d_DAC_FREQ_Hz               = (double) DAC_FREQ_Hz;
    d_THB3_interpolation_factor = (double) THB3_interpolation_factor;
  }

  val = d_DAC_FREQ_Hz / d_THB3_interpolation_factor;
  //log_debug("calculated T2_FREQ = %.15f Hz", val);

  return 0;
}

const char* get_AD9361_T2_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_DAC_FREQ_step_Hz;
  double d_THB3_interpolation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double DAC_FREQ_step_Hz;
    uint8_t THB3_interpolation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_DAC_FREQ_step_Hz(         app, inst, DAC_FREQ_step_Hz         );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_THB3_interpolation_factor(app, inst, THB3_interpolation_factor);
    if(ret != 0) { return ret; }

    d_DAC_FREQ_step_Hz          = (double) DAC_FREQ_step_Hz;
    d_THB3_interpolation_factor = (double) THB3_interpolation_factor;
  }

  val = d_DAC_FREQ_step_Hz / d_THB3_interpolation_factor;

  return 0;
}

const char* get_AD9361_T1_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_T2_FREQ_Hz;
  double d_THB2_interpolation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double T2_FREQ_Hz;
    uint8_t THB2_interpolation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_T2_FREQ_Hz(               app, inst, T2_FREQ_Hz               );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_THB2_interpolation_factor(app, inst, THB2_interpolation_factor);
    if(ret != 0) { return ret; }

    d_T2_FREQ_Hz                = (double) T2_FREQ_Hz;
    d_THB2_interpolation_factor = (double) THB2_interpolation_factor;
  }

  val = d_T2_FREQ_Hz / d_THB2_interpolation_factor;
  //log_debug("calculated T1_FREQ = %.15f Hz", val);

  return 0;
}

const char* get_AD9361_T1_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_T2_FREQ_step_Hz;
  double d_THB2_interpolation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double T2_FREQ_step_Hz;
    uint8_t THB2_interpolation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_T2_FREQ_step_Hz(          app, inst, T2_FREQ_step_Hz          );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_THB2_interpolation_factor(app, inst, THB2_interpolation_factor);
    if(ret != 0) { return ret; }

    d_T2_FREQ_step_Hz           = (double) T2_FREQ_step_Hz;
    d_THB2_interpolation_factor = (double) THB2_interpolation_factor;
  }

  val = d_T2_FREQ_step_Hz / d_THB2_interpolation_factor;

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 CLKTF frequency in Hz
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
const char* get_AD9361_CLKTF_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_T1_FREQ_Hz;
  double d_THB1_interpolation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double T1_FREQ_Hz;
    uint8_t THB1_interpolation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_T1_FREQ_Hz(               app, inst, T1_FREQ_Hz               );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_THB1_interpolation_factor(app, inst, THB1_interpolation_factor);
    if(ret != 0) { return ret; }

    d_T1_FREQ_Hz                = (double) T1_FREQ_Hz;
    d_THB1_interpolation_factor = (double) THB1_interpolation_factor;
  }

  val = d_T1_FREQ_Hz / d_THB1_interpolation_factor;
  //log_debug("calculated CLKTF_FREQ = %.15f Hz", val);

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 CLKTF frequency step size above or below the current
 *         CLKTF frequency in Hz
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
const char* get_AD9361_CLKTF_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_T1_FREQ_step_Hz;
  double d_THB1_interpolation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double T1_FREQ_step_Hz;
    uint8_t THB1_interpolation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_T1_FREQ_step_Hz(          app, inst, T1_FREQ_step_Hz          );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_THB1_interpolation_factor(app, inst, THB1_interpolation_factor);
    if(ret != 0) { return ret; }

    d_T1_FREQ_step_Hz           = (double) T1_FREQ_step_Hz;
    d_THB1_interpolation_factor = (double) THB1_interpolation_factor;
  }

  val = d_T1_FREQ_step_Hz / d_THB1_interpolation_factor;

  return 0;
}

#endif // _READERS_AD9361_TX_DAC_H
