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
  s->started = 1;
  if ((s->fd = open(p->fileName, O_RDONLY)) < 0) {
    asprintf(&self->errorString, "error opening file \"%s\": %s", p->fileName, strerror(errno));
    return RCC_ERROR;
  }
  self->ports[FILE_READ_OUT].output.u.operation = p->opcode;
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
 size_t n2read =  props->messageSize ? props->messageSize : port->current.maxLength;
 ssize_t n;

 (void)timedOut;(void)newRunCondition;
 if (props->messageSize > port->current.maxLength) {
   self->errorString = "message size property too large for buffers";
   return RCC_ERROR;
 }
 if (props->granularity)
   n2read -= n2read % props->granularity;
 if ((n = read(s->fd, port->current.data, n2read)) < 0) {
   asprintf(&self->errorString, "error reading file: %s", strerror(errno));
   return RCC_ERROR;
 }
 if (props->granularity && n)
   n -= n % props->granularity;
 // printf("In file_read.c got %zu data = %x\n", n, *(uint32_t *)port->current.data);
 port->output.length = n;
 props->bytesRead += n;
 if (n) {
   props->messagesWritten++;
   return RCC_ADVANCE;
 }
 if (props->repeat) {
   if (lseek(s->fd, 0, SEEK_SET) < 0) {
     asprintf(&self->errorString, "error rewinding file: %s", strerror(errno));
     return RCC_ERROR;
   }
   return RCC_OK;
 }
 close(s->fd);
 return RCC_ADVANCE_DONE;
}
