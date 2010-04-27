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
 * File source worker.
 *
 * Revision History:
 *
 *     06/23/2009 - Frank Pilhofer
 *                  Use WWW_N_<IN|OUT>PUT_PORTS constants from header file.
 *
 *     06/19/2009 - Frank Pilhofer
 *                  Fix spelling of Filesource -> FileSource.
 *
 *     06/15/2009 - Frank Pilhofer
 *                  Fix -Wall warnings.
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
#include <errno.h>
#include <RCC_Worker.h>
#include "FileSource.h"

/*
 * Worker private memory.
 *
 * This convenient data structure can be used to hold private data used by
 * the worker instance.  The container allocates (but does not initialize)
 * an instance of this structure per worker instance.
 */

typedef struct {
  int fd;
} FileSourceContext;

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
FileSourceInitialize (RCCWorker * wctx)
{
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  props->fileName[0] = '\0';
  props->bytesPerPacket = 0;
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
FileSourceStart (RCCWorker * wctx)
{
  FileSourceContext * ctx = (FileSourceContext *) wctx->memories[0];
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  ctx->fd = open (props->fileName, O_RDONLY, 0);
  props->offset = 0;

  if (ctx->fd < 0) {
    return RCC_ERROR;
  }

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
FileSourceStop (RCCWorker * wctx)
{
  FileSourceContext * ctx = (FileSourceContext *) wctx->memories[0];

  close (ctx->fd);

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
FileSourceRun (RCCWorker * wctx,
               RCCBoolean timedout,
               RCCBoolean * newRunCondition)
{
  FileSourceContext * ctx = (FileSourceContext *) wctx->memories[0];
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  RCCPort * pDataOut = &wctx->ports[FILESOURCE_DATAOUT];

  uint32_t max = props->bytesPerPacket ? props->bytesPerPacket : pDataOut->current.maxLength;
  ssize_t count = read (ctx->fd, pDataOut->current.data, max);

  if (count < 0) {
    return RCC_ERROR;
  }
  else if (count == 0) {
    printf ("FileSource: done.\n");
    return RCC_DONE;
  }

  pDataOut->output.length = count;
  pDataOut->output.u.operation = 0;
  props->offset += count;
  printf ("FileSource: Produced %u.\n", (unsigned int) count);

  return RCC_ADVANCE;
}

static
RCCResult
FileSourceTest (RCCWorker * wctx)
{
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  if (props->testId == 1) {
    props->errnoValue = errno;
  }
  else {
    return RCC_ERROR;
  }

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
FileSourceMemories[] = {
  sizeof (FileSourceContext),
  0
};

/*
 * Worker dispatch table.
 *
 * This is the only symbol that is exported (non-static) from this file.
 * The name of the dispatch table is used as this worker's "entrypoint".
 */

RCCDispatch
FileSourceWorker = {
  /*
   * Information for consistency checking by the container.
   */

  .version = RCC_VERSION,
  .numInputs = FILESOURCE_N_INPUT_PORTS,
  .numOutputs = FILESOURCE_N_OUTPUT_PORTS,
  .propertySize = sizeof (FileSourceProperties),
  .memSizes = FileSourceMemories,
  .threadProfile = 0,

  /*
   * Methods.  Can be NULL if not needed.
   */

  .initialize = FileSourceInitialize,
  .start = FileSourceStart,
  .stop = FileSourceStop,
  .release = NULL,
  .test = FileSourceTest,
  .afterConfigure = NULL,
  .beforeQuery = NULL,
  .run = FileSourceRun,

  /*
   * Implementation information for container behavior.
   */

  .runCondition = NULL, /* Implies a run condition of all ports ready. */
  .portInfo = NULL,
  .optionallyConnectedPorts = 0
};
