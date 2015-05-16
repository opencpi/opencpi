/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri May 15 17:12:14 2015 EDT
 * BASED ON THE FILE: prod.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the prod worker in C++
 */

#include "prod-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class ProdWorker : public ProdWorkerBase {
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
};

PROD_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PROD_END_INFO
