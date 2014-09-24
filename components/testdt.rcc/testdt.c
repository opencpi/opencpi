/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Nov  5 16:48:46 2011 EDT
 * BASED ON THE FILE: testdt.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: testdt
 */
#include "testdt_Worker.h"

TESTDT_METHOD_DECLARATIONS;
RCCDispatch testdt = {
  /* insert any custom initializations here */
  TESTDT_DISPATCH
};

/*
 * Methods to implement for worker testdt, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)self;(void)timedOut;(void)newRunCondition;
  return RCC_ADVANCE;
}
