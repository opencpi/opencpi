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

#ifndef _READERS_AD9361_RX_ADC_H
#define _READERS_AD9361_RX_ADC_H

/*! @file
 *  @brief Provides functions for reading in-situ ADC-related values
 *         from an operating AD9361 IC using an OpenCPI application.
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
   |                              "effective" ADC                              |
   |          /-----------------------------------------+               RX     |
   |         /                 +---+    +---+    +---+  |       +---+  data    |
   |        /            +---->|HB3|--->|HB2|--->|HB1|->|------>|FIR|------->  |
   |       /    /-----+  |     |   |    |   |    |   |  |       |   |          |
   |      /    /  ADC |--+     |   |    |   |    |   |  |       |   |          |
   |  -->+--->+       |     +--|> <|----|> <|----|> <|--|-------|> <|........  |
   |      \    \      |-+---+  +---+ R2 +---+ R1 +---+  | CLKRF +---+(RX_SAMPL |
   |       \    \-----+ |ADC    x1   FREQ x1  FREQ x1   | FREQ   x1   FREQ)    |
   |        \           |FREQ   x2        x2       x2   |        x2            |
   |         \          |       x3                      |        x4            |
   |          \-----------------------------------------+                      |
   |                    /\                                                     |
   |             +----+ | ADC_FREQ                                             |
   | BBPLL_FREQ  |    | |                                                      |
   | ----------->|    |-+                                                      |
   |             |    |                                                        |
   |             +----+                                                        |
   |             /1,  /2                                                       |
   |             /4,  /8                                                       |
   |             /16, /32                                                      |
   |             /64, /128                                                     |
   |             (/[2^BBPLL_Divider])                                          |
   |                                                                           |
   +---------------------------------------------------------------------------+
 
   \endverbatim
 *
 ******************************************************************************/

#include <cmath>                   // pow()
#include "OcpiApi.hh"              // OCPI::API::Application
#include "readers_ad9361_bb_pll.h" // get_AD9361_BBPLL_...() functions
#include "readers_ad9361_bb_rx_filters_digital.h" // get_AD9361_RHB...() functions

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 ADC frequency in Hz
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
const char* get_AD9361_ADC_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_BBPLL_FREQ;
  double d_BBPLL_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double  BBPLL_FREQ;
    uint8_t BBPLL_Divider;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_BBPLL_FREQ_Hz(app, inst, BBPLL_FREQ   );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_BBPLL_Divider(app, inst, BBPLL_Divider);
    if(ret != 0) { return ret; }

    d_BBPLL_FREQ    = (double) BBPLL_FREQ;
    d_BBPLL_Divider = (double) BBPLL_Divider;
  }

  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 7:
  // "The ADC clock rate equals the BBPLL divided by the factor in this
  // register, shown in Equation 2.
  // ADC Clock Rate = BBPLL Clock Rate / (2^(BBPLL Divider[2:0] (decimal)))"
  val = d_BBPLL_FREQ / pow(2,d_BBPLL_Divider);

  //log_debug("calculated ADC_FREQ = %.15f Hz", val);

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         ADC_FREQ step size above or below the current ADC_FREQ in Hz
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
const char* get_AD9361_ADC_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_BBPLL_FREQ_step_Hz;
  double d_BBPLL_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double  BBPLL_FREQ_step_Hz;
    uint8_t BBPLL_Divider;

    char* ret;

    const char* inst = app_inst_name_proxy;
    ret = (char*) get_AD9361_BBPLL_FREQ_step_Hz(app, inst, BBPLL_FREQ_step_Hz);
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_BBPLL_Divider(     app, inst, BBPLL_Divider     );
    if(ret != 0) { return ret; }

    d_BBPLL_FREQ_step_Hz = (double) BBPLL_FREQ_step_Hz;
    d_BBPLL_Divider      = (double) BBPLL_Divider;
  }

  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 7:
  // "The ADC clock rate equals the BBPLL divided by the factor in this
  // register, shown in Equation 2.
  // ADC Clock Rate = BBPLL Clock Rate / (2^(BBPLL Divider[2:0] (decimal)))"
  val = d_BBPLL_FREQ_step_Hz / pow(2,d_BBPLL_Divider);

  return 0;
}

const char* get_AD9361_R2_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_ADC_FREQ_Hz;
  double d_RHB3_decimation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double ADC_FREQ_Hz;
    uint8_t RHB3_decimation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_ADC_FREQ_Hz(           app, inst, ADC_FREQ_Hz           );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_RHB3_decimation_factor(app, inst, RHB3_decimation_factor);
    if(ret != 0) { return ret; }

    d_ADC_FREQ_Hz            = (double) ADC_FREQ_Hz;
    d_RHB3_decimation_factor = (double) RHB3_decimation_factor;
  }

  val = d_ADC_FREQ_Hz / d_RHB3_decimation_factor;

  return 0;
}

const char* get_AD9361_R2_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_ADC_FREQ_step_Hz;
  double d_RHB3_decimation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double ADC_FREQ_step_Hz;
    uint8_t RHB3_decimation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_ADC_FREQ_step_Hz(      app, inst, ADC_FREQ_step_Hz      );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_RHB3_decimation_factor(app, inst, RHB3_decimation_factor);
    if(ret != 0) { return ret; }

    d_ADC_FREQ_step_Hz       = (double) ADC_FREQ_step_Hz;
    d_RHB3_decimation_factor = (double) RHB3_decimation_factor;
  }

  val = d_ADC_FREQ_step_Hz / d_RHB3_decimation_factor;

  return 0;
}

const char* get_AD9361_R1_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_R2_FREQ_Hz;
  double d_RHB2_decimation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double R2_FREQ_Hz;
    uint8_t RHB2_decimation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_R2_FREQ_Hz(            app, inst, R2_FREQ_Hz            );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_RHB2_decimation_factor(app, inst, RHB2_decimation_factor);
    if(ret != 0) { return ret; }

    d_R2_FREQ_Hz             = (double) R2_FREQ_Hz;
    d_RHB2_decimation_factor = (double) RHB2_decimation_factor;
  }

  val = d_R2_FREQ_Hz / d_RHB2_decimation_factor;

  return 0;
}

const char* get_AD9361_R1_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_R2_FREQ_step_Hz;
  double d_RHB2_decimation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double R2_FREQ_step_Hz;
    uint8_t RHB2_decimation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_R2_FREQ_step_Hz(       app, inst, R2_FREQ_step_Hz       );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_RHB2_decimation_factor(app, inst, RHB2_decimation_factor);
    if(ret != 0) { return ret; }

    d_R2_FREQ_step_Hz        = (double) R2_FREQ_step_Hz;
    d_RHB2_decimation_factor = (double) RHB2_decimation_factor;
  }

  val = d_R2_FREQ_step_Hz / d_RHB2_decimation_factor;

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 CLKRF frequency in Hz
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
const char* get_AD9361_CLKRF_FREQ_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_R1_FREQ_Hz;
  double d_RHB1_decimation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double R1_FREQ_Hz;
    uint8_t RHB1_decimation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_R1_FREQ_Hz(            app, inst, R1_FREQ_Hz            );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_RHB1_decimation_factor(app, inst, RHB1_decimation_factor);
    if(ret != 0) { return ret; }

    d_R1_FREQ_Hz             = (double) R1_FREQ_Hz;
    d_RHB1_decimation_factor = (double) RHB1_decimation_factor;
  }

  val = d_R1_FREQ_Hz / d_RHB1_decimation_factor;

  return 0;
}

/*! @brief Get the nominal in-situ value with double floating point precision
 *         of the
 *         AD9361 CLKRF frequency step size above or below the current
 *         CLKRF frequency in Hz
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
const char* get_AD9361_CLKRF_FREQ_step_Hz(
    OCPI::API::Application& app, const char* app_inst_name_proxy,
    double& val)
{
  double d_R1_FREQ_step_Hz;
  double d_RHB1_decimation_factor;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation
    double R1_FREQ_step_Hz;
    uint8_t RHB1_decimation_factor;

    char* ret;
    const char* inst = app_inst_name_proxy;

    ret = (char*) get_AD9361_R1_FREQ_step_Hz(       app, inst, R1_FREQ_step_Hz       );
    if(ret != 0) { return ret; }
    ret = (char*) get_AD9361_RHB1_decimation_factor(app, inst, RHB1_decimation_factor);
    if(ret != 0) { return ret; }

    d_R1_FREQ_step_Hz        = (double) R1_FREQ_step_Hz;
    d_RHB1_decimation_factor = (double) RHB1_decimation_factor;
  }

  val = d_R1_FREQ_step_Hz / d_RHB1_decimation_factor;

  return 0;
}

#endif // _READERS_AD9361_RX_ADC_H
