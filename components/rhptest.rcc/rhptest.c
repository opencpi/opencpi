/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Aug 28 21:01:13 2012 EDT
 * BASED ON THE FILE: ptest.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: ptest
 */
#include "rhptest_Worker.h"

RHPTEST_METHOD_DECLARATIONS;
RCCDispatch rhptest = {
 /* insert any custom initializations here */
 RHPTEST_DISPATCH
};

/*
 * Methods to implement for worker ptest, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)timedOut;(void)newRunCondition;
 RhptestProperties *p = self->properties;
 if (p->error)
   self->container.setError("This is a test error: %d", 1234);
 return RCC_DONE;
}
