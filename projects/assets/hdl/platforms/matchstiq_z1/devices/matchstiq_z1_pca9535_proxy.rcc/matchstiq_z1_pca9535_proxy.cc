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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jul 24 15:33:06 2015 EDT
 * BASED ON THE FILE: matchstiq_z1_pca9535_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the matchstiq_z1_pca9535_proxy worker in C++
 */

#include "matchstiq_z1_pca9535_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Matchstiq_z1_pca9535_proxyWorkerTypes;

class Matchstiq_z1_pca9535_proxyWorker : public Matchstiq_z1_pca9535_proxyWorkerBase {
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }

  RCCResult frontLNA_enable_written(){
    slave.set_config_port0(m_properties.frontLNA_enable ? 1:2);
    return RCC_OK;
  }

  RCCResult lime_rx_input_written(){
    if(m_properties.lime_rx_input != 2 && m_properties.lime_rx_input != 3)
      return setError("Invalid Lime RX input.\n Valid Inputs are 2 and 3");
    //These values were determined by monitoring I2C transactions on the Matchstiq-Z1
    m_properties.lime_rx_input == 2 ? slave.set_output_port1(0x06) : slave.set_output_port1(0x07);
    return RCC_OK;
  }

  RCCResult filter_bandwidth_written(){
    //These values were determined by monitoring I2C transactions on the Matchstiq-Z1
    switch(m_properties.filter_bandwidth){
    case 0://unfiltered
      slave.set_output_port0(0x80);
      break;
    case 1://300  to 700  MHz
      slave.set_output_port0(0x10);
      break;
    case 2://625  to 1080 MHz
      slave.set_output_port0(0x38);
      break;
    case 3://1000 to 2100 MHz
      slave.set_output_port0(0x4C);
      break;
    case 4://1700 to 2500 MHz
      slave.set_output_port0(0x68);
      break;
    case 5://2200 to 3800 MHz
      slave.set_output_port0(0xC4);
      break;
    default:
      return setError("Invalid Preselection Filter Bandwidth.\n Valid bandwidths are:\n"
		      "0 = unfiltered\n"
		      "1 = 300  to 700  MHz\n"
		      "2 = 625  to 1080 MHz\n"
		      "3 = 1000 to 2100 MHz\n"
		      "4 = 1700 to 2500 MHz\n"
		      "5 = 2200 to 3800 MHz\n");}
    return RCC_OK;
  }

};

MATCHSTIQ_Z1_PCA9535_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
MATCHSTIQ_Z1_PCA9535_PROXY_END_INFO
