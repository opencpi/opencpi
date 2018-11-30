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

#ifndef _READERS_AD9361_RX_RFPLL_H
#define _READERS_AD9361_RX_RFPLL_H

/*! @file readers_ad9361_rf_rx_pll.h
 *  @brief Provides functions for reading Receiver (RX) Radio
 *         Frequency (RF) Phase-Locked Loop (RFPLL) values from an AD9361 IC.
 *         See AD9361_Reference_Manual_UG-570.pdf
 *         Figure 4. PLL Synthesizer Block Diagram
 ******************************************************************************/

#include "calc_ad9361_rf_rx_pll.h"

/*! @brief Retrieve with double floating point precision
 *         the theoretical value of the
 *             AD9361 RX RF LO frequency in Hz
 *         from an AD9361 IC.
 *
 *  @param[out] val                 Retrieved value.
 *  @param[in]  AD9361_interfacer   Template object which has the following
 *                                  methods for AD9361 register access:
 *                                    get_general_rfpll_dividers(),
 *                                    get_ref_divide_config_1(),
 *                                    get_ref_divide_config_2(),
 *                                    get_rx_synth_integer_byte_0(),
 *                                    get_rx_synth_integer_byte_1(),
 *                                    get_rx_synth_fract_byte_0(),
 *                                    get_rx_synth_fract_byte_1(),
 *                                    get_rx_synth_fract_byte_2().
 *  @param[in]  AD9361_reference_clock_rate_Hz Frequency of the device connected
 *                                             to the AD9361 XTAL pin(s).
 *  @return 0 if there are no errors, non-zero char array pointer if there
 *          are errors (char array content will describe the error).
 ******************************************************************************/
template<typename T>
const char* get_AD9361_Rx_RFPLL_LO_freq_Hz(
    double& val,
    T& AD9361_interfacer,
    double AD9361_reference_clock_rate_Hz) {

  regs_calc_AD9361_Rx_RFPLL_LO_freq_Hz_t regs;
  T& ifc = AD9361_interfacer;

  regs.general_rfpll_dividers  = ifc.get_general_rfpll_dividers();
  regs.ref_divide_config_1     = ifc.get_ref_divide_config_1();
  regs.ref_divide_config_2     = ifc.get_ref_divide_config_2();
  regs.rx_synth_integer_byte_0 = ifc.get_rx_synth_integer_byte_0();
  regs.rx_synth_integer_byte_1 = ifc.get_rx_synth_integer_byte_1();
  regs.rx_synth_fract_byte_0   = ifc.get_rx_synth_fract_byte_0();
  regs.rx_synth_fract_byte_1   = ifc.get_rx_synth_fract_byte_1();
  regs.rx_synth_fract_byte_2   = ifc.get_rx_synth_fract_byte_2();

  return calc_AD9361_Rx_RFPLL_LO_freq_Hz(
    val, AD9361_reference_clock_rate_Hz, regs);
}

#endif // _READERS_AD9361_RX_RFPLL_H
