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
static size_t mysizes[] = {sizeof(MyState), 0};

FILE_WRITE_METHOD_DECLARATIONS;
RCCDispatch file_write = {
 /* insert any custom initializations here */
 FILE_WRITE_DISPATCH
 .memSizes = mysizes
};

/*
 * Methods to implement for worker file_read, based on metadata.
 */

static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memories[0];
  File_writeProperties *p = self->properties;

  if (s->started) {
    self->errorString = "file_read cannot be restarted";
    return RCC_ERROR;
  }
  s->started = 1;
  if ((s->fd = creat(p->fileName, 0666)) < 0) {
    asprintf(&self->errorString, "error creating file \"%s\": %s", p->fileName, strerror(errno));
    return RCC_ERROR;
  }
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
 RCCPort *port = &self->ports[FILE_WRITE_IN];
 File_writeProperties *props = self->properties;
 MyState *s = self->memories[0];

 // printf("In file_write.c got %u data = %x\n", port->input.length, *(uint32_t *)port->current.data);

 (void)timedOut;(void)newRunCondition;
 ssize_t n;
 if (props->messagesInFile) {
   struct {
     uint32_t length;
     uint32_t opcode;
   } m;
   m.length = port->input.length;
   m.opcode = port->input.u.operation;
   if ((n = write(s->fd, &m, sizeof(m))) < 0) {
     asprintf(&self->errorString, "error writing header to file: %s", strerror(errno));
     return RCC_ERROR;
   }
 }
 if (port->input.length) {
   if ((n = write(s->fd, port->current.data, port->input.length)) < 0 || n != port->input.length) {
     asprintf(&self->errorString, "error writing data to file: %s", strerror(errno));
     return RCC_ERROR;
   }
   props->bytesWritten += n;
 }
 props->messagesWritten++;
 return RCC_ADVANCE;
}
