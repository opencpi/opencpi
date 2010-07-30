/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jul  2 16:01:20 2010 EDT
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
  // Just put some hand-authored text here to make this file not replaced all the time
  return RCC_ADVANCE;
}
