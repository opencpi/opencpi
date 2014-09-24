/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Jun  5 21:24:44 2014 EDT
 * BASED ON THE FILE: bias_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the bias_cc worker in C++
 */

#include "OcpiApi.hh"
#include "bias_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class Bias_ccWorker : public Bias_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    const uint32_t *inData  = in.data().data;   // data arg of data message at "in" port
    uint32_t *outData = out.data().data;  // same at "out" port

    static int x = 0;
    if (!x) {
      OCPI::API::Application &app = getApplication(); // test this method
      std::string name, value;
      fprintf(stderr, "Dump of all initial property values:\n");
      for (unsigned n = 0; app.getProperty(n, name, value); n++)
	fprintf(stderr, "Property %2u: %s = \"%s\"\n", n, name.c_str(), value.c_str());
      x = 1;
    }
    out.checkLength(in.length());               // make sure input will fit in output buffer
    for (unsigned n = in.data_length(); n; n--) // n is length in sequence elements of input
      *outData++ = *inData++ + properties().biasValue;
    out.setInfo(in.opCode(), in.length());      // Set the metadata for the output message
    return in.length() ? RCC_ADVANCE : RCC_ADVANCE_DONE;
  }
};

BIAS_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
BIAS_CC_END_INFO
