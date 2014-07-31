/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Jul 30 09:38:27 2014 EDT
 * BASED ON THE FILE: cilkmatrixsink_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the cilkmatrixsink_cc worker in C++
 */

#include "cilkmatrixsink_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class Cilkmatrixsink_ccWorker : public Cilkmatrixsink_ccWorkerBase {


  RCCResult run(bool /*timedout*/) {
    if (in.length() == 0 ) {
      return RCC_DONE;
    } 
    return RCC_ADVANCE;
  }

};

CILKMATRIXSINK_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
CILKMATRIXSINK_CC_END_INFO
