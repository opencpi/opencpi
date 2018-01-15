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
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Dec 21 15:54:06 2011 EST
 * BASED ON THE FILE: file_read.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: file_read
 */
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "file_write_msg_Worker.h"
#include "stream_data_file_format.h"

typedef struct {
  int fd;
  int started;
} MyState;
static size_t mysizes[] = {sizeof(MyState), 0};

FILE_WRITE_MSG_METHOD_DECLARATIONS;
RCCDispatch file_write_msg = {
 /* insert any custom initializations here */
 FILE_WRITE_MSG_DISPATCH
 .memSizes = mysizes
};

/*
 * Methods to implement for worker file_read, based on metadata.
 */

static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memories[0];
  File_write_msgProperties *p = self->properties;

  if (s->started) {
    self->container.setError("file_write cannot be restarted");
    return RCC_ERROR;
  }
  s->started = 1;
  if ((s->fd = creat(p->fileName, 0666)) < 0) {
    self->container.setError( "error creating file \"%s\": %s", p->fileName, strerror(errno));
    return RCC_ERROR;
  }
  return RCC_OK;
} 

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 (void)timedOut;(void)newRunCondition;
 RCCPort *port = &self->ports[FILE_WRITE_MSG_IN];
 File_write_msgProperties *props = self->properties;
 MyState *s = self->memories[0];
 ssize_t n;

 printf("In file_write_msg got %zu bytes of data\n", port->input.length);

 FileHeader h;
 h.pad = 0;
 h.endian = 1;
 h.opcode = port->input.u.operation;
 h.length = port->input.length;
 if ((n = write(s->fd, &h, sizeof(FileHeader) )) < 0)  {
   self->container.setError( "error writing header to file: %s", strerror(errno));
   return RCC_ERROR;
 }
 props->bytesWritten += n;

 if (port->input.length) {
   if ((n = write(s->fd, port->current.data, port->input.length)) < 0) {
     self->container.setError( "error writing data to file: %s", strerror(errno));
     return RCC_ERROR;
   }
   props->bytesWritten += n;
   props->messagesRead++;
   return RCC_ADVANCE;
 }

 printf("file_write_msg: Done\n");
 close(s->fd);
 return RCC_DONE;
}
