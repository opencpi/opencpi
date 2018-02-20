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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <string.h>
#include "hello_Worker.h"

HELLO_METHOD_DECLARATIONS;
RCCDispatch hello = {
  /* insert any custom initializations here */
  HELLO_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */

// The message we send all the time
#define MESSAGE "Hello, world\n"

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *out = &self->ports[HELLO_OUT];

  if (sizeof(MESSAGE) > out->current.maxLength)
    return RCC_FATAL;
  strcpy(out->current.data, MESSAGE);
  out->output.length = sizeof(MESSAGE);
  return RCC_ADVANCE;
}
