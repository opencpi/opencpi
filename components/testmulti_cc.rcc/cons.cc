/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri May 15 17:12:14 2015 EDT
 * BASED ON THE FILE: cons.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the cons worker in C++
 */

#include "cons-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class ConsWorker : public ConsWorkerBase {
  RCCResult run(bool /*timedout*/) {
    return RCC_ADVANCE;
  }
};

CONS_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
CONS_END_INFO
