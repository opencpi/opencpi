/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Aug 28 21:01:13 2012 EDT
 * BASED ON THE FILE: ptest.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: ptest
 */
#include "ptest_Worker.h"

PTEST_METHOD_DECLARATIONS;
RCCDispatch ptest = {
 /* insert any custom initializations here */
 PTEST_DISPATCH
};

/*
 * Methods to implement for worker ptest, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)timedOut;(void)newRunCondition;
 PtestProperties *p = self->properties;
 if (p->error)
   self->container.setError("This is a test error: %d", 1234);
 return RCC_DONE;
}
