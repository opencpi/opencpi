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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Aug 31 13:40:06 2018 EDT
 * BASED ON THE FILE: copy_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the copy_cc worker in C++
 */

#include "copy_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Copy_ccWorkerTypes;

class Copy_ccWorker : public Copy_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    if (in.length() == 0)
      return RCC_ADVANCE_DONE;
    strcpy(out.send().mesg(), in.send().mesg());
    out.setLength(in.length());
    return RCC_ADVANCE; // change this as needed for this worker to do something useful
    // return RCC_ADVANCE; when all inputs/outputs should be advanced each time "run" is called.
    // return RCC_ADVANCE_DONE; when all inputs/outputs should be advanced, and there is nothing more to do.
    // return RCC_DONE; when there is nothing more to do, and inputs/outputs do not need to be advanced.
  }
};

COPY_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
COPY_CC_END_INFO
