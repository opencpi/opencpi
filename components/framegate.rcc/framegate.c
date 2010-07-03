/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Jun 26 16:55:37 2010 EDT
 * BASED ON THE FILE: framegate.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: framegate
 */
#include "framegate_Worker.h"

#include <string.h>

/* ---- Worker Memories Table -------------------------------------------- */

#define GATE_N_BYTES_INDEX 0

static uint32_t memories [ ] =
{
  sizeof ( size_t ), /* Trackes bytes between skips */
  0
};

/* ---- Worker Dispatch Table -------------------------------------------- */

FRAMEGATE_METHOD_DECLARATIONS;
RCCDispatch framegate =
{
  /* insert any custom initializations here */
  .memSizes = memories,

  FRAMEGATE_DISPATCH
};

/*
 * Methods to implement for worker framegate, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  ( void ) self;

  /* Nothing to do */

  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  FramegateProperties* p = ( FramegateProperties* ) self->properties;

  size_t* gate_n_bytes = ( size_t* ) self->memories [ GATE_N_BYTES_INDEX ];

  *gate_n_bytes = p->gateSize; // Pass first buffer

  return RCC_OK;
}

static RCCResult stop ( RCCWorker* self )
{
  ( void ) self;

  /* Nothing to do */

  return RCC_OK;
}

static RCCResult release ( RCCWorker* self )
{
  ( void ) self;

  /* Nothing to do */
  return RCC_OK;
}

/*
  We assume that all buffers are the same size and that the buffers are a
  multiple of the gateSize. The IP makes the same assumptions.
*/

#define BYPASS 0

static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  RCCResult rc = RCC_ADVANCE;

  FramegateProperties* p = ( FramegateProperties* ) self->properties;

  size_t* gate_n_bytes = ( size_t* ) self->memories [ GATE_N_BYTES_INDEX ];

  size_t n_bytes = self->ports [ FRAMEGATE_IN ].current.maxLength;

  if ( ( p->frameGateCtrl == BYPASS ) || ( *gate_n_bytes >= p->gateSize ) )
  {
    *gate_n_bytes = 0;

    const uint8_t* p_src =
             ( const uint8_t* ) self->ports [ FRAMEGATE_IN ].current.data;

    uint8_t* p_dst =
                  ( uint8_t* ) self->ports [ FRAMEGATE_OUT ].current.data;

    memcpy ( p_dst, p_src, n_bytes );

    rc = RCC_ADVANCE;
  }
  else
  {
    *gate_n_bytes += n_bytes;

    /* Only adavance input port (not output was produced) */
    self->container->advance ( &( self->ports [ FRAMEGATE_IN ] ), 0 );

    rc = RCC_OK;
  }

  return rc;
}
