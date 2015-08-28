/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Wed Dec 21 15:54:06 2011 EST
 * BASED ON THE FILE: file_read.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: file_read
 */
#include <inttypes.h>
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
  if (self->crewSize > 1 && (!p->messageSize || p->messagesInFile))
    return self->container.
      setError("Must use fixed message sizes and no messages-in-file when file_read is scaled > 1");
  if (s->started)
    return RCC_OK;
  s->started = 1;
  if ((s->fd = open(p->fileName, O_RDONLY)) < 0)
    return self->container.setError("error opening file \"%s\": %s", p->fileName, strerror(errno));
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
  size_t n2read =  props->messageSize ? props->messageSize : port->current.maxLength;
  ssize_t n;
  (void)timedOut;(void)newRunCondition;

  // Initialize our seek position the first time we're here
  if (self->crewSize > 1 && self->firstRun && self->member &&
      lseek(s->fd, props->messageSize * self->member, SEEK_CUR) == -1)
    return self->container.setError("error skipping data in scaled worker");
  if (props->messagesInFile) {
    struct {
      uint32_t length;
      uint32_t opcode;
    } m;
    if (read(s->fd, &m, sizeof(m)) != sizeof(m)) {
      props->badMessage = 1;
      return self->container.setError("can't read message header from file: %s",
				      strerror(errno));
    }
    n2read = m.length;
    port->output.u.operation = (RCCOpCode)m.opcode;
  }
  if (n2read > port->current.maxLength)
    return self->container.setError("message size (%zu) too large for max buffer size (%u)",
				    n2read, port->current.maxLength);
  if ((n = read(s->fd, port->current.data, n2read)) < 0)
    return self->container.setError("error reading file: %s", strerror(errno));
  if (props->messagesInFile && n != (ssize_t)n2read) {
    props->badMessage = 1;
    return self->container.setError("message truncated in file. header said %zu file had %zu",
				    n2read, n);
  }
  // Truncate the message for the granularity
  if (props->granularity)
    n -= n % props->granularity;
  if (self->crewSize > 1 && n) {
    // Skip to the next place this member should read
    if (lseek(s->fd, props->messageSize - n + props->messageSize * (self->crewSize - 1), SEEK_CUR) == -1)
      return self->container.setError("error skipping data in scaled worker");
  }
  port->output.length = n;
  props->bytesRead += n;
  if (n) {
    props->messagesWritten++;
    return RCC_ADVANCE;
  }
  if (props->repeat) {
    if (lseek(s->fd, self->member * props->messageSize, SEEK_SET) < 0)
      return self->container.setError("error rewinding file: %s", strerror(errno));
    return RCC_OK;
  }
  off_t 
    last = lseek(s->fd, 0, SEEK_CUR),
    end = lseek(s->fd, 0, SEEK_END);
  close(s->fd);
  if (props->suppressEOF)
    return RCC_DONE;
  if (last - end >= props->messageSize)
    return RCC_DONE;
  props->messagesWritten++;
  return RCC_ADVANCE_DONE;
}
