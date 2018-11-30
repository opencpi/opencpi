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

#ifndef _CALC_AD9361_BBPLL_H
#define _CALC_AD9361_BBPLL_H

/*! @file
 *  @brief
 *
 * \verbatim
                         AD9361 IC
   +---------------------------------------------------------------------------+
   |                                                                           |
   |                       BBPLL                                               |
   |         +-------------------------------+                                 |
   |         |   ref                         |                                 |
   |         |   scaler                      |                                 |
   |         |   +---+   +-----------------+ |             +----+              |
   |         |   |   |   | x (N_integer    | | BBPLL_FREQ  |    |              |
   | F_REF-->|-->|   |-->| + [N_fractional |-|------------>|    |--> ADC_FREQ  |
   |         |   +---+   | /BBPLL_MODULUS)]| |             |    |              |
   |         |    x1     +-----------------+ |             +----+              |
   |         |    /2                         |             /1,  /2             |
   |         |    /4                         |             /4,  /8             |
   |         |    *2                         |             /16, /32            |
   |         |                               |             /64, /128           |
   |         |                               |             (/[2^BBPLL_Divider])|
   |         +-------------------------------+                                 |
   +---------------------------------------------------------------------------+
  
   BBPLL_freq formula
   BBPLL_freq = F_REF*ref_scaler*(N_Integer + (N_Fractional/BBPLL_MODULUS))
  
   \endverbatim
 *
 ******************************************************************************/

#include <cstdint>         // uint8_t, uint32_t types
#include <sstream>         // std::ostringstream
#include "ad9361.h"        // BBPLL_MODULUS macro
#include "ad9361_common.h" // regs_clock_bbpll_t

struct regs_calc_AD9361_BBPLL_ref_scaler_t {
  uint8_t bbpll_ref_clock_scaler;
};

struct calc_AD9361_BBPLL_ref_scaler_t {
  typedef float BBPLL_ref_scaler_t;
  BBPLL_ref_scaler_t BBPLL_ref_scaler ;
};

void get_min_AD9361_BBPLL_ref_scaler(
    calc_AD9361_BBPLL_ref_scaler_t& calc_obj) {
  calc_obj.BBPLL_ref_scaler = 0.25F;
}

void get_max_AD9361_BBPLL_ref_scaler(
    calc_AD9361_BBPLL_ref_scaler_t& calc_obj) {
  calc_obj.BBPLL_ref_scaler = 2.0F;
}

const char* regs_calc_AD9361_BBPLL_ref_scaler(
    calc_AD9361_BBPLL_ref_scaler_t& calc_obj,
    const regs_calc_AD9361_BBPLL_ref_scaler_t& regs) {
  // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 21:
  // SPI Register 0x045-Ref Clock Scaler
  // [D1:D0]-Ref Clock Scaler[1:0]
  // The reference clock frequency is scaled before it enters the
  // BBPLL. 00: x1; 01: x1/2; 10: x1/4; 11: x2.
  uint8_t Ref_Clock_Scaler_1_0 = regs.bbpll_ref_clock_scaler & 0x03;
  switch(Ref_Clock_Scaler_1_0)
  {
    case        0x00: calc_obj.BBPLL_ref_scaler = 1.0F;  break;
    case        0x01: calc_obj.BBPLL_ref_scaler = 0.5F;  break;
    case        0x02: calc_obj.BBPLL_ref_scaler = 0.25F; break;
    default: // 0x03
                      calc_obj.BBPLL_ref_scaler = 2.0F;  break;
  }
  return 0;
}

struct regs_calc_AD9361_BBPLL_N_Integer_t {
  uint8_t bbpll_integer_bb_freq_word;
};

struct calc_AD9361_BBPLL_N_Integer_t {
  typedef uint8_t BBPLL_N_Integer_t;
  BBPLL_N_Integer_t BBPLL_N_Integer;
};

void get_min_AD9361_BBPLL_N_Integer(
    calc_AD9361_BBPLL_N_Integer_t& calc_obj) {
  calc_obj.BBPLL_N_Integer = 0;
}

void get_max_AD9361_BBPLL_N_Integer(
    calc_AD9361_BBPLL_N_Integer_t& calc_obj) {
  calc_obj.BBPLL_N_Integer = 255; // (2^8)-1
}

const char* regs_calc_AD9361_BBPLL_N_Integer(
    calc_AD9361_BBPLL_N_Integer_t& calc_obj,
    const regs_calc_AD9361_BBPLL_N_Integer_t& regs) {
  calc_obj.BBPLL_N_Integer = regs.bbpll_integer_bb_freq_word;
  return 0;
}

struct regs_calc_AD9361_BBPLL_N_Fractional_t {
  uint8_t bbpll_fract_bb_freq_word_1;
  uint8_t bbpll_fract_bb_freq_word_2;
  uint8_t bbpll_fract_bb_freq_word_3;
};

struct calc_AD9361_BBPLL_N_Fractional_t {
  typedef uint32_t BBPLL_N_Fractional_t;
  BBPLL_N_Fractional_t BBPLL_N_Fractional;
};

void get_min_AD9361_BBPLL_N_Fractional(
    calc_AD9361_BBPLL_N_Fractional_t& calc_obj) {
  calc_obj.BBPLL_N_Fractional = 0;
}

void get_max_AD9361_BBPLL_N_Fractional(
    calc_AD9361_BBPLL_N_Fractional_t& calc_obj) {
  calc_obj.BBPLL_N_Fractional = 1048575; // (2^20)-1
}

const char* regs_calc_AD9361_BBPLL_N_Fractional(
    calc_AD9361_BBPLL_N_Fractional_t& calc_obj,
    const regs_calc_AD9361_BBPLL_N_Fractional_t& regs) {
  uint32_t tmp = 0;

  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 19

  // Fractional BB Frequency Word[20:16]
  {
    uint8_t reg = regs.bbpll_fract_bb_freq_word_1;
    uint8_t Fractional_BB_Frequency_Word_20_16 = reg & 0x1f;
    tmp |= ((uint32_t)Fractional_BB_Frequency_Word_20_16) << 16;
  }

  // Fractional BB Frequency Word[15:8]
  {
    uint8_t reg = regs.bbpll_fract_bb_freq_word_2;
    uint8_t Fractional_BB_Frequency_Word_15_8 = reg;
    tmp |= ((uint32_t)Fractional_BB_Frequency_Word_15_8) << 8;
  }

  // Fractional BB Frequency Word[7:0]
  {
    uint8_t reg = regs.bbpll_fract_bb_freq_word_3;
    uint8_t Fractional_BB_Frequency_Word_7_0 = reg;
    tmp |= ((uint32_t)Fractional_BB_Frequency_Word_7_0);
  }

  calc_obj.BBPLL_N_Fractional = tmp;

  return 0;
}

struct regs_calc_AD9361_BBPLL_FREQ_Hz_t :
       regs_calc_AD9361_BBPLL_ref_scaler_t,
       regs_calc_AD9361_BBPLL_N_Integer_t,
       regs_calc_AD9361_BBPLL_N_Fractional_t {
};

struct calc_AD9361_BBPLL_FREQ_Hz_t :
       calc_AD9361_BBPLL_ref_scaler_t ,
       calc_AD9361_BBPLL_N_Integer_t,
       calc_AD9361_BBPLL_N_Fractional_t {
  typedef double BBPLL_input_F_REF_t;
  BBPLL_input_F_REF_t BBPLL_input_F_REF;
  typedef double BBPLL_FREQ_Hz_t;
  BBPLL_FREQ_Hz_t BBPLL_FREQ_Hz;
};

void calc_AD9361_BBPLL_FREQ_Hz(
    calc_AD9361_BBPLL_FREQ_Hz_t& calc_obj) {
  // see "BBPLL_freq formula" for explanation of the following;
  double frac, x;
  frac  = (double)calc_obj.BBPLL_N_Fractional;
  frac /= (double)BBPLL_MODULUS;
  x     = (double)calc_obj.BBPLL_input_F_REF;
  x    *= (double)calc_obj.BBPLL_ref_scaler;
  x    *= (((double)calc_obj.BBPLL_N_Integer)+frac);
  calc_obj.BBPLL_FREQ_Hz = x;
}

void calc_min_AD9361_BBPLL_FREQ_Hz(
    //double AD9361_BBPLL_input_F_REF,
    calc_AD9361_BBPLL_FREQ_Hz_t& calc_obj) {
  //calc_obj.BBPLL_input_F_REF = AD9361_BBPLL_input_F_REF;
  get_min_AD9361_BBPLL_ref_scaler(  calc_obj);
  get_min_AD9361_BBPLL_N_Integer(   calc_obj);
  get_min_AD9361_BBPLL_N_Fractional(calc_obj);
  calc_AD9361_BBPLL_FREQ_Hz(        calc_obj);
  calc_obj.BBPLL_FREQ_Hz = std::max(715e6, calc_obj.BBPLL_FREQ_Hz);
  calc_obj.BBPLL_FREQ_Hz = std::min(1430e6, calc_obj.BBPLL_FREQ_Hz);
}

void calc_max_AD9361_BBPLL_FREQ_Hz(
    //double AD9361_BBPLL_input_F_REF,
    calc_AD9361_BBPLL_FREQ_Hz_t& calc_obj) {
  //calc_obj.BBPLL_input_F_REF = AD9361_BBPLL_input_F_REF;
  
  get_max_AD9361_BBPLL_ref_scaler(  calc_obj);
  get_max_AD9361_BBPLL_N_Integer(   calc_obj);
  get_max_AD9361_BBPLL_N_Fractional(calc_obj);
  calc_AD9361_BBPLL_FREQ_Hz(        calc_obj);
  calc_obj.BBPLL_FREQ_Hz = std::max(715e6, calc_obj.BBPLL_FREQ_Hz);
  calc_obj.BBPLL_FREQ_Hz = std::min(1430e6, calc_obj.BBPLL_FREQ_Hz);
}

const char* regs_calc_AD9361_BBPLL_FREQ_Hz(
    calc_AD9361_BBPLL_FREQ_Hz_t& calc_obj,
    const regs_calc_AD9361_BBPLL_FREQ_Hz_t& regs) {

  char* ret;

  ret = (char*) regs_calc_AD9361_BBPLL_ref_scaler(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_BBPLL_N_Integer(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }
  ret = (char*) regs_calc_AD9361_BBPLL_N_Fractional(calc_obj, regs);
  if(ret != 0) {
    return ret;
  }

  calc_AD9361_BBPLL_FREQ_Hz(calc_obj);

  return 0;
}

typedef regs_clock_bbpll_t regs_calc_AD9361_BBPLL_Divider_t;

struct calc_AD9361_BBPLL_Divider_t {
  typedef uint8_t BBPLL_Divider_t;
  BBPLL_Divider_t BBPLL_Divider;
};

void get_min_AD9361_BBPLL_Divider(
    calc_AD9361_BBPLL_Divider_t& calc_obj) {
  // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 7:
  // SPI Register 0x00A-BBPLL
  // [D2:D0]-BBPLL Divider[2:0]
  // BBPLL Divider[2:0] is valid from 1 through 6.
  calc_obj.BBPLL_Divider = 1;
}

void get_max_AD9361_BBPLL_Divider(
    calc_AD9361_BBPLL_Divider_t& calc_obj) {
  // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 7:
  // SPI Register 0x00A-BBPLL
  // [D2:D0]-BBPLL Divider[2:0]
  // BBPLL Divider[2:0] is valid from 1 through 6.
  calc_obj.BBPLL_Divider = 6;
}

const char* regs_calc_AD9361_BBPLL_Divider(
    calc_AD9361_BBPLL_Divider_t& calc_obj,
    const regs_calc_AD9361_BBPLL_Divider_t& regs) {
  // from AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 7:
  // SPI Register 0x00A-BBPLL
  // [D2:D0]-BBPLL Divider[2:0]
  // BBPLL Divider[2:0] is valid from 1 through 6.
  uint8_t BBPLL_Divider_2_0 = (regs.clock_bbpll & 0x07);
  if((BBPLL_Divider_2_0 == 0) or (BBPLL_Divider_2_0 >= 7)) {
    std::ostringstream oss;
    oss << "could not calculate AD9361 BBPLL Divider due to invalid value ";
    oss << "of " << regs.clock_bbpll << "for register 0x00A";
    return oss.str().c_str();
  }
  calc_obj.BBPLL_Divider = BBPLL_Divider_2_0;

  return 0;
}

#endif // _CALC_AD9361_BBPLL_H
