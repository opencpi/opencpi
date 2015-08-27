/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Mar  9 17:36:50 2015 EDT
 * BASED ON THE FILE: fft1d.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the fft1d worker in C
 */

#include "fft1d_Worker.h"

FFT1D_METHOD_DECLARATIONS;
RCCDispatch fft1d = {
 /* insert any custom initializations here */
 FFT1D_DISPATCH
};

/*
 * Methods to implement for worker fft1d, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 return RCC_ADVANCE;
}
