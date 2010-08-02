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
