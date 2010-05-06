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
 *                  Fix -Wall warnings.
 *
 *     06/03/2009 - Frank Pilhofer
 *                  Initial version.
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <RCC_Worker.h>
#include "fileSink.h"

#if defined (__VXWORKS__)
#include <ioLib.h>
#endif

typedef struct {
  int fd;
} FileSinkContext;

static
RCCResult
FileSinkInitialize (RCCWorker * wctx)
{
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  props->portName[0] = '\0';
  props->fileName[0] = '\0';
  props->offset = 0;
  props->errnoValue = 0;
  props->verbose = 0;

  return RCC_OK;
}

static
RCCResult
FileSinkStart (RCCWorker * wctx)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  ctx->fd = open (props->fileName, O_WRONLY | O_CREAT | O_TRUNC, 0666);

  if (ctx->fd < 0) {
    if (props->verbose) {
      printf ("Output port %s: Failed to open \"%s\" for writing: %s\n",
              props->portName, props->fileName,
              strerror (errno));
    }

    props->errnoValue = errno;
    return RCC_ERROR;
  }

  if (props->verbose) {
    printf ("Output port %s: Openend \"%s\" for writing.\n",
            props->portName, props->fileName);
  }

  props->offset = 0;
  return RCC_OK;
}

static
RCCResult
FileSinkStop (RCCWorker * wctx)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  if (close (ctx->fd) != 0) {
    if (props->verbose) {
      printf ("Output port %s: Error closing \"%s\": %s\n",
              props->portName, props->fileName,
              strerror (errno));
    }

    props->errnoValue = errno;
    return RCC_ERROR;
  }

  if (props->verbose) {
    printf ("Output port %s: Closing \"%s\".\n",
            props->portName, props->fileName);
  }

  return RCC_OK;
}

static
RCCResult
FileSinkRun (RCCWorker * wctx,
             RCCBoolean timedout,
             RCCBoolean * newRunCondition)
{
  FileSinkContext * ctx = (FileSinkContext *) wctx->memories[0];
  FileSinkProperties * props = (FileSinkProperties *) wctx->properties;

  RCCPort * pDataIn = &wctx->ports[FILESINK_DATAIN];
  ssize_t count = write (ctx->fd, (char *) pDataIn->current.data, pDataIn->input.length);

  if (count != pDataIn->input.length) {
    if (props->verbose) {
      printf ("Output port %s: Failed to write to \"%s\": %s\n",
              props->portName, props->fileName,
              strerror (errno));
    }

    props->errnoValue = errno;
    return RCC_ERROR;
  }

  if (props->verbose) {
    printf ("Output port %s: Wrote %d bytes.\n",
            props->portName, (int) count);
  }

  props->offset += count;
  return RCC_ADVANCE;
}

static
uint32_t
FileSinkMemories[] = {
  sizeof (FileSinkContext),
  0
};

RCCDispatch
TestWorkerFileSinkWorker = {
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
