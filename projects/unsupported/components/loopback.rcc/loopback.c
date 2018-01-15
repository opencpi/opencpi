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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Aug  6 05:56:05 2012 PDT
 * BASED ON THE FILE: loopback.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: loopback
 */
#include "loopback_Worker.h"

LOOPBACK_METHOD_DECLARATIONS;
RCCDispatch loopback = {
 /* insert any custom initializations here */
 LOOPBACK_DISPATCH
};

/*
 * Methods to implement for worker loopback, based on metadata.
 */

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  (void)timedOut;(void)newRunCondition;

 RCCPort
   *in = &self->ports[LOOPBACK_IN],
   *out = &self->ports[LOOPBACK_OUT];
   
 if (in->input.length > out->current.maxLength) {
   self->container.setError( "output buffer too small" );
   return RCC_ERROR;
 }

#ifndef NDEBUG
 printf("%s got %zu bytes of data\n", __FILE__,  in->input.length);
#endif

 //#define ZCOPY__
#ifdef ZCOPY__
 // Zero copy transfer
 self->container.send( out, &in->current, in->input.u.operation, in->input.length);
 return RCC_OK;
#else
 out->output.u.operation = in->input.u.operation;
 out->output.length = in->input.length;
 memcpy( out->current.data, in->current.data,  in->input.length);
 return RCC_ADVANCE; 
#endif



}
