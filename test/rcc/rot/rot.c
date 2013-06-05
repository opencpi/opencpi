
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * "rot" worker implementing a simple caesarean "encryption".
 *
 * Revision History:
 *
 *     06/23/2009 - Frank Pilhofer
 *                  Use WWW_N_<IN|OUT>PUT_PORTS constants from header file.
 *
 *     06/03/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <stdio.h>
#include <stddef.h>
#include <RCC_Worker.h>
#include <ctype.h>
#include "rot.h"

static
RCCResult
rotInitialize (RCCWorker * wctx)
{
  RotProperties * props = (RotProperties *) wctx->properties;
  props->key = 0;
  return RCC_OK;
}

static
RCCResult
rotStart (RCCWorker * wctx)
{
  (void)wctx;
  return RCC_OK;
}

static
RCCResult
rotStop (RCCWorker * wctx)
{
  (void)wctx;
  return RCC_OK;
}

static
RCCResult
rotRun (RCCWorker * wctx,
        RCCBoolean timedout,
        RCCBoolean * newRunCondition)
{
  (void)timedout;(void)newRunCondition;
  RotProperties * props = (RotProperties *) wctx->properties;

  RCCPort * pDataIn = &wctx->ports[ROT_DATAIN];
  RCCPort * pDataOut = &wctx->ports[ROT_DATAOUT];

  unsigned int count = pDataIn->input.length;
  const char * iptr = pDataIn->current.data;
  char * optr = pDataOut->current.data;
  int key = props->key;

  if (count > pDataOut->current.maxLength) {
    return RCC_ERROR;
  }

  pDataOut->output.length = count;

  while (count--) {
    char c = *iptr++;

    if (isupper (c)) {
      c = (char)('A' + (((c - 'A') + key) % 26));
    }
    else if (islower (c)) {
      c = (char)('a' + (((c - 'a') + key) % 26));
    }

    *optr++ = c;
  }

  return RCC_ADVANCE;
}

RCCDispatch
rotWorker = {
  /*
   * Information for consistency checking by the container.
   */

  .version = RCC_VERSION,
  .numInputs = ROT_N_INPUT_PORTS,
  .numOutputs = ROT_N_OUTPUT_PORTS,
  .propertySize = sizeof (RotProperties),
  .memSizes = NULL,
  .threadProfile = 0,

  /*
   * Methods.  Can be NULL if not needed.
   */

  .initialize = rotInitialize,
  .start = rotStart,
  .stop = rotStop,
  .release = NULL,
  .afterConfigure = NULL,
  .beforeQuery = NULL,
  .test = NULL,
  .run = rotRun,

  /*
   * Implementation information for container behavior.
   */

  .runCondition = NULL, /* Implies a run condition of all ports ready. */
  .portInfo = NULL, /* Non-default port information */
  .optionallyConnectedPorts = 0 /* Bit mask */
};
