/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Dec 21 15:54:06 2011 EST
 * BASED ON THE FILE: file_read.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: file_read
 */
#define _GNU_SOURCE // for asprintf
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include "file_write_Worker.h"

typedef struct {
  int fd;
  int started;
} MyState;

FILE_WRITE_METHOD_DECLARATIONS;
RCCDispatch file_write = {
 /* insert any custom initializations here */
 FILE_WRITE_DISPATCH
 .memSize = sizeof(MyState)
};

/*
 * Methods to implement for worker file_read, based on metadata.
 */

static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memory;
  File_writeProperties *p = self->properties;

  if (s->started)
    return self->container.setError("file_write cannot be restarted");
  if ((s->fd = creat(p->fileName, 0666)) < 0)
    return self->container.setError("error creating file \"%s\": %s",
				    p->fileName, strerror(errno));
  s->started = 1;
  return RCC_OK;
} 

static RCCResult
release(RCCWorker *self) {
  MyState *s = self->memory;
  if (s->started)
    close(s->fd);
 return RCC_OK;
}

static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 RCCPort *port = &self->ports[FILE_WRITE_IN];
 File_writeProperties *props = self->properties;
 MyState *s = self->memory;

 // printf("In file_write.c got %zu/%u data = %x\n", port->input.length, port->input.u.operation,
 //	*(uint32_t *)port->current.data);

 (void)timedOut;(void)newRunCondition;
 if (port->input.length == 0 && port->input.u.operation == 0 && props->stopOnEOF)
   return RCC_ADVANCE_DONE;
 if (props->messagesInFile) {
   struct {
     uint32_t length;
     uint32_t opcode;
   } m = { port->input.length, port->input.u.operation };
   if (write(s->fd, &m, sizeof(m)) != (ssize_t)sizeof(m))
     return self->container.setError("error writing header to file: %s", strerror(errno));
 }
 if (port->input.length &&
     write(s->fd, port->current.data, port->input.length) != (ssize_t)port->input.length)
   return self->container.setError("error writing data to file: length %zu(%zx): %s",
				   port->input.length, port->input.length, strerror(errno));
 props->bytesWritten += port->input.length;
 props->messagesWritten++; // this includes non-EOF ZLMs even though no data was written.
 return RCC_ADVANCE;
}
