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
 * THIS FILE WAS ORIGINALLY GENERATED ON Tue Jul 27 12:54:07 2010 EDT
 * BASED ON THE FILE: splitter2x2.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: splitter2x2
 */
#include "splitter2x2_Worker.h"

#include <stdio.h>
#include <string.h>

SPLITTER2X2_METHOD_DECLARATIONS;
RCCDispatch splitter2x2 =
{
  /* insert any custom initializations here */
  SPLITTER2X2_DISPATCH
};

/*
 * Methods to implement for worker splitter2x2, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  ( void ) self;
  /* Nothing to do */
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

/*
  Port A (from ADC) s1 (slave 1)
  Port B (from SMA0) s0 (slave 0)
  Port C (to delay) M0 (master 0)
  Port D (to framegate) M1 (master 1)

  Bit  0 select M0 source (0=s0 1=s1)
  Bit  7 disable M0
  Bit  8 select M1 source (0=s0 1=s1)
  Bit 15 disable M1
*/

#define SPLITTER2X2_WSIINA SPLITTER2X2_IN1
#define SPLITTER2X2_WSIINB SPLITTER2X2_IN0
#define SPLITTER2X2_WSIOUTC SPLITTER2X2_OUT0
#define SPLITTER2X2_WSIOUTD SPLITTER2X2_OUT1

enum
{
  A_TO_C = 0x8001,
  B_TO_C = 0x8000,
  A_TO_D = 0x0081,
  B_TO_D = 0x0080,
  A_TO_C_AND_D = 0x0101,
  B_TO_C_AND_D = 0x0000
  // There are other combinations that I did not need
};

static void copy ( void* to, const void* from, size_t n_bytes )
{
  memcpy ( to, from, n_bytes );
}

static RCCResult run ( RCCWorker* self,
                       RCCBoolean timedOut,
                       RCCBoolean* newRunCondition )
{
  ( void ) timedOut;
  ( void ) newRunCondition;

  Splitter2x2Properties* props = ( Splitter2x2Properties* ) self->properties;

  RCCPort* port_a = &( self->ports [ SPLITTER2X2_WSIINA ] );
  RCCPort* port_b = &( self->ports [ SPLITTER2X2_WSIINB ] );
  RCCPort* port_c = &( self->ports [ SPLITTER2X2_WSIOUTC ] );
  RCCPort* port_d = &( self->ports [ SPLITTER2X2_WSIOUTD ] );

  switch ( props->splitCtrl )
  {
    case A_TO_C:
      if ( port_c->current.data && port_a->current.data )
      {
        copy ( port_c->current.data,
               port_a->current.data,
               port_a->input.length );
        port_c->output.length = port_a->input.length;
        port_c->output.u.operation = port_a->input.u.operation;
      }
      break;
    case B_TO_C:
      if ( port_c->current.data && port_b->current.data )
      {
        copy ( port_c->current.data,
               port_b->current.data,
               port_b->input.length );
        port_c->output.length = port_b->input.length;
        port_c->output.u.operation = port_b->input.u.operation;
      }
      break;
    case A_TO_D:
      if ( port_d->current.data && port_a->current.data )
      {
        copy ( port_d->current.data,
               port_a->current.data,
               port_a->input.length );
        port_d->output.length = port_a->input.length;
        port_d->output.u.operation = port_a->input.u.operation;
      }
      break;
    case B_TO_D:
      if ( port_d->current.data && port_b->current.data )
      {
        copy ( port_d->current.data,
               port_b->current.data,
               port_b->input.length );
        port_d->output.length = port_b->input.length;
        port_d->output.u.operation = port_b->input.u.operation;
      }
      break;
    case A_TO_C_AND_D:
      if ( port_c->current.data && port_a->current.data )
      {
        copy ( port_c->current.data,
               port_a->current.data,
               port_a->input.length );
        port_c->output.length = port_a->input.length;
        port_c->output.u.operation = port_a->input.u.operation;
      }

      if ( port_d->current.data && port_a->current.data )
      {
        copy ( port_d->current.data,
               port_a->current.data,
               port_a->input.length );
        port_d->output.length = port_a->input.length;
        port_d->output.u.operation = port_a->input.u.operation;
      }
      break;
    case B_TO_C_AND_D:
      if ( port_c->current.data && port_b->current.data )
      {
        copy ( port_c->current.data,
               port_b->current.data,
               port_b->input.length );
        port_c->output.length = port_b->input.length;
        port_c->output.u.operation = port_b->input.u.operation;
      }

      if ( port_d->current.data && port_b->current.data )
      {

        copy ( port_d->current.data,
               port_b->current.data,
               port_b->input.length );
        port_d->output.length = port_b->input.length;
        port_d->output.u.operation = port_b->input.u.operation;
      }
      break;
    default:
      printf ( "\nSplitter2x2: Unsupported combination 0x%08x\n",
               props->splitCtrl );
      break;
  }

  return RCC_ADVANCE;
}
