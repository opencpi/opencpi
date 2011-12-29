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
#include "file_write_Worker.h"

typedef struct {
  int fd;
  int started;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};

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
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 RCCPort *port = &self->ports[FILE_WRITE_IN];
 File_writeProperties *props = self->properties;
 MyState *s = self->memories[0];

 (void)timedOut;(void)newRunCondition;
 if (port->input.length) {
   ssize_t n;
   if ((n = write(s->fd, port->current.data, port->input.length)) < 0) {
     asprintf(&self->errorString, "error reading file: %s", strerror(errno));
     return RCC_ERROR;
   }
   props->bytesWritten += n;
   props->messagesRead++;
   return RCC_ADVANCE;
 }
 close(s->fd);
 return RCC_DONE;
}
