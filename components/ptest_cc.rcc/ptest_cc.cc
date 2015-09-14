/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Sep  8 16:59:19 2015 EDT
 * BASED ON THE FILE: ptest_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the ptest_cc worker in C++
 */

#include "ptest_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Ptest_ccWorkerTypes;

class Ptest_ccWorker : public Ptest_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
};

PTEST_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
PTEST_CC_END_INFO
