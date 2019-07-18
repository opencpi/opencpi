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

#ifndef _CALC_AD9361_RX_ADC_H
#define _CALC_AD9361_RX_ADC_H

/*! @file
 *  @brief
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

#include <cmath>   // pow()
#include "calc_ad9361_bb_pll.h"
#include "calc_ad9361_bb_rx_filters_digital.h"

struct regs_calc_AD9361_ADC_FREQ_Hz_t :
       regs_calc_AD9361_BBPLL_FREQ_Hz_t,
       regs_calc_AD9361_BBPLL_Divider_t {
};

struct calc_AD9361_ADC_FREQ_Hz_t :
       calc_AD9361_BBPLL_FREQ_Hz_t,
       calc_AD9361_BBPLL_Divider_t {
  DEFINE_AD9361_SETTING(ADC_FREQ_Hz, double)
};

void calc_AD9361_ADC_FREQ_Hz(
    calc_AD9361_ADC_FREQ_Hz_t& calc_obj) {
  // AD9361_Register_Map_Reference_Manual_UG-671.pdf pg. 7:
  // "The ADC clock rate equals the BBPLL divided by the factor in this
  // register, shown in Equation 2.
  // ADC Clock Rate = BBPLL Clock Rate / (2^(BBPLL Divider[2:0] (decimal)))"
  double tmp;
  tmp  = ((double)calc_obj.BBPLL_FREQ_Hz);
  tmp /= pow(2.,((double)calc_obj.BBPLL_Divider));
  calc_obj.ADC_FREQ_Hz = tmp;
}

void calc_min_AD9361_ADC_FREQ_Hz(
    calc_AD9361_ADC_FREQ_Hz_t& calc_obj) {

  calc_min_AD9361_BBPLL_FREQ_Hz(calc_obj);
  get_max_AD9361_BBPLL_Divider( calc_obj);
  calc_AD9361_ADC_FREQ_Hz(      calc_obj);
  // ADC FREQ is not valid below 25 MHz or above 640 MHz
  calc_obj.ADC_FREQ_Hz = std::max(25e6, calc_obj.ADC_FREQ_Hz);
  calc_obj.ADC_FREQ_Hz = std::min(640e6, calc_obj.ADC_FREQ_Hz);
}

void calc_max_AD9361_ADC_FREQ_Hz(
    calc_AD9361_ADC_FREQ_Hz_t& calc_obj) {

  calc_max_AD9361_BBPLL_FREQ_Hz(calc_obj);
  get_min_AD9361_BBPLL_Divider( calc_obj);
  calc_AD9361_ADC_FREQ_Hz(      calc_obj);
  // ADC FREQ is not valid below 25 MHz or above 640 MHz
  calc_obj.ADC_FREQ_Hz = std::max(25e6, calc_obj.ADC_FREQ_Hz);
  calc_obj.ADC_FREQ_Hz = std::min(640e6, calc_obj.ADC_FREQ_Hz);
}

/*! @brief Calculate based upon register contents.
 ******************************************************************************/
const char* calc_AD9361_ADC_FREQ_Hz(
    calc_AD9361_ADC_FREQ_Hz_t& calc_obj,
    const regs_calc_AD9361_ADC_FREQ_Hz_t& regs) {

  char* ret;

  ret = (char*) regs_calc_AD9361_BBPLL_FREQ_Hz(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_BBPLL_Divider(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }

  calc_AD9361_ADC_FREQ_Hz(calc_obj);
  return 0;
}

struct regs_calc_AD9361_R2_FREQ_Hz_t :
       regs_calc_AD9361_ADC_FREQ_Hz_t,
       regs_calc_AD9361_RHB3_decimation_factor_t {
};

struct calc_AD9361_R2_FREQ_Hz_t :
       calc_AD9361_ADC_FREQ_Hz_t,
       calc_AD9361_RHB3_decimation_factor_t {
  DEFINE_AD9361_SETTING(R2_FREQ_Hz, double)
};

void calc_AD9361_R2_FREQ_Hz(
    calc_AD9361_R2_FREQ_Hz_t& calc_obj) {
  double tmp;
  tmp = (double)calc_obj.ADC_FREQ_Hz;
  tmp /= (double)calc_obj.RHB3_decimation_factor;
  calc_obj.R2_FREQ_Hz = tmp;
}

void calc_min_AD9361_R2_FREQ_Hz(
    calc_AD9361_R2_FREQ_Hz_t& calc_obj) {

  calc_min_AD9361_ADC_FREQ_Hz(          calc_obj);
  get_max_AD9361_RHB3_decimation_factor(calc_obj);
  calc_AD9361_R2_FREQ_Hz(               calc_obj);
}

void calc_max_AD9361_R2_FREQ_Hz(
    calc_AD9361_R2_FREQ_Hz_t& calc_obj) {

  calc_max_AD9361_ADC_FREQ_Hz(          calc_obj);
  get_min_AD9361_RHB3_decimation_factor(calc_obj);
  calc_AD9361_R2_FREQ_Hz(               calc_obj);
}

const char* regs_calc_AD9361_R2_FREQ_Hz(
    calc_AD9361_R2_FREQ_Hz_t& calc_obj,
    const regs_calc_AD9361_R2_FREQ_Hz_t& regs) {

  char* ret;

  ret = (char*) calc_AD9361_ADC_FREQ_Hz(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_RHB3_decimation_factor(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }

  calc_AD9361_R2_FREQ_Hz(calc_obj);

  return 0;
}

typedef regs_calc_AD9361_R2_FREQ_Hz_t regs_calc_AD9361_R1_FREQ_Hz_t;

struct calc_AD9361_R1_FREQ_Hz_t :
       calc_AD9361_R2_FREQ_Hz_t,
       calc_AD9361_RHB2_decimation_factor_t {
  DEFINE_AD9361_SETTING(R1_FREQ_Hz, double)
};

void calc_AD9361_R1_FREQ_Hz(
    calc_AD9361_R1_FREQ_Hz_t& calc_obj) {
  double tmp;
  tmp  = (double)calc_obj.R2_FREQ_Hz;
  tmp /= (double)calc_obj.RHB2_decimation_factor;
  calc_obj.R1_FREQ_Hz = tmp;
}

void calc_min_AD9361_R1_FREQ_Hz(
    calc_AD9361_R1_FREQ_Hz_t& calc_obj) {

  calc_min_AD9361_R2_FREQ_Hz(           calc_obj);
  get_max_AD9361_RHB2_decimation_factor(calc_obj);
  calc_AD9361_R1_FREQ_Hz(               calc_obj);
}

void calc_max_AD9361_R1_FREQ_Hz(
    calc_AD9361_R1_FREQ_Hz_t& calc_obj) {

  calc_max_AD9361_R2_FREQ_Hz(           calc_obj);
  get_min_AD9361_RHB2_decimation_factor(calc_obj);
  calc_AD9361_R1_FREQ_Hz(               calc_obj);
}

const char* regs_calc_AD9361_R1_FREQ_Hz(
    calc_AD9361_R1_FREQ_Hz_t& calc_obj,
    const regs_calc_AD9361_R1_FREQ_Hz_t& regs) {

  char* ret;

  ret = (char*) regs_calc_AD9361_R2_FREQ_Hz(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_RHB2_decimation_factor(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }

  calc_AD9361_R1_FREQ_Hz(calc_obj);

  return 0;
}

typedef regs_calc_AD9361_R1_FREQ_Hz_t regs_calc_AD9361_CLKRF_FREQ_Hz_t;

struct calc_AD9361_CLKRF_FREQ_Hz_t :
       calc_AD9361_R1_FREQ_Hz_t,
       calc_AD9361_RHB1_decimation_factor_t {
  DEFINE_AD9361_SETTING(CLKRF_FREQ_Hz, double)
};

void calc_AD9361_CLKRF_FREQ_Hz(
    calc_AD9361_CLKRF_FREQ_Hz_t& calc_obj) {
  double tmp;
  tmp  = (double)calc_obj.R1_FREQ_Hz;
  tmp /= (double)calc_obj.RHB1_decimation_factor;
  calc_obj.CLKRF_FREQ_Hz = tmp;
}

void calc_min_AD9361_CLKRF_FREQ_Hz(
    calc_AD9361_CLKRF_FREQ_Hz_t& calc_obj) {

  calc_min_AD9361_R1_FREQ_Hz(           calc_obj);
  get_max_AD9361_RHB1_decimation_factor(calc_obj);
  calc_AD9361_CLKRF_FREQ_Hz(            calc_obj);
  // CLKRF FREQ is only valid at or below 61.44 MHz
  calc_obj.CLKRF_FREQ_Hz = std::min(61.44e6, calc_obj.CLKRF_FREQ_Hz);
}

void calc_max_AD9361_CLKRF_FREQ_Hz(
    calc_AD9361_CLKRF_FREQ_Hz_t& calc_obj) {

  calc_max_AD9361_R1_FREQ_Hz(           calc_obj);
  get_min_AD9361_RHB1_decimation_factor(calc_obj);
  calc_AD9361_CLKRF_FREQ_Hz(            calc_obj);
  // CLKRF FREQ is only valid at or below 61.44 MHz
  calc_obj.CLKRF_FREQ_Hz = std::min(61.44e6, calc_obj.CLKRF_FREQ_Hz);
}

const char* regs_calc_AD9361_CLKRF_FREQ_Hz(
    calc_AD9361_CLKRF_FREQ_Hz_t& calc_obj,
    const regs_calc_AD9361_CLKRF_FREQ_Hz_t& regs) {

  char* ret;

  ret = (char*) regs_calc_AD9361_R1_FREQ_Hz(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_RHB1_decimation_factor(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }

  calc_AD9361_CLKRF_FREQ_Hz(calc_obj);

  return 0;
}

typedef regs_calc_AD9361_R1_FREQ_Hz_t regs_calc_AD9361_RX_SAMPL_FREQ_Hz_t;

struct calc_AD9361_RX_SAMPL_FREQ_Hz_t :
       calc_AD9361_CLKRF_FREQ_Hz_t,
       calc_AD9361_Rx_FIR_decimation_factor_t {
  DEFINE_AD9361_SETTING(RX_SAMPL_FREQ_Hz, double)
};

void calc_AD9361_RX_SAMPL_FREQ_Hz(
    calc_AD9361_RX_SAMPL_FREQ_Hz_t& calc_obj) {
  double tmp;
  tmp  = (double)calc_obj.CLKRF_FREQ_Hz;
  tmp /= (double)calc_obj.Rx_FIR_decimation_factor;
  calc_obj.RX_SAMPL_FREQ_Hz = tmp;
}

void calc_min_AD9361_RX_SAMPL_FREQ_Hz(
    calc_AD9361_RX_SAMPL_FREQ_Hz_t& calc_obj) {

  calc_min_AD9361_CLKRF_FREQ_Hz(          calc_obj);
  get_max_AD9361_Rx_FIR_decimation_factor(calc_obj);
  calc_AD9361_RX_SAMPL_FREQ_Hz(           calc_obj);
}

void calc_max_AD9361_RX_SAMPL_FREQ_Hz(
    calc_AD9361_RX_SAMPL_FREQ_Hz_t& calc_obj) {

  calc_max_AD9361_CLKRF_FREQ_Hz(          calc_obj);
  get_min_AD9361_Rx_FIR_decimation_factor(calc_obj);
  calc_AD9361_RX_SAMPL_FREQ_Hz(           calc_obj);
}

calc_AD9361_RX_SAMPL_FREQ_Hz_t::RX_SAMPL_FREQ_Hz_t 
calc_min_AD9361_RX_SAMPL_FREQ_Hz(
calc_AD9361_RX_SAMPL_FREQ_Hz_t::BBPLL_FREQ_Hz_t BBPLL_input_F_REF) {
  calc_AD9361_RX_SAMPL_FREQ_Hz_t calc_obj;
  calc_obj.BBPLL_input_F_REF = BBPLL_input_F_REF;
  calc_min_AD9361_RX_SAMPL_FREQ_Hz(calc_obj);
//#define PRINT_VAR(x) std::cout << #x"=" << x << "\n"
//  PRINT_VAR(calc_obj.BBPLL_FREQ_Hz);
//  PRINT_VAR(calc_obj.ADC_FREQ_Hz);
//  PRINT_VAR(calc_obj.R2_FREQ_Hz);
//  PRINT_VAR(calc_obj.R1_FREQ_Hz);
//  PRINT_VAR(calc_obj.CLKRF_FREQ_Hz);
  return calc_obj.RX_SAMPL_FREQ_Hz;
}

calc_AD9361_RX_SAMPL_FREQ_Hz_t::RX_SAMPL_FREQ_Hz_t 
calc_max_AD9361_RX_SAMPL_FREQ_Hz(
calc_AD9361_RX_SAMPL_FREQ_Hz_t::BBPLL_FREQ_Hz_t BBPLL_input_F_REF) {
  calc_AD9361_RX_SAMPL_FREQ_Hz_t calc_obj;
  calc_obj.BBPLL_input_F_REF = BBPLL_input_F_REF;
  calc_max_AD9361_RX_SAMPL_FREQ_Hz(calc_obj);
//  PRINT_VAR(calc_obj.BBPLL_FREQ_Hz);
//  PRINT_VAR(calc_obj.ADC_FREQ_Hz);
//  PRINT_VAR(calc_obj.R2_FREQ_Hz);
//  PRINT_VAR(calc_obj.R1_FREQ_Hz);
//  PRINT_VAR(calc_obj.CLKRF_FREQ_Hz);
  return calc_obj.RX_SAMPL_FREQ_Hz;
}

calc_AD9361_RX_SAMPL_FREQ_Hz_t::RX_SAMPL_FREQ_Hz_t 
calc_min_AD9361_RX_SAMPL_FREQ_Hz(
calc_AD9361_RX_SAMPL_FREQ_Hz_t::BBPLL_FREQ_Hz_t BBPLL_input_F_REF,
AD9361_Rx_FIR_decimation_factor_t Rx_FIR_decimation_factor) {

  calc_AD9361_RX_SAMPL_FREQ_Hz_t calc_obj;

  calc_obj.BBPLL_input_F_REF = BBPLL_input_F_REF;

  calc_min_AD9361_CLKRF_FREQ_Hz(calc_obj);

  calc_obj.Rx_FIR_decimation_factor = Rx_FIR_decimation_factor;

  calc_AD9361_RX_SAMPL_FREQ_Hz(calc_obj);
  return calc_obj.RX_SAMPL_FREQ_Hz;

}

calc_AD9361_RX_SAMPL_FREQ_Hz_t::RX_SAMPL_FREQ_Hz_t 
calc_max_AD9361_RX_SAMPL_FREQ_Hz(
calc_AD9361_RX_SAMPL_FREQ_Hz_t::BBPLL_FREQ_Hz_t BBPLL_input_F_REF,
AD9361_Rx_FIR_decimation_factor_t Rx_FIR_decimation_factor) {

  calc_AD9361_RX_SAMPL_FREQ_Hz_t calc_obj;

  calc_obj.BBPLL_input_F_REF = BBPLL_input_F_REF;

  calc_max_AD9361_CLKRF_FREQ_Hz(calc_obj);

  calc_obj.Rx_FIR_decimation_factor = Rx_FIR_decimation_factor;

  get_min_AD9361_Rx_FIR_decimation_factor(calc_obj);

  calc_AD9361_RX_SAMPL_FREQ_Hz(calc_obj);
  return calc_obj.RX_SAMPL_FREQ_Hz;
}

const char* regs_calc_AD9361_RX_SAMPL_FREQ_Hz(
    calc_AD9361_RX_SAMPL_FREQ_Hz_t& calc_obj,
    const regs_calc_AD9361_RX_SAMPL_FREQ_Hz_t& regs) {

  char* ret;

  ret = (char*) regs_calc_AD9361_CLKRF_FREQ_Hz(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_Rx_FIR_decimation_factor(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }

  calc_AD9361_RX_SAMPL_FREQ_Hz(calc_obj);

  return 0;
}

#endif // _CALC_AD9361_RX_ADC_H
