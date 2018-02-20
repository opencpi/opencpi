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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jul 24 15:15:52 2015 EDT
 * BASED ON THE FILE: matchstiq_z1_avr_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the matchstiq_z1_avr_proxy worker in C++
 */

#include "matchstiq_z1_avr_proxy-worker.hh"
#include <math.h>
#include <unistd.h>

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Matchstiq_z1_avr_proxyWorkerTypes;

class Matchstiq_z1_avr_proxyWorker : public Matchstiq_z1_avr_proxyWorkerBase {
 
  RCCResult start() {
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }

  RCCResult serial_num_read() {
    usleep(2500);
    m_properties.serial_num = slave.get_MS_SERIAL_NUM();
    usleep(2500);
    return RCC_OK;
  }

  RCCResult led_written() {
    usleep(2500);
    switch (m_properties.led) {
    case LED_OFF: slave.set_DUAL_LED_REG(0); break;
    case LED_GREEN: slave.set_DUAL_LED_REG(1); break;
    case LED_RED: slave.set_DUAL_LED_REG(2); break;
    case LED_ORANGE: slave.set_DUAL_LED_REG(3); break;
    default:;}
    return RCC_OK;
  }

  RCCResult warp_voltage_written(){
    usleep(2500);
    if(m_properties.warp_voltage < 683 || m_properties.warp_voltage > 3413)
	  return setError("Invalid WARP voltage value.\nValid WARP voltage values are between 683 and 3413");
    slave.set_TCVCXO_WARP_REG(m_properties.warp_voltage);
    return RCC_OK;
  }

  RCCResult attenuation_written(){
    usleep(2500);
    double fractpart, intpart;
    fractpart = modf ((double)m_properties.attenuation, &intpart);
    if(m_properties.attenuation < 0 || m_properties.attenuation > 31.5 || (fractpart != 0 && fractpart !=0.5))
	  return setError("Invalid Attenuator value.\nValid attentuator values are 0 to 31.5 in 0.5 dB steps");
    uint16_t attenuationTimesTwo;
    attenuationTimesTwo = (uint16_t)floor(m_properties.attenuation*2);
    slave.set_RF_STEP_ATTEN_REG(attenuationTimesTwo);
    return RCC_OK;
  }

};

MATCHSTIQ_Z1_AVR_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
MATCHSTIQ_Z1_AVR_PROXY_END_INFO
