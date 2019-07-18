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

#ifndef _CALC_AD9361_TX_FILTERS_DIGITAL_H
#define _CALC_AD9361_TX_FILTERS_DIGITAL_H

/*! @file
 *  @brief
 * \verbatim
 
    TX_SAMPL_FREQ..corresponds to the overall effective DAC sampling
                   rate (CLKTF_FREQ) divided by the TX FIR interpolation factor.
                   The unit is in complex samples per second.
    CLKTF_FREQ.....corresponds to the overall effective DAC sampling
                   rate in complex samples per second. (I *think* this is
                   assumed to be the TX_FB_CLK pin's clock rate, the AD9361
                   probably synchronizes at some point between the TX_FB_CLK
                   domain and the clocks derived from the DAC_FREQ clock
                   domain.)

    x1,x2,x3,x4 below refers to the multiplication of the clock rate

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
   |                                                                           |
   +---------------------------------------------------------------------------+
 
   \endverbatim
 *
 ******************************************************************************/

#include <cstdint>         // uint8_t type
#include <sstream>         // std::ostringstream
#include "ad9361_common.h" // BITMASK_D... macros

struct regs_general_tx_enable_filter_ctrl_t {
  uint8_t general_tx_enable_filter_ctrl;
};

typedef regs_general_tx_enable_filter_ctrl_t regs_calc_AD9361_THB3_interpolation_factor_t;

struct calc_AD9361_THB3_interpolation_factor_t {
  DEFINE_AD9361_SETTING(THB3_interpolation_factor , uint8_t)
};

void get_min_AD9361_THB3_interpolation_factor(
    calc_AD9361_THB3_interpolation_factor_t& calc_obj) {
  calc_obj.THB3_interpolation_factor = 1;
}

void get_max_AD9361_THB3_interpolation_factor(
    calc_AD9361_THB3_interpolation_factor_t& calc_obj) {
  calc_obj.THB3_interpolation_factor = 3;
}

const char* calc_AD9361_THB3_interpolation_factor(
    calc_AD9361_THB3_interpolation_factor_t& calc_obj,
    const regs_calc_AD9361_THB3_interpolation_factor_t& regs) {
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
  uint8_t reg = regs.general_tx_enable_filter_ctrl;
  uint8_t THB3_Enable_and_Interp_1_0;
  {
    // operands to binary operators (e.g. >>) always undergo integral promotion
    int tmp = (reg & (BITMASK_D5 | BITMASK_D4)) >> 4;
    THB3_Enable_and_Interp_1_0 = (uint8_t)(tmp & 0xff);
  }
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 5 Table 2.
  switch(THB3_Enable_and_Interp_1_0) {
    case 0x00: calc_obj.THB3_interpolation_factor = 1; break;
    case 0x01: calc_obj.THB3_interpolation_factor = 2; break;
    case 0x02: calc_obj.THB3_interpolation_factor = 3; break;
    default:
      std::ostringstream oss;
      oss << "could not calculate AD9361 THB3 interpolation factor due to ";
      oss << "invalid value of " << reg << "for register 0x002";
      return oss.str().c_str();
  }
  return 0;
}

typedef regs_general_tx_enable_filter_ctrl_t regs_calc_AD9361_THB2_interpolation_factor_t;

struct calc_AD9361_THB2_interpolation_factor_t {
  DEFINE_AD9361_SETTING(THB2_interpolation_factor , uint8_t)
};

void get_min_AD9361_THB2_interpolation_factor(
    calc_AD9361_THB2_interpolation_factor_t& calc_obj) {
  calc_obj.THB2_interpolation_factor = 1;
}

void get_max_AD9361_THB2_interpolation_factor(
    calc_AD9361_THB2_interpolation_factor_t& calc_obj) {
  calc_obj.THB2_interpolation_factor = 2;
}

const char* calc_AD9361_THB2_interpolation_factor(
    calc_AD9361_THB2_interpolation_factor_t& calc_obj,
    const regs_calc_AD9361_THB2_interpolation_factor_t& regs) {
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
  uint8_t reg = regs.general_tx_enable_filter_ctrl;
  uint8_t THB2_Enable;
  {
    // operands to binary operators (e.g. >>) always undergo integral promotion
    int tmp = (reg & BITMASK_D3) >> 3;
    THB2_Enable = (uint8_t)(tmp & 0xff);
  }
  calc_obj.THB2_interpolation_factor = (THB2_Enable == 1) ? 2 : 1;

  return 0;
}

typedef regs_general_tx_enable_filter_ctrl_t regs_calc_AD9361_THB1_interpolation_factor_t;

struct calc_AD9361_THB1_interpolation_factor_t {
  DEFINE_AD9361_SETTING(THB1_interpolation_factor , uint8_t)
};

void get_min_AD9361_THB1_interpolation_factor(
    calc_AD9361_THB1_interpolation_factor_t& calc_obj) {
  calc_obj.THB1_interpolation_factor = 1;
}

void get_max_AD9361_THB1_interpolation_factor(
    calc_AD9361_THB1_interpolation_factor_t& calc_obj) {
  calc_obj.THB1_interpolation_factor = 2;
}

const char* calc_AD9361_THB1_interpolation_factor(
    calc_AD9361_THB1_interpolation_factor_t& calc_obj,
    const regs_calc_AD9361_THB1_interpolation_factor_t& regs) {
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
  uint8_t reg = regs.general_tx_enable_filter_ctrl;
  uint8_t THB1_Enable;
  {
    // operands to binary operators (e.g. >>) always undergo integral promotion
    int tmp = (reg & BITMASK_D2) >> 2;
    THB1_Enable = (uint8_t)(tmp & 0xff);
  }
  calc_obj.THB1_interpolation_factor = (THB1_Enable == 1) ? 2 : 1;

  return 0;
}

typedef regs_general_tx_enable_filter_ctrl_t regs_calc_AD9361_Tx_FIR_interpolation_factor_t;

enum class AD9361_Tx_FIR_interpolation_factor_t {one=1, two=2, four=4};

struct calc_AD9361_Tx_FIR_interpolation_factor_t {
  AD9361_Tx_FIR_interpolation_factor_t Tx_FIR_interpolation_factor;
};

void get_min_AD9361_Tx_FIR_interpolation_factor(
    calc_AD9361_Tx_FIR_interpolation_factor_t& calc_obj) {
  calc_obj.Tx_FIR_interpolation_factor = AD9361_Tx_FIR_interpolation_factor_t::one;
}

void get_max_AD9361_Tx_FIR_interpolation_factor(
    calc_AD9361_Tx_FIR_interpolation_factor_t& calc_obj) {
  calc_obj.Tx_FIR_interpolation_factor = AD9361_Tx_FIR_interpolation_factor_t::four;
}

/*! @brief Calculate based upon register contents.
 ******************************************************************************/
const char* calc_AD9361_Tx_FIR_interpolation_factor(
    calc_AD9361_Tx_FIR_interpolation_factor_t& calc_obj,
    const regs_calc_AD9361_Tx_FIR_interpolation_factor_t& regs) {
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 4
  uint8_t reg = regs.general_tx_enable_filter_ctrl;
  uint8_t Tx_FIR_Enable_and_Decimation_1_0 = (reg & (BITMASK_D1 | BITMASK_D0));
  switch(Tx_FIR_Enable_and_Decimation_1_0) {
    case 0x00:
      calc_obj.Tx_FIR_interpolation_factor = AD9361_Tx_FIR_interpolation_factor_t::one;
      break;
    case 0x01:
      calc_obj.Tx_FIR_interpolation_factor = AD9361_Tx_FIR_interpolation_factor_t::one;
      break;
    case 0x02:
      calc_obj.Tx_FIR_interpolation_factor = AD9361_Tx_FIR_interpolation_factor_t::two;
      break;
    default:
      calc_obj.Tx_FIR_interpolation_factor = AD9361_Tx_FIR_interpolation_factor_t::four;
      break;
  }
  return 0;
};

#endif // _CALC_AD9361_TX_FILTERS_DIGITAL_H
