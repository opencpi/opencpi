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
    fprintf(stderr, "NOTIFIED: %s\n", isInitialized() ? "init" : "none");
    return RCC_OK;
  } // notification that testws property has been written
  RCCResult run(bool /*timedout*/) {
    size_t length = in.data().data().size();
    const uint32_t *inData  = in.data().data().data();
    uint32_t *outData = out.data().data().data();

    out.setOpCode(in.opCode());        // Set the metadata for the output message
    out.data().data().resize(length);  // resize output array
    for (unsigned n = length; n; n--) // n is length in sequence elements of input
      *outData++ = *inData++ + properties().biasValue;
    return in.length() ? RCC_ADVANCE : RCC_ADVANCE_DONE;
  }
};

BIAS_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
BIAS_CC_END_INFO
