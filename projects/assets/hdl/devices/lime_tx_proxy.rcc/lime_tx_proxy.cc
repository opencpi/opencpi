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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun  2 17:40:06 2015 EDT
 * BASED ON THE FILE: lime_tx_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the lime_tx_proxy worker in C++
 */

#include "lime_shared.h"
#include "lime_tx_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Lime_tx_proxyWorkerTypes;
using namespace OCPI::Lime;

class Lime_tx_proxyWorker : public Lime_tx_proxyWorkerBase {
  RunCondition m_aRunCondition;
public:
  Lime_tx_proxyWorker() : m_aRunCondition(RCC_NO_PORTS) {
    //Run function should never be called
    setRunCondition(&m_aRunCondition);
  }
private:
  // notification that lpf_bw_hz property has been written
  RCCResult lpf_bw_hz_written() {
    uint8_t val;
    const char *err = getLpfBwValue(m_properties.lpf_bw_hz, val);
    if (err)
      return setError("Error setting LPF bandwidth of (%g Hz): %s", m_properties.lpf_bw_hz, err);
    slave.set_tx_bwc_lpf((slave.get_tx_bwc_lpf()&0xC3)|(val << 2));
    return RCC_OK;
  }
  // notification that post_lpf_gain_db property has been written
  RCCResult post_lpf_gain_db_written() {
    if (m_properties.post_lpf_gain_db > -4 or m_properties.post_lpf_gain_db < -35)
      return setError("Invalid post LPF gain value(%d). Valid values are -4 to -35.",
		      m_properties.post_lpf_gain_db);
    slave.set_tx_vga1gain((slave.get_tx_vga1gain()&0xE0) |
			  (m_properties.post_lpf_gain_db+35));
    return RCC_OK;
  }
  // notification that pre_mixer_dc_offset_i property has been written
  RCCResult pre_mixer_dc_offset_i_written() {
    slave.set_tx_vga1dc_i(m_properties.pre_mixer_dc_offset_i);
    return RCC_OK;
  }
  // notification that pre_mixer_dc_offset_q property has been written
  RCCResult pre_mixer_dc_offset_q_written() {
    slave.set_tx_vga1dc_q(m_properties.pre_mixer_dc_offset_q);
    return RCC_OK;
  }
  static uint8_t readVtune(void *arg) {
    return (*(Lime_tx_proxyWorker*)arg).slave.get_tx_vtune();
  }
  static void writeVcoCap(void *arg, uint8_t val) {
    (*(Lime_tx_proxyWorker*)arg).slave.set_tx_vcocap(((*(Lime_tx_proxyWorker*)arg).slave.get_tx_vcocap()&0xC0)|val);
  }
  // notification that center_freq_hz property has been written
  RCCResult center_freq_hz_written() {
    uint8_t freqsel;
    const char *err = getFreqValue(m_properties.center_freq_hz, freqsel);
    if (err)
      return setError("Error in center frequency (%g): %s", m_properties.center_freq_hz, err);
    Divider div;
    calcDividers(30.72e6, m_properties.center_freq_hz, div);
    slave.set_tx_freqsel((freqsel << 2) | m_properties.output_select);
    slave.set_tx_nint(div.nint);
    slave.set_tx_nfrac_hi(div.nfrac_hi);
    slave.set_tx_nfrac_mid(div.nfrac_mid);
    slave.set_tx_nfrac_lo(div.nfrac_lo);
    // Set VCO Cap - performing calibration
    if ((err = setVcoCap(readVtune, writeVcoCap, this)))
      return setError("Error setting VcoCap value: %s", err);
    return RCC_OK;
  }
  // notification that output_gain_db property has been written
  RCCResult output_gain_db_written() {
    if (m_properties.output_gain_db > 25 || m_properties.output_gain_db < 0)
      return setError("Invalid output gain (%u). Valid values are 0dB to 25dB.\n");
    slave.set_tx_vga2gain((slave.get_tx_vga2gain()&0x7) | (m_properties.output_gain_db << 3));
    return RCC_OK;
  }
  // notification that output_select property has been written
  RCCResult output_select_written() {
    if (m_properties.output_select > 2)
      return setError("Output selection \"%u\" out of range 1-2 (0 is disable all)",
		      m_properties.output_select);
    slave.set_tx_pa_en((slave.get_tx_pa_en() & 0xE7) | (m_properties.output_select << 3));
    return RCC_OK;
  }
  // enable required for both initialize and start
  RCCResult enable() {
    slave.set_top_ctl0(slave.get_top_ctl0() | (1 << 3));
    slave.set_clk_ctl(slave.get_clk_ctl() | (1));
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
    slave.set_top_ctl0(slave.get_top_ctl0() & (~(1 << 3)));
    slave.set_clk_ctl(slave.get_clk_ctl() & (0xFE));
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE;
  }
};

LIME_TX_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
LIME_TX_PROXY_END_INFO
