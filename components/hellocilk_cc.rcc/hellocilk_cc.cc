/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Aug 16 08:17:31 2014 EDT
 * BASED ON THE FILE: hellocilk_cc.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the hellocilk_cc worker in Cilk
 */

#include "hellocilk_cc-worker.hh"
#include <stdio.h>

using namespace OCPI::RCC; // for easy access to RCC data types and constants

class Hellocilk_ccWorker : public Hellocilk_ccWorkerBase {
  RCCResult run(bool /*timedout*/) {
    cilk_for (int n=0; n<200; n++) {
      for ( int h=0; h<2; h++ ){
	printf("Hello world from cilk worker number %d, interation %d\n", getCilkWorkerNumber(), n );
      }
    }
    return RCC_ADVANCE_DONE;
  }
};

HELLOCILK_CC_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
HELLOCILK_CC_END_INFO
