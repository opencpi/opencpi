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
  uint64_t fileLength;
  off_t nextSeek;
} MyState;
static size_t mysizes[] = {sizeof(MyState), 0};

// We will run when we get a data message, OR an EOF message
static uint32_t myPortMasks[] = {(1<<FILE_WRITE_IN) | (1 << FILE_WRITE_EOF_OUT),
				 1<<FILE_WRITE_EOF_IN, 0 };
static RCCRunCondition myRunCondition = {myPortMasks, 0, 0};

FILE_WRITE_METHOD_DECLARATIONS;
RCCDispatch file_write = {
 /* insert any custom initializations here */
 FILE_WRITE_DISPATCH
 .runCondition = &myRunCondition,
 .memSizes = mysizes
};

static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memories[0];
  File_writeProperties *p = self->properties;

  if (s->started)
    return self->container.setError("file_read cannot be restarted");
  s->started = 1;
  if ((s->fd = creat(p->fileName, 0666)) < 0)
    return self->container.setError("error creating file \"%s\": %s",
				    p->fileName, strerror(errno));
  if (self->crewSize > 1) {
    if (!p->messageSize)
      return
	self->container.setError("worker can only be scaled if a message size is specified");
    if ((s->nextSeek =
	 lseek(s->fd, self->member * p->messageSize, SEEK_SET)) == -1)
      return
	self->container.setError("error seeking file \"%s\": %s", p->fileName, strerror(errno));
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
  RCCPort
    *data = &self->ports[FILE_WRITE_IN],
    *eof_in = &self->ports[FILE_WRITE_EOF_IN];
  File_writeProperties *props = self->properties;
  MyState *s = self->memories[0];
  uint64_t fileLength;

  (void)timedOut;(void)newRunCondition;
  if (eof_in->current.data) {
    // In the scaled case grab the file length determined by whichever member got the EOF
    s->fileLength = *(uint64_t*)eof_in->current.data;
    self->container.advance(eof_in, 0);
    //printf("Member %zu got FILELENGTH: %llu %llu\n", self->member, s->fileLength, s->nextSeek);
  }
  if (s->fileLength && (uint64_t)s->nextSeek >= s->fileLength)
    return RCC_DONE;
  if (!data->current.data)
    return RCC_OK;
  ssize_t n = data->input.length;
  if (props->messagesInFile) {
    struct {
      uint32_t length;
      uint32_t opcode;
    } m;
    m.length = n;
    m.opcode = data->input.u.operation;
    if (write(s->fd, &m, sizeof(m)) < 0)
      return self->container.setError("error writing header to file: %s", strerror(errno));
  }
#if 0
  printf("writer %zu of %zu others %zu length %zu written %zu pos %zu data %x\n",
	 self->member, self->crewSize, data->connectedCrewSize, data->input.length,
	 (size_t)props->messagesWritten, (size_t)lseek(s->fd, 0, SEEK_CUR),
	 *(uint32_t*)(data->current.data));
#endif
  if (self->crewSize > 1 && n < props->messageSize)
    fileLength = (size_t)lseek(s->fd, 0, SEEK_CUR) + n;
  if (n) {
    if ((n = write(s->fd, data->current.data, n)) < 0 ||
	(size_t)n != data->input.length)
      return self->container.setError("error writing data to file: %s", strerror(errno));
    props->bytesWritten += n;
    props->messagesWritten++;
#if 0
    printf("written %zu others %zu crew %zu ms %zu skip %zu\n", n,
	   data->connectedCrewSize, self->crewSize, (size_t)props->messageSize,
	   data->connectedCrewSize * (self->crewSize - 1) * props->messageSize);
#endif
    if (self->crewSize > 1 && n >= props->messageSize && 
	//	props->messagesWritten % data->connectedCrewSize == 0 &&
	(s->nextSeek =
	 lseek(s->fd, (self->crewSize - 1) * props->messageSize, SEEK_CUR)) == -1)
      return
	self->container.setError("scaled seeking output file failed: %s", strerror(errno));
  }
  self->container.advance(data, 0);
  if (s->fileLength && (uint64_t)s->nextSeek >= s->fileLength)
    return RCC_DONE;
  if (self->crewSize > 1 && n < props->messageSize) {
    RCCPort *eof_out = &self->ports[FILE_WRITE_EOF_OUT];
    *(uint64_t*)eof_out->current.data = s->fileLength = fileLength;
    eof_out->output.length = sizeof(uint64_t);
    self->container.advance(eof_out, 0);
    //printf("Member %zu sending FILELENGTH: %llu\n", self->member, s->fileLength);
    return RCC_DONE;
  }
  if (n == 0 && props->stopOnEOF)
    return RCC_DONE;
  return RCC_OK;
}
