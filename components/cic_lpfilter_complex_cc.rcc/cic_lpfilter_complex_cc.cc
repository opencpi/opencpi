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
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Jul 15 10:43:11 2015 EDT
 * BASED ON THE FILE: cic_lpfilter_complex_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the cic_lpfilter_complex_cc worker in C++
 */

#include "cic_lpfilter_complex_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Cic_lpfilter_complex_ccWorkerTypes;

class Cic_lpfilter_complex_ccWorker : public Cic_lpfilter_complex_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    switch (in.opCode()) {
    case Iqstream_with_syncIq_OPERATION:
      {
	const Iqstream_with_syncIqData *inIq = in.iq().data().data();
	size_t inLength = in.iq().data().size();
	Iqstream_with_syncIqData *outIq = out.iq().data().data();
	size_t outLength = out.iq().data().capacity();
	if (outLength > inLength)
	  return setError("Output buffer size (%zu) smaller than input (%zu)");
	out.iq().data().resize(inLength);
	out.setOpCode(Iqstream_with_syncIq_OPERATION);
	printf("IQ Op: %d\n", inIq->I + inIq->Q);
	break;
      }
    case Iqstream_with_syncTime_OPERATION:
      {
	const uint64_t &now = in.Time().time();
	printf("Time Op: %llu\n", (unsigned long long)now);
	break;
      }
    case Iqstream_with_syncSync_OPERATION:
      printf("Sync Op:\n");
      break;
    }
    return RCC_ADVANCE;
  }
};

CIC_LPFILTER_COMPLEX_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
CIC_LPFILTER_COMPLEX_CC_END_INFO
