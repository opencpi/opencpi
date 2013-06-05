
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

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <RCC_Worker.h>
#include "fileSource.h"

#if defined (__VXWORKS__)
#include <ioLib.h>
#endif

typedef struct {
  int fd;
} FileSourceContext;

static
RCCResult
FileSourceInitialize (RCCWorker * wctx)
{
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  props->portName[0] = '\0';
  props->fileName[0] = '\0';
  props->bytesPerPacket = 0;
  props->offset = 0;
  props->errnoValue = 0;
  props->atEof = 0;
  props->verbose = 0;

  return RCC_OK;
}

static
RCCResult
FileSourceStart (RCCWorker * wctx)
{
  FileSourceContext * ctx = (FileSourceContext *) wctx->memories[0];
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  ctx->fd = open (props->fileName, O_RDONLY, 0);

  if (ctx->fd < 0) {
    if (props->verbose) {
      printf ("Input port %s: Failed to open \"%s\" for reading: %s\n",
              props->portName, props->fileName,
              strerror (errno));
    }

    props->errnoValue = (uint32_t)errno;
    return RCC_ERROR;
  }

  if (props->verbose) {
    printf ("Input port %s: Openend \"%s\" for reading.\n",
            props->portName, props->fileName);
  }

  props->offset = 0;
  return RCC_OK;
}

static
RCCResult
FileSourceStop (RCCWorker * wctx)
{
  FileSourceContext * ctx = (FileSourceContext *) wctx->memories[0];
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  if (props->verbose) {
    printf ("Input port %s: Closing \"%s\".\n",
            props->portName, props->fileName);
  }

  close (ctx->fd);
  return RCC_OK;
}

static
RCCResult
FileSourceRun (RCCWorker * wctx,
               RCCBoolean timedout,
               RCCBoolean * newRunCondition)
{
  ( void ) timedout;
  ( void ) newRunCondition;
  FileSourceContext * ctx = (FileSourceContext *) wctx->memories[0];
  FileSourceProperties * props = (FileSourceProperties *) wctx->properties;

  RCCPort * pDataOut = &wctx->ports[FILESOURCE_DATAOUT];

  size_t max = props->bytesPerPacket ? props->bytesPerPacket : pDataOut->current.maxLength;
  ssize_t count = read (ctx->fd, (char *) pDataOut->current.data, max);

  if (count < 0) {
    if (props->verbose) {
      printf ("Input port %s: Failed to read from \"%s\": %s\n",
              props->portName, props->fileName,
              strerror (errno));
    }

    props->errnoValue = (uint32_t)errno;
    return RCC_ERROR;
  }
  else if (count == 0) {
    if (props->verbose) {
      printf ("Input port %s: End of file.\n",
              props->portName);
    }

    props->atEof = 1;
    return RCC_DONE;
  }

  pDataOut->output.length = (size_t)count;
  pDataOut->output.u.operation = 0;
  props->offset += (uint32_t)count;

  if (props->verbose) {
    printf ("Input port %s: Read %d bytes.\n",
            props->portName, (int) count);
  }

  return RCC_ADVANCE;
}

static
size_t
FileSourceMemories[] = {
  sizeof (FileSourceContext),
  0
};

RCCDispatch
TestWorkerFileSourceWorker = {
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
  .test = NULL,
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
