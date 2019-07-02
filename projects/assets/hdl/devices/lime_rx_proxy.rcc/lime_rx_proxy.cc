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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun  2 14:28:27 2015 EDT
 * BASED ON THE FILE: lime_rx_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the lime_rx_proxy worker in C++
 */

#include <assert.h>
#include "lime_shared.h"
#include "lime_rx_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Lime_rx_proxyWorkerTypes;
using namespace OCPI::Lime;

class Lime_rx_proxyWorker : public Lime_rx_proxyWorkerBase {
  RunCondition m_aRunCondition;
public:
  Lime_rx_proxyWorker() : m_aRunCondition(RCC_NO_PORTS) {
    //Run function should never be called
    setRunCondition(&m_aRunCondition);
  }
private:
  // notification that input_select property has been written
  RCCResult input_select_written() {
    if (m_properties.input_select > 3)
      return setError("Input selection \"%u\" out of range 1-3 (0 is disable all)",
		      m_properties.input_select);
    slave.set_rx_freqsel((slave.get_rx_freqsel()&0xFC) | m_properties.input_select);
    slave.set_rxfe_cbe_lna((slave.get_rxfe_cbe_lna()&0xCF) | (m_properties.input_select << 4));
    return RCC_OK;
  }
  // notification that input_gain_db property has been written
  RCCResult input_gain_db_written() {
    uint8_t gain;
    uint16_t old_value = slave.get_rxfe_cbe_lna() & 0x3f;
    switch (m_properties.input_gain_db) {
    case -6:
      //printf ("got a -6 command old value is: %x \n", old_value);
      if ( (old_value & 0x30) == 0x30)
      {
        gain = 2;
        //printf("true \n");
      }
      else
      {
        gain = 1;
        //printf("false %x \n", old_value & 0x30);
      }
      break;
    case 0:
      gain = 2;
      break;
    case +6:
        gain = 3;
      break;
    default:
      return setError("Input_gain_db (\"%d\") can only be -6, 0, or +6",
		      m_properties.input_gain_db);
    }
    //printf ("set gain to: %x \n", gain);
    slave.set_rxfe_cbe_lna(old_value | (gain << 6));
    return RCC_OK;
  }

  static uint8_t readVtune(void *arg) {
    return (*(Lime_rx_proxyWorker*)arg).slave.get_rx_vtune();
  }
  static void writeVcoCap(void *arg, uint8_t val) {
    (*(Lime_rx_proxyWorker*)arg).slave.set_rx_vcocap(((*(Lime_rx_proxyWorker*)arg).slave.get_rx_vcocap()&0xC0)|val);
  }
  // notification that center_freq_hz property has been written
  RCCResult center_freq_hz_written() {
    uint8_t regval;
    const char *err = getFreqValue(m_properties.center_freq_hz, regval);
    if (err)
      return setError("Error in center frequency (%g): %s", m_properties.center_freq_hz);
    Divider div;
    calcDividers(30.72e6, m_properties.center_freq_hz, div);

    slave.set_rx_freqsel((regval << 2) | m_properties.input_select);
    slave.set_rx_nint(div.nint);
    slave.set_rx_nfrac_hi(div.nfrac_hi);
    slave.set_rx_nfrac_mid(div.nfrac_mid);
    slave.set_rx_nfrac_lo(div.nfrac_lo);
    // Set VCO Cap - performing calibration
    if ((err = setVcoCap(readVtune, writeVcoCap, this)))
      return setError("Error setting VcoCap value: %s", err);
    return RCC_OK;
  }
  // notification that pre_lpf_gain_db property has been written
  RCCResult pre_lpf_gain_db_written() {
    //Set rx_vga1gain register
    uint8_t regval;
    // This should probably be a table or calculation and allow the
    // full range of 7 bit values per the data sheet.  But it is not log-linear,
    // so may need some sort of table
    switch (m_properties.pre_lpf_gain_db) {
    case 5:
      regval = 2;
      break;
    case 19:
      regval = 102;
      break;
    case 30:
      regval = 120;
      break;
    default:
      return setError("Invalid pre_lpf gain. Valid values are 5dB, 19dB, or 30dB.");
    }
    slave.set_rxfe_rfb_tia((slave.get_rxfe_rfb_tia() & 0x80) | regval);
    // do we need to set the capacitor value too???
    return RCC_OK;
  }

  // notification that lpf_bw_hz property has been written
  RCCResult lpf_bw_hz_written() {
    uint8_t val;
    const char *err = getLpfBwValue(m_properties.lpf_bw_hz, val);
    if (err)
      return setError("Error setting LPF bandwidth of (%g Hz): %s", m_properties.lpf_bw_hz, err);
    slave.set_rx_bwc_lpf((slave.get_rx_bwc_lpf()&0xC3)|(val<<2));
    slave.set_top_ctl2((slave.get_top_ctl2()&0xF0)|(val));
    return RCC_OK;
  }
  // notification that post_lpf_gain_db property has been written
  RCCResult post_lpf_gain_db_written() {
    if(m_properties.post_lpf_gain_db % 3 || m_properties.post_lpf_gain_db > 30)
      return setError("Invalid gain: %u. Valid values are 0-30 dB in increments of 3.",
		      m_properties.post_lpf_gain_db);
    //Set rx_vga2gain register
    slave.set_rx_vga2gain((slave.get_rx_vga2gain() & 0xE0) |
			  (m_properties.post_lpf_gain_db / 3));
    return RCC_OK;
  }
  // notification that post_mixer_dc_offset_i property has been written
  RCCResult post_mixer_dc_offset_i_written() {
    if (m_properties.post_mixer_dc_offset_i > 0x7F || m_properties.post_mixer_dc_offset_i > 0x7F)
      setError("Invalid post mixer DC offset (I) setting (%u). "
	       "Valid values are between 0x00 and 0x7F",
	       m_properties.post_mixer_dc_offset_i);
    slave.set_rxfe_dcoff_i(m_properties.post_mixer_dc_offset_i | (1 << 7));
    return RCC_OK;
  }
  // notification that post_mixer_dc_offset_i property has been written
  RCCResult post_mixer_dc_offset_q_written() {
    if (m_properties.post_mixer_dc_offset_q > 0x7F || m_properties.post_mixer_dc_offset_q > 0x7F)
      setError("Invalid post mixer DC offset setting (%u). "
	       "Valid values are between 0x00 and 0x7F",
	       m_properties.post_mixer_dc_offset_q);
    slave.set_rxfe_dcoff_q(m_properties.post_mixer_dc_offset_q | (1 << 7));
    return RCC_OK;
  }
  // enable required for both initialize and start
  RCCResult enable() {
    slave.set_top_ctl0(slave.get_top_ctl0() | (1 << 2));
    slave.set_clk_ctl(slave.get_clk_ctl() | (1 << 2));
    return RCC_OK;
  }
  RCCResult initialize() {
    enable();
    return RCC_OK;
  }
  RCCResult start() {
    if(isSuspended()) //Prevents duplication of initialize
      enable();
    return RCC_OK;
  }
  RCCResult stop() {
    slave.set_top_ctl0(slave.get_top_ctl0() & (~(1 << 2)));
    slave.set_clk_ctl(slave.get_clk_ctl() & (0xFB));
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE;
  }
};

LIME_RX_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
LIME_RX_PROXY_END_INFO
