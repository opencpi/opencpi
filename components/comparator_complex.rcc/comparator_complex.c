/*
 * This file is protected by Copyright. Please refer to the COPYRIGHT file
 * distributed with this source distribution.
 *
 * This file is part of OpenCPI <http://www.opencpi.org>
 *
 * OpenCPI is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, either version 3 of the License, or (at your option) any
 * later version.
 *
 * OpenCPI is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Aug 24 06:10:33 2012 PDT
 * BASED ON THE FILE: comparator.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: comparator
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "comparator_complex_Worker.h"
#include "signal_utils.h"
#include "assert.h"

COMPARATOR_COMPLEX_METHOD_DECLARATIONS;
RCCDispatch comparator_complex = {
  /* insert any custom initializations here */
  COMPARATOR_COMPLEX_DISPATCH
};


static RCCResult
start(RCCWorker *self) {
  Comparator_complexProperties *p = self->properties;

  // We do this since a single failure will turn this to a false but we still
  // want to run the entire test to generate the output files for debug and 
  // analysis
  p->passed = 1;

  return RCC_OK;
}

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

  Comparator_complexProperties *p = self->properties;

  printf("In comparator::runComplexTest\n");

  RCCPort
    *in_unit_test = &self->ports[COMPARATOR_COMPLEX_IN_UNIT_TEST],
    *in_expected = &self->ports[COMPARATOR_COMPLEX_IN_EXPECTED],
    *out_delta = &self->ports[COMPARATOR_COMPLEX_OUT_DELTA],
    *out_actual = &self->ports[COMPARATOR_COMPLEX_OUT_ACTUAL];
   
  Comparator_complexIn_unit_testIq
    *inUTData = in_unit_test->current.data,
    *inEXData = in_expected->current.data,
    *outActualData = out_actual->current.data;

  int16_t
    *outDeltaData = out_delta->current.data;

  if ( in_unit_test->input.length != in_expected->input.length ) {
    printf("/n/n The data length coming from the unit under test and the expected results file differ\n");
    printf("UUT data length = %zu, expected data length = %zu\n", in_unit_test->input.length ,  in_expected->input.length );
    printf("This will cause the test to fail. The output of the UUT and the expected results data should be identical\n");
    p->passed = 0;
  }
  size_t len = byteLen2Complex( in_unit_test->input.length );
  size_t i;
  if (real2bytes(len) > out_delta->current.maxLength)
    return self->container.setError("Delta output buffer size %zu, but %zu is required",
				    out_delta->current.maxLength, real2bytes(len));
  for ( i=0; i<len; i++ ) {
    double delta = fabs( scabs(inUTData->data[i].I, inUTData->data[i].Q) - 
			 scabs(inEXData->data[i].I, inEXData->data[i].Q) );

    //    printf("Calculated delta = %f \n", delta );
    if ( delta > p->deviation ) {
      p->passed = 0;
    }
    outDeltaData[i] = Scale( delta );
  }
  out_delta->output.u.operation = 0;
  out_delta->output.length = Complex2bytes(len);

  memcpy( outActualData, inUTData,  in_unit_test->input.length);
  out_actual->output.u.operation = in_unit_test->input.u.operation;
  out_actual->output.length = in_unit_test->input.length;
 
  return RCC_ADVANCE;
}
