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
#include <math.h>
#include "file_read_msg_Worker.h"
#include "stream_data_file_format.h"

typedef struct {
  int fd;
  int started;
  FileHeader header;
  int blcm;
} MyState;
static uint32_t mysizes[] = {sizeof(MyState), 0};

FILE_READ_MSG_METHOD_DECLARATIONS;
RCCDispatch file_read_msg = {
  /* insert any custom initializations here */
  FILE_READ_MSG_DISPATCH
  .memSizes = mysizes
};

/*
 * Methods to implement for worker file_read, based on metadata.
 */

#define DATA_SIZE 2048*20
#define SCALE_F 10000
#define PI 3.14159265
#define S_SIZE (DATA_SIZE/40)
RCCResult genTestFile( RCCWorker * self )
{
  File_read_msgProperties *p = self->properties;
  unsigned int n;
  double interval = 2.0*PI/S_SIZE;
  FileHeader h;
  int fd;
  unsigned bc=0;
  printf("Generating simple data file\n");
  if ((fd = creat(p->fileName, 0666)) < 0) {
    self->container.setError( "error opening file \"%s\": %s", p->fileName, strerror(errno));
    return RCC_ERROR;
  }

#ifdef DEBUG_PASS_THRU
  // Generate a text message
  {
    h.endian = 1;
    h.opcode = 3;
    char * cbuffer = "This is a test message for the real data stream";
    h.length = strlen( cbuffer ) + 1;

    bc+=sizeof(FileHeader);
    if (write(fd, &h, sizeof(FileHeader)) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }

    bc+= h.length;
    if (write(fd, cbuffer, h.length) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }
  }
#endif

  if ( p->genReal ) {
    // Generate a "real" sinwave
    h.endian = 1;
    h.opcode = 0;
    h.length = DATA_SIZE * sizeof(uint16_t);
    bc+=sizeof(FileHeader);
    if (write(fd, &h, sizeof(FileHeader)) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }
    uint16_t buffer[DATA_SIZE];
    for ( n=0; n<h.length / sizeof(uint16_t); n++ ) {
      buffer[n] = (uint16_t) ( SCALE_F * sin( interval * n ));
    }
    bc+= h.length;
    if (write(fd, buffer, h.length) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }
  }
  else {

    // Generate a "complex" sinwave
    h.endian = 1;
    h.opcode = 0;
    h.length = DATA_SIZE * sizeof(uint16_t) * 2;
    bc+=sizeof(FileHeader);
    if (write(fd, &h, sizeof(FileHeader)) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }
    uint16_t buffer[DATA_SIZE*2];
    for ( n=0; n<h.length / (sizeof(uint16_t)*2); n+=2 ) {
      buffer[n] = (uint16_t) ( SCALE_F * sin( interval * n ));
      buffer[n+1] = (uint16_t) ( SCALE_F * cos( interval * n ));
    }
    bc+= h.length;
    if (write(fd, buffer, h.length) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }


  }

#ifdef DEBUG_PASS_THRU
  // Generate a text message
  {
    h.endian = 1;
    h.opcode = 3;
    char * cbuffer = "This is another test message for the real data stream";
    h.length = strlen( cbuffer ) + 1;
    bc+= sizeof(FileHeader);
    if (write(fd, &h, sizeof(FileHeader)) < 0) {
      self->container.setError("error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }
    bc+= h.length;
    if (write(fd, cbuffer, h.length) < 0) {
      self->container.setError( "error reading file: %s", strerror(errno));
      return RCC_ERROR;
    }
  }
#endif


  close(fd);
  return RCC_OK;
}
 

static RCCResult
start(RCCWorker *self) {
  MyState *s = self->memories[0];
  File_read_msgProperties *p = self->properties;
  if (s->started) {
    self->container.setError("file_read cannot be restarted");
    return RCC_ERROR;
  }

  // A conveinience function to generates a simple sinwave file
  if ( p->genTestFile ) {
    genTestFile( self );
    p->genTestFile = 0;
  }

  s->started = 1;
  if ((s->fd = open(p->fileName, O_RDONLY)) < 0) {
    self->container.setError( "error opening file \"%s\": %s", p->fileName, strerror(errno));
    return RCC_ERROR;
  }
  p->finished = 0;
  return RCC_OK;
}


static RCCResult
retDone( RCCBoolean cont ) 
{
  if ( ! cont ) {
    return RCC_DONE;
  }
  else {
    return RCC_ADVANCE;
  }
}


static RCCResult
run(RCCWorker *self, RCCBoolean timedOut, RCCBoolean *newRunCondition) {
 RCCPort *port = &self->ports[FILE_READ_MSG_OUT];
 File_read_msgProperties *props = self->properties;
 MyState *s = self->memories[0];
 size_t n2read =  props->messageSize ? props->messageSize : port->current.maxLength;
 ssize_t n, readl;


 if ( props->stepThruMsg ) {
   if ( ! props->stepNow ) {
     return RCC_OK;
   }
   props->stepNow = 0;
 }

 (void)timedOut;(void)newRunCondition;
 if (props->messageSize > port->current.maxLength) {
   self->container.setError( "message size property too large for buffers" );
   return RCC_ERROR;
 }
 if (props->granularity) {
   n2read -= n2read % props->granularity;
 }

 if ( s->blcm == 0 ) {
   if ((n = read(s->fd, &s->header, sizeof(FileHeader))) < 0) {
     self->container.setError( "error reading file: %s", strerror(errno));
     return RCC_ERROR;
   } 
   if ( n == 0 ) {
     printf("file_reader_msg: Finished !!\n");
     props->finished = 1;
     return retDone( props->continuous );
   }
   printf("file_reader_msg(%s): Data length = %d\n", props->fileName, s->header.length );
   s->blcm = s->header.length;
 }
 self->ports[FILE_READ_MSG_OUT].output.u.operation = s->header.opcode;

 // printf("port max len = %d\n", port->current.maxLength);
 
 readl = (port->current.maxLength > s->header.length) ?  s->header.length : port->current.maxLength;
 if ((n = read(s->fd, port->current.data, readl )) < 0) {
   self->container.setError( "error reading file: %s", strerror(errno));
   return RCC_ERROR;
 } 
 if ( n == 0 ) {
   return retDone( props->continuous );
 }
 //printf("In file_read_msg.c got data = %s\n", port->current.data);


 // printf("sending %d bytes on file read\n", n);

 port->output.length = n;
 props->bytesRead += n;
 s->blcm -= n;
 if (n) {
   props->messagesWritten++;
   return RCC_ADVANCE;
 }
 close(s->fd);

 // If the continuous flag is set, send the last buffer of data forever
 if ( props->continuous ) {
   return RCC_ADVANCE;
 }
 return RCC_ADVANCE_DONE;
}

