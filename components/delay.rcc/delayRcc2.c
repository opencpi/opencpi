/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun 29 17:34:09 2010 EDT
 * BASED ON THE FILE: delayRcc2.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: delayRcc2
 */
#include "delayRcc2_Worker.h"

DELAYRCC2_METHOD_DECLARATIONS;
RCCDispatch delayRcc2 = {
  /* insert any custom initializations here */
  DELAYRCC2_DISPATCH
};

/*
 * Methods to implement for worker delayRcc2, based on metadata.
*/

static RCCResult initialize(RCCWorker *self) {
  return RCC_OK;
}

static RCCResult start(RCCWorker *self) {
  return RCC_OK;
}

static RCCResult run(RCCWorker *self,
                     RCCBoolean timedOut,
                     RCCBoolean *newRunCondition) {
  return RCC_ADVANCE;
}
