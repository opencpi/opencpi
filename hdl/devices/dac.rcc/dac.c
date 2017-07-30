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
 * THIS FILE WAS ORIGINALLY GENERATED ON Sun Jul  4 14:25:38 2010 EDT
 * BASED ON THE FILE: dac.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: dac
 */
#include "dac_Worker.h"

#include <stdio.h>
#include <strings.h> // for bzero

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
