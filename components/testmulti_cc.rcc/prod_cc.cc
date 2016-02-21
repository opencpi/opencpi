/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu May 28 20:30:32 2015 EDT
 * BASED ON THE FILE: prod.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the prod worker in C++
 */

#include "prod_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Prod_ccWorkerTypes;

class Prod_ccWorker : public Prod_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    // Test worker
    return RCC_ADVANCE;
  }
};

PROD_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PROD_CC_END_INFO
