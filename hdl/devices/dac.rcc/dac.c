
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
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Jul  4 14:25:38 2010 EDT
 * BASED ON THE FILE: dac.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: dac
 */
#include "dac_Worker.h"

#include <stdio.h>
#include <string.h>

DAC_METHOD_DECLARATIONS;
RCCDispatch dac = {
  /* insert any custom initializations here */
  .memSizes = 0,
  .test = 0,
  .afterConfigure = 0,
  .beforeQuery = 0,
  .runCondition = 0,
  .portInfo = 0,
  .optionallyConnectedPorts = 0,

  DAC_DISPATCH
};

/*
 * Methods to implement for worker dac, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  DacProperties* p = ( DacProperties* ) self->properties;

  bzero ( p, sizeof ( *p ) );

  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  DacProperties* p = ( DacProperties* ) self->properties;

  if ( p->dacControl == 0x19 )
  {
    printf ( "DAC is emitting a CM\n" );
  }

  return RCC_OK;
}

static RCCResult stop ( RCCWorker* self )
{
  DacProperties* p = ( DacProperties* ) self->properties;

  if ( p->dacControl == 0x19 )
  {
    printf ( "DAC is done emitting a CM\n" );
  }

 return RCC_OK;
}

static RCCResult release ( RCCWorker* self )
{
  /* Nothing to do */
  ( void ) self;
  return RCC_OK;
}

static RCCResult run ( RCCWorker *self,
                       RCCBoolean timedOut,
                       RCCBoolean *newRunCondition )
{
  ( void ) self;
  ( void ) timedOut;
  ( void ) newRunCondition;
  /* Nothing to do */
  return RCC_ADVANCE;
}
