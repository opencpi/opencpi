/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Jun 26 21:23:12 2016 EDT
 * BASED ON THE FILE: ad9361_tx_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the ad9361_tx_proxy worker in C++
 */

#include "ad9361_tx_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Ad9361_tx_proxyWorkerTypes;

class Ad9361_tx_proxyWorker : public Ad9361_tx_proxyWorkerBase {
  RCCResult initialize() {
    return RCC_OK;
  }
  RCCResult stop() {
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
  // notification that pre_mixer_dc_offset_i property has been written
  RCCResult pre_mixer_dc_offset_i_written() {
    return RCC_OK;
  }
  // notification that pre_mixer_dc_offset_q property has been written
  RCCResult pre_mixer_dc_offset_q_written() {
    return RCC_OK;
  }
  // notification that center_freq_hz property has been written
  RCCResult center_freq_hz_written() {
    return RCC_OK;
  }
  // notification that output_gain_db property has been written
  RCCResult output_gain_db_written() {
    return RCC_OK;
  }
  // notification that output_select property has been written
  RCCResult output_select_written() {
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
};

AD9361_TX_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
AD9361_TX_PROXY_END_INFO
