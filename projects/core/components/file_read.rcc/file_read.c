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
#include "file_read_Worker.h"

typedef struct {
  int fd;
  int started;
} MyState;
static size_t mysizes[] = {sizeof(MyState), 0};

FILE_READ_METHOD_DECLARATIONS;
RCCDispatch file_read = {
 /* insert any custom initializations here */
 FILE_READ_DISPATCH
 .memSizes = mysizes
};

/*
 * Methods to implement for worker file_read, based on metadata.
 */
static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memories[0];
  File_readProperties *p = self->properties;
  if (s->started)
    return RCC_OK;
  if ((s->fd = open(p->fileName, O_RDONLY)) < 0)
    return self->container.setError("error opening file \"%s\": %s", p->fileName, strerror(errno));
  s->started = 1;
  self->ports[FILE_READ_OUT].output.u.operation = p->opcode;
  if (p->granularity)
    p->messageSize -= p->messageSize % p->granularity;
  return RCC_OK;
} 

static RCCResult
release(RCCWorker *self) {
 MyState *s = self->memories[0];
  if (s->started)
    close(s->fd);
  return RCC_OK;
}

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
  RCCPort *port = &self->ports[FILE_READ_OUT];
  File_readProperties *props = self->properties;
  MyState *s = self->memories[0];
  size_t n2read = props->messageSize ? props->messageSize : port->current.maxLength;
  ssize_t n = 0; // needed only for warning suppression
  RCCBoolean zlmIn = 0;
  (void)timedOut;(void)newRunCondition;

  if (props->messagesInFile) {
    struct {
      uint32_t length;
      uint32_t opcode;
    } m;
    if ((n = read(s->fd, &m, sizeof(m))) != sizeof(m) && n) {
      props->badMessage = 1;
      return self->container.setError("can't read message header from file (%zd): %s",
				      n, strerror(errno));
    }
    zlmIn = n && m.length == 0;
    port->output.u.operation = n ? (RCCOpCode)m.opcode : props->opcode;
    n2read = n = n ? m.length : 0;
  }
  if (n2read > port->current.maxLength)
    return self->container.setError("message size (%zu) too large for max buffer size (%u)",
				    n2read, port->current.maxLength);
  if (n2read && (n = read(s->fd, port->current.data, n2read)) < 0)
    return self->container.setError("error reading file: %s", strerror(errno));
  if (props->messagesInFile && n != (ssize_t)n2read) {
    props->badMessage = 1;
    return self->container.setError("message truncated in file. header said %zu file had %zd",
				    n2read, n);
  }
  // Truncate the message for the granularity
  if (props->granularity)
    n -= n % props->granularity;
  port->output.length = n;
  props->bytesRead += n;
  if (n || zlmIn) { // MIF mode just passes ZLMs through with no special action
    props->messagesWritten++;
    return RCC_ADVANCE;
  }
  if (props->repeat) {
    if (lseek(s->fd, 0, SEEK_SET) < 0)
      return self->container.setError("error rewinding file: %s", strerror(errno));
    return RCC_OK;
  }
  close(s->fd);
  // Suppressing EOF with v2 just means we truly do not assert EOF at all.
  if (props->suppressEOF)
    return RCC_DONE;
#if 0 // API version < 2
  props->messagesWritten++;
#else
  port->output.eof = true;
#endif
  return RCC_ADVANCE_DONE;
}
