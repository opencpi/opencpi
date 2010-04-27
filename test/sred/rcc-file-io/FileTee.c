// Copyright (c) 2009 Mercury Federal Systems.
// 
// This file is part of OpenCPI.
// 
// OpenCPI is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// OpenCPI is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.

/*
 * File tee worker.
 *
 * Revision History:
 *
 *     06/23/2009 - Frank Pilhofer
 *                  Use WWW_N_<IN|OUT>PUT_PORTS constants from header file.
 *
 *     06/19/2009 - Frank Pilhofer
 *                  Fix spelling of Filetee -> FileTee.
 *
 *     06/15/2009 - Frank Pilhofer
 *                  Reserve extra byte for null character in lastFileName.
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
#include "FileTee.h"

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
} FileTeeContext;

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
FileTeeInitialize (RCCWorker * wctx)
{
  FileTeeContext * ctx = (FileTeeContext *) wctx->memories[0];
  FileTeeProperties * props = (FileTeeProperties *) wctx->properties;

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
FileTeeStart (RCCWorker * wctx)
{
  FileTeeContext * ctx = (FileTeeContext *) wctx->memories[0];
  FileTeeProperties * props = (FileTeeProperties *) wctx->properties;

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
FileTeeStop (RCCWorker * wctx)
{
  FileTeeContext * ctx = (FileTeeContext *) wctx->memories[0];
  FileTeeProperties * props = (FileTeeProperties *) wctx->properties;

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
FileTeeRun (RCCWorker * wctx,
	    RCCBoolean timedout,
	    RCCBoolean * newRunCondition)
{
  FileTeeContext * ctx = (FileTeeContext *) wctx->memories[0];
  FileTeeProperties * props = (FileTeeProperties *) wctx->properties;

  RCCPort * pDataIn = &wctx->ports[FILETEE_DATAIN];
  RCCPort * pDataOut = &wctx->ports[FILETEE_DATAOUT];
  uint32_t length = pDataIn->input.length;

  ssize_t count = write (ctx->fd, pDataIn->current.data, length);

  if (count != length) {
    return RCC_ERROR;
  }

  props->offset += length;
  wctx->container->send (pDataOut, &pDataIn->current, 0, length);

  printf ("FileTee: Copied %u.\n", count);
  return RCC_OK;
}

/*
 * Worker memory requests.
 *
 * These memories are allocated by the container and provided to the worker
 * in the "memories" member of the RCCWorker structure.
 */

static
uint32_t
FileTeeMemories[] = {
  sizeof (FileTeeContext),
  0
};

/*
 * Worker dispatch table.
 *
 * This is the only symbol that is exported (non-static) from this file.
 * The name of the dispatch table is used as this worker's "entrypoint".
 */

RCCDispatch
FileTeeWorker = {
  /*
   * Information for consistency checking by the container.
   */

  .version = RCC_VERSION,
  .numInputs = FILETEE_N_INPUT_PORTS,
  .numOutputs = FILETEE_N_OUTPUT_PORTS,
  .propertySize = sizeof (FileTeeProperties),
  .memSizes = FileTeeMemories,
  .threadProfile = 0,

  /*
   * Methods.  Can be NULL if not needed.
   */

  .initialize = FileTeeInitialize,
  .start = FileTeeStart,
  .stop = FileTeeStop,
  .release = NULL,
  .test = NULL,
  .afterConfigure = NULL,
  .beforeQuery = NULL,
  .run = FileTeeRun,

  /*
   * Implementation information for container behavior.
   */

  .runCondition = NULL,
  .portInfo = NULL,
  .optionallyConnectedPorts = 0
};
