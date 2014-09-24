/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Sep 24 16:24:22 2014 EDT
 * BASED ON THE FILE: dsample_complex.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the dsample_complex worker in C
 */

#include "dsample_complex_Worker.h"

DSAMPLE_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch dsample_complex = {
 /* insert any custom initializations here */
 DSAMPLE_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker dsample_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 return RCC_ADVANCE;
}
