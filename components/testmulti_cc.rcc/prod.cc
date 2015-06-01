/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu May 28 20:30:32 2015 EDT
 * BASED ON THE FILE: prod.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the prod worker in C++
 */

#include "prod-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace ProdWorkerTypes;

class ProdWorker : public ProdWorkerBase {
  RCCResult run(bool /*timedout*/) {
    // Test worker
    return RCC_ADVANCE;
  }
};

PROD_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PROD_END_INFO
