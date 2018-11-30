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

#ifndef _CALC_AD9361_RX_RFPLL_H
#define _CALC_AD9361_RX_RFPLL_H

#include <sstream>         // std::ostringstream
#include <cstdint>         // uin32_t, etc
#include "ad9361_common.h" // BITMASK_D... macros, regs_general_rfpll_divider_t
#include "ad9361.h"        // RFPLL_MODULUS macro (this is a ADI No-OS header)

struct regs_calc_AD9361_Rx_RFPLL_ref_divider_t {
  uint8_t ref_divide_config_1;
  uint8_t ref_divide_config_2;
};

const char* calc_AD9361_Rx_RFPLL_ref_divider(
    float& val,
    const regs_calc_AD9361_Rx_RFPLL_ref_divider_t& regs) {
  // see AD9361_Register_Map_Reference_Manual_UG-671.pdf Rev. 0 pg. 68
  uint8_t Rx_Ref_Divider_1 = (uint8_t)(regs.ref_divide_config_1 & BITMASK_D0);
  uint8_t Rx_Ref_Divider_0 = (uint8_t)(regs.ref_divide_config_2 & BITMASK_D7) >> 7;
  uint8_t Rx_Ref_Divider_1_0 = (Rx_Ref_Divider_1 << 1) | Rx_Ref_Divider_0;
  switch(Rx_Ref_Divider_1_0)
  {
    case        0x00: val = 1.0F;  break;
    case        0x01: val = 0.5F;  break;
    case        0x02: val = 0.25F; break;
    default: // 0x03
                      val = 2.0F;  break;
  }
  return 0;
}

struct regs_calc_AD9361_Rx_RFPLL_N_Integer_t {
  uint8_t rx_synth_integer_byte_0;
  uint8_t rx_synth_integer_byte_1;
};

const char* calc_AD9361_Rx_RFPLL_N_Integer(
    uint16_t& val,
    const regs_calc_AD9361_Rx_RFPLL_N_Integer_t& regs) {
  uint16_t integer = (uint16_t)regs.rx_synth_integer_byte_0;
  integer |= ((uint16_t)((regs.rx_synth_integer_byte_1 & 0x07)) << 8);
  val = integer;

  return 0;
}

struct regs_calc_AD9361_Rx_RFPLL_N_Fractional_t {
  uint8_t rx_synth_fract_byte_0;
  uint8_t rx_synth_fract_byte_1;
  uint8_t rx_synth_fract_byte_2;
};

const char* calc_AD9361_Rx_RFPLL_N_Fractional(
    uint32_t& val,
    const regs_calc_AD9361_Rx_RFPLL_N_Fractional_t& regs) {
  uint32_t frac = (uint32_t)regs.rx_synth_fract_byte_0;
  frac |= ((uint32_t)regs.rx_synth_fract_byte_1) << 8;
  frac |= ((uint32_t)(regs.rx_synth_fract_byte_2 & 0x7f)) << 16;
  val = frac;

  return 0;
}

typedef regs_general_rfpll_divider_t regs_calc_AD9361_Rx_RFPLL_external_div_2_enable_t;

const char* calc_AD9361_Rx_RFPLL_external_div_2_enable(
    bool& val,
    const regs_calc_AD9361_Rx_RFPLL_external_div_2_enable_t& regs) {
  uint8_t divider = regs.general_rfpll_dividers & 0x0f;
  if(divider <= 7) {
    val = (divider == 7);
  }
  else {
    std::ostringstream oss;
    oss << "Invalid value read for ";
    oss << "general_rfpll_dividers register" << regs.general_rfpll_dividers;
    return oss.str().c_str();
  }
  return 0;
}

typedef regs_general_rfpll_divider_t regs_calc_AD9361_Rx_RFPLL_VCO_Divider_t;

const char* calc_AD9361_Rx_RFPLL_VCO_Divider(
    uint8_t& val,
    const regs_calc_AD9361_Rx_RFPLL_VCO_Divider_t& regs) {
  uint8_t divider = regs.general_rfpll_dividers & 0x0f;
  switch(divider)
  {
    case 0: val = 2; break;
    case 1: val = 4; break;
    case 2: val = 8; break;
    case 3: val = 16; break;
    case 4: val = 32; break;
    case 5: val = 64; break;
    case 6: val = 128; break;
    case 7:
    {
      std::ostringstream oss;
      oss << "Value requested for AD9361_Rx_RFPLL_VCO_Divider before ";
      oss << "checking if register was set to divide-by-2";
      return oss.str().c_str();
    }
    default:
      std::ostringstream oss;
      oss << "Invalid value read for ";
      oss << "general_rfpll_dividers register" << regs.general_rfpll_dividers;
      return oss.str().c_str();
  }
  return 0;
}

struct regs_calc_AD9361_Rx_RFPLL_LO_freq_Hz_t :
       regs_general_rfpll_divider_t,
       regs_calc_AD9361_Rx_RFPLL_ref_divider_t,
       regs_calc_AD9361_Rx_RFPLL_N_Integer_t,
       regs_calc_AD9361_Rx_RFPLL_N_Fractional_t {
};

/*! @brief Calculate with double floating point precision
 *         the value of the
 *             AD9361 RX RF LO frequency in Hz
 *         based on register values passed in via arguments.
 *
 *  @param[out] val                 Calculated value.
 *  @param[in]  Rx_RFPLL_input_FREF Frequency in Hz of RFPLL input clock.
 *  @param[in]  regs                Struct containing register values for
 *                                  the registers which the Rx RFPLL LO
 *                                  freq depends upon.
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
const char* calc_AD9361_Rx_RFPLL_LO_freq_Hz(
    double& val,
    double Rx_RFPLL_input_F_REF,
    const regs_calc_AD9361_Rx_RFPLL_LO_freq_Hz_t& regs) {
  double d_Rx_RFPLL_input_F_REF;
  double d_Rx_RFPLL_ref_divider;
  double d_Rx_RFPLL_N_Integer;
  double d_Rx_RFPLL_N_Fractional;
  double d_Rx_RFPLL_VCO_Divider;

  { // restrict scope so we don't accidentally use non-double values
    // for later calculation

    bool Rx_RFPLL_external_div_2_enable;
    {
      bool& val = Rx_RFPLL_external_div_2_enable;
      const char* ret = calc_AD9361_Rx_RFPLL_external_div_2_enable(val, regs);
      if(ret != 0) {
        return ret;
      }
    }

    if(Rx_RFPLL_external_div_2_enable) {
      val = ((double)Rx_RFPLL_input_F_REF) / 2.;
      return 0;
    }

    float Rx_RFPLL_ref_divider;
    {
      float& val = Rx_RFPLL_ref_divider;
      const char* ret = calc_AD9361_Rx_RFPLL_ref_divider(val, regs);
      if(ret != 0) {
        return ret;
      }
    }

    uint16_t Rx_RFPLL_N_Integer;
    {
      uint16_t& val = Rx_RFPLL_N_Integer;
      const char* ret = calc_AD9361_Rx_RFPLL_N_Integer(val, regs);
      if(ret != 0) {
        return ret;
      }
    }

    uint32_t Rx_RFPLL_N_Fractional;
    {
      uint32_t& val = Rx_RFPLL_N_Fractional;
      const char* ret = calc_AD9361_Rx_RFPLL_N_Fractional(val, regs);
      if(ret != 0) {
        return ret;
      }
    }

    uint8_t Rx_RFPLL_VCO_Divider;
    {
      uint8_t& val = Rx_RFPLL_VCO_Divider;
      const char* ret = calc_AD9361_Rx_RFPLL_VCO_Divider(val, regs);
      if(ret != 0) {
        return ret;
      }
    }

    d_Rx_RFPLL_input_F_REF  = (double) Rx_RFPLL_input_F_REF;
    d_Rx_RFPLL_ref_divider  = (double) Rx_RFPLL_ref_divider;
    d_Rx_RFPLL_N_Integer    = (double) Rx_RFPLL_N_Integer;
    d_Rx_RFPLL_N_Fractional = (double) Rx_RFPLL_N_Fractional;
    d_Rx_RFPLL_VCO_Divider  = (double) Rx_RFPLL_VCO_Divider;
  }

  //AD9361_Reference_Manual_UG-570.pdf Figure 4. PLL Synthesizer Block Diagram
  //(calculating the "FREF = 10MHz TO 80MHz" signal)
  double x = d_Rx_RFPLL_input_F_REF;
  // why multiply (*) not divide? (I suspect ADI's term "divider" is a misnomer)
  x *= d_Rx_RFPLL_ref_divider;

  //AD9361_Reference_Manual_UG-570.pdf Figure 4. PLL Synthesizer Block Diagram
  //(calculating the "TO VCO DIVIDER BLOCK" signal)
  // this calculation is similar to what's done in Analog Device's No-OS's
  // ad9361.c's ad9361_calc_rfpll_int_freq() function, except WE use floating
  // point and don't round
  x *= (d_Rx_RFPLL_N_Integer + (d_Rx_RFPLL_N_Fractional/RFPLL_MODULUS));

  //AD9361_Reference_Manual_UG-570.pdf Figure 4. VCO Divider
  //(calculating the "LO" signal which is the  output of the MUX)
  x /= d_Rx_RFPLL_VCO_Divider;

  val = x;

  return 0;
}

#endif // _CALC_AD9361_RX_RFPLL_H
