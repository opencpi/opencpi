
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
 * File sink worker.
 *
 * Revision History:
 *
 *     06/23/2009 - Frank Pilhofer
 *                  Use WWW_N_<IN|OUT>PUT_PORTS constants from header file.
 *
 *     06/19/2009 - Frank Pilhofer
 *                  Fix spelling of Filesink -> FileSink.
 *
 *     06/15/2009 - Frank Pilhofer
 *                  - Reserve extra byte for null character in lastFileName.
 *                  - Fix -Wall warnings.
 *
 *     06/03/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <RCC_Worker.h>
#include "FileSink.h"

/*
 * Worker private memory.
 *
 * This convenient data structure can be used to hold private data used by
 * the worker instance.  The container allocates (but does not initialize)
 * an instance of this structure per worker instance.
 */

typedef struct {
  char lastFileName[257];
  int fd;
} FileSinkContext;

/*
 * Worker initialization.
 *
 * The initialize operation provides the worker with the opportunity to
 * perform any one-time initialization to achieve a known state prior to
 * normal execution.
 *
 * The property set and the worker context (if applicable) should be
 * initialized here.
 */

static
RCCResult
FileSinkInitialize (RCCWorker * wctx)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  ctx->lastFileName[0] = '\0';
  props->fileName[0] = '\0';
  props->offset = 0;

  return RCC_OK;
}

/*
 * Prepare to run.
 *
 * The start operation provides the worker with the opportunity to perform
 * any one-time initialization that are dependent on initial configuration
 * property values, since those property values are not set prior to the
 * the initialize operation.  This operation also provides the opportunity
 * to prepare to resume internal processing after the stop operation is
 * used.
 */

static
RCCResult
FileSinkStart (RCCWorker * wctx)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  if (strcmp (props->fileName, ctx->lastFileName) == 0) {
    ctx->fd = open (props->fileName, O_WRONLY | O_APPEND, 0);
  }
  else {
    ctx->fd = open (props->fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  }

  props->offset = 0;

  if (ctx->fd < 0) {
    return RCC_ERROR;
  }

  strcpy (ctx->lastFileName, props->fileName);
  return RCC_OK;
}

/*
 * Stop processing.
 *
 * The stop operation is provided to command a worker to stop internal
 * processing in a way that can be later restarted via the start operation.
 */

static
RCCResult
FileSinkStop (RCCWorker * wctx)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];

  if (close (ctx->fd) != 0) {
    return RCC_ERROR;
  }

  return RCC_OK;
}

/*
 * Process data.
 *
 * The run operation requests that the worker perform its normal computation.
 * The container only calls this operation when the worker's run condition is
 * true.
 *
 * Normally this involves using messages in buffers at input ports to produce
 * messages at output ports.
 */

static
RCCResult
FileSinkRun (RCCWorker * wctx,
             RCCBoolean timedout,
             RCCBoolean * newRunCondition)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  RCCPort * pDataIn = &wctx->ports[FILESINK_DATAIN];
  ssize_t count = write (ctx->fd, pDataIn->current.data, pDataIn->input.length);

  if (count != pDataIn->input.length) {
    return RCC_ERROR;
  }

  props->offset += count;
  printf ("FileSink: Consumed %u.\n", (unsigned int) count);
  return RCC_ADVANCE;
}

/*
 * Worker memory requests.
 *
 * These memories are allocated by the container and provided to the worker
 * in the "memories" member of the RCCWorker structure.
 */

static
uint32_t
FileSinkMemories[] = {
  sizeof (FileSinkContext),
  0
};

/*
 * Worker dispatch table.
 *
 * This is the only symbol that is exported (non-static) from this file.
 * The name of the dispatch table is used as this worker's "entrypoint".
 */

RCCDispatch
FileSinkWorker = {
  /*
   * Information for consistency checking by the container.
   */

  .version = RCC_VERSION,
  .numInputs = FILESINK_N_INPUT_PORTS,
  .numOutputs = FILESINK_N_OUTPUT_PORTS,
  .propertySize = sizeof (FileSinkProperties),
  .memSizes = FileSinkMemories,
  .threadProfile = 0,

  /*
   * Methods.  Can be NULL if not needed.
   */

  .initialize = FileSinkInitialize,
  .start = FileSinkStart,
  .stop = FileSinkStop,
  .release = NULL,
  .test = NULL,
  .afterConfigure = NULL,
  .beforeQuery = NULL,
  .run = FileSinkRun,

  /*
   * Implementation information for container behavior.
   */

  .runCondition = NULL, /* Implies a run condition of all ports ready. */
  .portInfo = NULL,
  .optionallyConnectedPorts = 0
};
