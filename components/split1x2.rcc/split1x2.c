/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Sat Jun 26 12:42:24 2010 EDT
 * BASED ON THE FILE: split1x2.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: split1x2
 */
#include "split1x2_Worker.h"

#include <stdio.h>
#include <string.h>

SPLIT1X2_METHOD_DECLARATIONS;
RCCDispatch split1x2 =
{
  /* insert any custom initializations here */
  SPLIT1X2_DISPATCH
};

/*
 * Methods to implement for worker split1x2, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  Split1x2Properties* p = ( Split1x2Properties* ) self->properties;

  p->n_buffers = 0;

  return RCC_OK;
}

static RCCResult start ( RCCWorker* self )
{
  ( void ) self;
  /* Nothing to do */
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

static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  const uint8_t* p_src =
              ( const uint8_t* ) self->ports [ SPLIT1X2_WSIIN ].current.data;

  size_t n_bytes = self->ports [ SPLIT1X2_WSIIN ].current.maxLength;

  uint8_t* p_dst_a =
                  ( uint8_t* ) self->ports [ SPLIT1X2_WSIOUTA ].current.data;

  memcpy ( p_dst_a, p_src, n_bytes );

  self->ports [ SPLIT1X2_WSIOUTA ].output.length = n_bytes;

  uint8_t* p_dst_b =
                  ( uint8_t* ) self->ports [ SPLIT1X2_WSIOUTB ].current.data;

  memcpy ( p_dst_b, p_src, n_bytes );

  self->ports [ SPLIT1X2_WSIOUTB ].output.length = n_bytes;

  return RCC_ADVANCE;
}
