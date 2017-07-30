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
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun  2 08:16:35 2016 EDT
 * BASED ON THE FILE: ad9361_rx_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the ad9361_rx_proxy worker in C++
 */

extern "C" {
#include "ad9361_api.h"
}
#include "ad9361_rx_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Ad9361_rx_proxyWorkerTypes;

class Ad9361_rx_proxyWorker : public Ad9361_rx_proxyWorkerBase {
  struct ad9361_rf_phy *m_ad9361_phy;
  RCCResult initialize() {
    AD9361_InitParam p;
    ad9361_init(&m_ad9361_phy, &p);
    return RCC_OK;
  }
  RCCResult stop() {
    return RCC_OK;
  }
  // notification that input_select property has been written
  RCCResult input_select_written() {
    return RCC_OK;
  }
  // notification that input_gain_db property has been written
  RCCResult input_gain_db_written() {
    return RCC_OK;
  }
  // notification that center_freq_hz property has been written
  RCCResult center_freq_hz_written() {
    return RCC_OK;
  }
  // notification that post_mixer_dc_offset_i property has been written
  RCCResult post_mixer_dc_offset_i_written() {
    return RCC_OK;
  }
  // notification that post_mixer_dc_offset_q property has been written
  RCCResult post_mixer_dc_offset_q_written() {
    return RCC_OK;
  }
  // notification that pre_lpf_gain_db property has been written
  RCCResult pre_lpf_gain_db_written() {
    return RCC_OK;
  }
  // notification that lpf_bw_hz property has been written
  RCCResult lpf_bw_hz_written() {
    return RCC_OK;
  }
  // notification that post_lpf_gain_db property has been written
  RCCResult post_lpf_gain_db_written() {
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
};

AD9361_RX_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
AD9361_RX_PROXY_END_INFO
