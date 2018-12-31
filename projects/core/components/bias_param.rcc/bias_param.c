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
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Jun  7 16:40:56 2014 EDT
 * BASED ON THE FILE: bias_param.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the bias_param worker in C
 */

#include "bias_param_Worker.h"

BIAS_PARAM_METHOD_DECLARATIONS;
RCCDispatch bias_param = {
 /* insert any custom initializations here */
 BIAS_PARAM_DISPATCH
};

/*
 * Methods to implement for worker bias_param, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)self;(void)timedOut;(void)newRunCondition;
 RCCPort
   *in = &self->ports[BIAS_PARAM_IN],
   *out = &self->ports[BIAS_PARAM_OUT];
 uint32_t
   *inData = in->current.data,
   *outData = out->current.data;

 if (in->input.length > out->current.maxLength) {
   self->errorString = "output buffer too small";
   return RCC_ERROR;
 }
 if (in->input.length % sizeof(uint32_t)) {
   self->errorString = "input message size not a multiple of data type";
   return RCC_ERROR;
 }
 for (unsigned n = in->input.length / sizeof(uint32_t); n; n--)
   *outData++ = *inData++ + BIAS_PARAM_BIASVALUE;
 out->output.length = in->input.length;
 out->output.u.operation = in->input.u.operation;
 return in->input.length ? RCC_ADVANCE : RCC_ADVANCE_DONE;
}
