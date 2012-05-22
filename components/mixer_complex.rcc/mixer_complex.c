/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue May 22 09:05:20 2012 EDT
 * BASED ON THE FILE: mixer_complex.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: mixer_complex
 */
#include "mixer_complex_Worker.h"

MIXER_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch mixer_complex = {
 /* insert any custom initializations here */
 MIXER_COMPLEX_DISPATCH
};

/*
 * Methods to implement for worker mixer_complex, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 return RCC_ADVANCE;
}
