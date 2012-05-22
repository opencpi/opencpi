/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 22 10:04:14 2012 EDT
 * BASED ON THE FILE: cic_lpfilter_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: cic_lpfilter_complex
 */
#include "cic_lpfilter_complex_Worker.h"

CIC_LPFILTER_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch cic_lpfilter_complex = {
 /* insert any custom initializations here */
 CIC_LPFILTER_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker cic_lpfilter_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 return RCC_ADVANCE;
}
