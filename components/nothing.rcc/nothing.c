/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Feb  8 14:22:19 2016 EST
 * BASED ON THE FILE: nothing.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the nothing worker in C
 */

#include "nothing_Worker.h"

NOTHING_METHOD_DECLARATIONS;
RCCDispatch nothing = {
 /* insert any custom initializations here */
 NOTHING_DISPATCH
};

/*
 * Methods to implement for worker nothing, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 return RCC_DONE;
}
