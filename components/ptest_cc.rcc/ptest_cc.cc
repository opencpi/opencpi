/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Nov 13 17:35:41 2014 EST
 * BASED ON THE FILE: ptest_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the ptest worker in C++
 */

#include "ptest-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class PtestWorker : public PtestWorkerBase {
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
};

PTEST_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PTEST_END_INFO
