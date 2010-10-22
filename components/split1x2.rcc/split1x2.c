
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
  .memSizes = 0,
  .test = 0,
  .afterConfigure = 0,
  .beforeQuery = 0,
  .runCondition = 0,
  .portInfo = 0,
  .optionallyConnectedPorts = 0,

  /* insert any custom initializations here */
  SPLIT1X2_DISPATCH
};

/*
 * Methods to implement for worker split1x2, based on metadata.
*/

static RCCResult initialize ( RCCWorker* self )
{
  Split1x2Properties* p = ( Split1x2Properties* ) self->properties;

  p->n_bytes_a = 0;
  p->n_buffers_a = 0;

  p->n_bytes_b = 0;
  p->n_buffers_b = 0;

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
  Split1x2Properties* props = ( Split1x2Properties* ) self->properties;

  RCCPort* port_in = &( self->ports [ SPLIT1X2_WSIIN ] );

  RCCPort* port_a = &( self->ports [ SPLIT1X2_WSIOUTA ] );

  if ( port_in->current.data && port_a->current.data )
  {
    copy ( port_a->current.data,
           port_in->current.data,
           port_in->input.length );

    port_a->output.length = port_in->input.length;
    port_a->output.u.operation = port_in->input.u.operation;

    props->n_buffers_a++;
    props->n_bytes_a += port_in->input.length;
  }

  RCCPort* port_b = &( self->ports [ SPLIT1X2_WSIOUTB ] );

  if ( port_in->current.data && port_b->current.data )
  {
    copy ( port_b->current.data,
           port_in->current.data,
           port_in->input.length );

    port_b->output.length = port_in->input.length;
    port_b->output.u.operation = port_in->input.u.operation;

    props->n_buffers_b++;
    props->n_bytes_b += port_in->input.length;
  }

  return RCC_ADVANCE;
}
