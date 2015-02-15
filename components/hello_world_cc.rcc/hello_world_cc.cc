/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Feb 14 17:53:11 2015 EST
 * BASED ON THE FILE: hello_world_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the hello_world_cc worker in C++
 */

#include "hello_world_cc-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class Hello_world_ccWorker : public Hello_world_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    printf("Hello World!\n");
    return RCC_DONE;
  }
};

HELLO_WORLD_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
HELLO_WORLD_CC_END_INFO
