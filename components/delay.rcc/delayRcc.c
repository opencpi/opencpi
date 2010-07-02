/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jun 29 17:34:08 2010 EDT
 * BASED ON THE FILE: delayRcc.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: delayRcc
 */
#include "delayRcc_Worker.h"

DELAYRCC_METHOD_DECLARATIONS;
RCCDispatch delayRcc = {
  /* insert any custom initializations here */
  DELAYRCC_DISPATCH
};

/*
 * Methods to implement for worker delayRcc, based on metadata.
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
