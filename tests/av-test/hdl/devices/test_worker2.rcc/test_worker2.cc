/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Dec 17 17:09:53 2018 EST
 * BASED ON THE FILE: test_worker2.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the test_worker2 worker in C++
 */

#include "test_worker2-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Test_worker2WorkerTypes;

class Test_worker2Worker : public Test_worker2WorkerBase {
  RCCResult run(bool /*timedout*/) {
    return RCC_DONE; // change this as needed for this worker to do something useful
    // return RCC_ADVANCE; when all inputs/outputs should be advanced each time "run" is called.
    // return RCC_ADVANCE_DONE; when all inputs/outputs should be advanced, and there is nothing more to do.
    // return RCC_DONE; when there is nothing more to do, and inputs/outputs do not need to be advanced.
  }
};

TEST_WORKER2_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
TEST_WORKER2_END_INFO
