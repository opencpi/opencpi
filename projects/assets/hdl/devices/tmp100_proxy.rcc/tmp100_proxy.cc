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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jul 24 15:49:16 2015 EDT
 * BASED ON THE FILE: tmp100_proxy.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the tmp100_proxy worker in C++
 */

#include "tmp100_proxy-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Tmp100_proxyWorkerTypes;

class Tmp100_proxyWorker : public Tmp100_proxyWorkerBase {

  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
  RCCResult temperature_read()
  {
    unsigned short regValue = slave.get_temp_reg();
    //printf("value read from I2c is %x\n",regValue);
    unsigned short swapped = (regValue>>8) | (regValue<<8);
    m_properties.temperature = 128.0 / 32768.0 * swapped;
    return RCC_OK;
  }

};

TMP100_PROXY_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
TMP100_PROXY_END_INFO
