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
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun  5 21:24:44 2014 EDT
 * BASED ON THE FILE: bias_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the bias_cc worker in C++
 */

#include "bias_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Bias_ccWorkerTypes;

class Bias_ccWorker : public Bias_ccWorkerBase {
  RCCResult testws_read() {return RCC_OK;} // notification that testws property will be read
  RCCResult testws_written() {
    fprintf(stderr, "NOTIFIED: %s: %u\n", isInitialized() ? "init" : "none", properties().testws);
    return RCC_OK;
  } // notification that testws property has been written
  RCCResult run(bool /*timedout*/) {
    out.setOpCode(in.opCode());        // Set the metadata for the output message
    // Allow ZLMs to pass through unmolested.
    if (!in.length()) {
      out.setLength(0);
      return RCC_ADVANCE;
    }
    size_t length = in.data().data().size();
    const uint32_t *inData  = in.data().data().data();
    uint32_t *outData = out.data().data().data();

    out.data().data().resize(length);  // resize output array
    for (unsigned n = length; n; n--) // n is length in sequence elements of input
      *outData++ = *inData++ + properties().biasValue;
    return RCC_ADVANCE;
  }
};

BIAS_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
BIAS_CC_END_INFO
