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
 *  @brief Provides functions for reading Receiver (RX) BaseBand
 *         (BB) Analog-to-Digital Converter (ADC) values from an AD9361 IC.
 ******************************************************************************/

#include "calc_ad9361_bb_rx_adc.h"

/*! @brief Retrieve with double floating point precision
 *         the theoretical value of the
 *             AD9361 RX SAMPL FREQ in Hz
 *         from an AD9361 IC.
 *
 *  @param[out] val                 Retrieved value.
 *  @param[in]  AD9361_interfacer   Template object which has the following
 *                                  methods for AD9361 register access:
 *  @param[in]  AD9361_XTAL_freq_Hz Frequency of the device connected to the
 *                                  AD9361 XTAL pin(s).
 *
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
template<typename T>
const char* get_AD9361_RX_SAMPL_FREQ_Hz(
    double& val,
    T& AD9361_interfacer,
    double AD9361_reference_clock_rate_Hz) {

  regs_calc_AD9361_CLKRF_FREQ_Hz_t regs;
  T& ifc = AD9361_interfacer;

  regs.bbpll_ref_clock_scaler        = ifc.get_bbpll_ref_clock_scaler();
  regs.bbpll_fract_bb_freq_word_1    = ifc.get_bbpll_fract_bb_freq_word_1();
  regs.bbpll_fract_bb_freq_word_2    = ifc.get_bbpll_fract_bb_freq_word_2();
  regs.bbpll_fract_bb_freq_word_3    = ifc.get_bbpll_fract_bb_freq_word_3();
  regs.bbpll_integer_bb_freq_word    = ifc.get_bbpll_integer_bb_freq_word();
  regs.clock_bbpll                   = ifc.get_clock_bbpll();
  regs.general_rx_enable_filter_ctrl = ifc.get_general_rx_enable_filter_ctrl();

  calc_AD9361_RX_SAMPL_FREQ_Hz_t calc_obj;
  calc_obj.BBPLL_input_F_REF = AD9361_reference_clock_rate_Hz;
  const char* ret = regs_calc_AD9361_RX_SAMPL_FREQ_Hz(calc_obj, regs);
  val = calc_obj.RX_SAMPL_FREQ_Hz;

  return ret;
}

#endif // _READERS_AD9361_RX_ADC_H
