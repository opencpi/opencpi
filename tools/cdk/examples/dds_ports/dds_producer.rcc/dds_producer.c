/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */
#include <stdio.h>
#include <string.h>
#include "dds_producer_Worker.h"
#include "../workerStruct.h"

typedef struct {
  uint32_t p_count;
  uint64_t d_count;
  uint32_t u1;
} State;

static uint32_t sizes[] = {sizeof(State), 0 };

DDS_PRODUCER_METHOD_DECLARATIONS;
RCCDispatch dds_producer = {
  /* insert any custom initializations here */
  .memSizes = sizes,
  DDS_PRODUCER_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */


static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  State *myState = self->memories[0];  
  Dds_producerProperties * props = (Dds_producerProperties*)self->properties;

  // We will run 10 times then delay to allow an external app to run and provide data to the consumer
  if ( myState->p_count++ >= props->msgs_to_produce )  {
    return RCC_OK;
  }

  RCCPort *out = &self->ports[DDS_PRODUCER_OUT];
  WorkerTestMsg * sample = (WorkerTestMsg *)out->current.data;
  memset( sample, 0, out->current.maxLength );
  uint8_t * off  = (uint8_t*)(&sample[1]);
  unsigned int n;
  int vi=0;
  int i;


  sample->userId = 1;
  sample->u1 = 445;
  sample->v_longlong1 = 98765432;
  sample->v_short  = 99;

  for (n=0; n<3; n++ ) {
    sample->v_l_array[n] = 20*n;
  }

  for (n=0; n<3; n++ ) {
    for (i=0; i<4; i++ ) {
      sample->v_l_array1[n][i] = vi++;
    }
  }

  sample->v_bool = 1;
  sample->v_double = 9876.543;

  for (n=0; n<5; n++ ) {
    sample->v_oct_array[n] = 20*n;
  }

  sample->v_long2 = 2;


  // EmbMsg structure
  {
    off = align( 8, off );
    uint32_t * value = (uint32_t*)off; off+=4;
    *value = 125689;
    off = align( 8, off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 4;
    off = align( 8, off );
    double * d = (double *)off;
    for (n=0; n<*length; n++ ) {
      d[n] = 44*n;
    }
    off += (*length * sizeof(double));
  }



  // string
  {
    off = align( 4,  off );
    char * data = (char*)off;
    strcpy( data, "Test Message from the producer worker");
    off += strlen(data) + 1;
  }


  // sequqence of longs
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 5;
    uint32_t * l = (uint32_t*)off;
    for (n=0; n<*length; n++ ) {
      l[n] = 20+n;      
    }
    off += 4*(*length);
  }


  // sequqence of doubles
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8,  off );
    *length = 6;
    double * l = (double*)off;
    for (n=0; n<*length; n++ ) {
      l[n] = 30+n;      
    }
    off += 8*(*length);
  }



  // sequqence of shorts
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 2,  off ); // not needed
    *length = 7;
    uint16_t * l = (uint16_t*)off;
    for (n=0; n<*length; n++ ) {
      l[n] = 40+n;      
    }
    off += 2*(*length);
  }


  // sequqence of chars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 8;
    int8_t * l = (int8_t*)off;
    for (n=0; n<*length; n++ ) {
      l[n] = n;      
    }
    off += 1*(*length);
  }



  // sequqence of uchars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 9;
    uint8_t * l = (uint8_t*)off;
    for (n=0; n<*length; n++ ) {
      l[n] = n+1;      
    }
    off += 1*(*length);
  }



  // sequqence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 10;  
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      sprintf(string, "string(%d) from producer", n );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }


  // Array of emb structs size = 3
  {
    vi=0;
    off = align( 8,  off );    
    int y;
    for (y=0; y<3; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * value = (uint32_t*)off; off+=4;
	*value = 2*y;
	off = align( 8,  off );
	uint32_t * length = (uint32_t*)off; off+=4;
	*length = 3;
	off = align( 8,  off );
	double * d = (double *)off;
	for (n=0; n<*length; n++ ) {
	  d[n] = vi++;
	}
	off += (*length * sizeof(double));
      }
    }
  }

  // Sequence of arrays
  {
    vi=0;
    unsigned int y;
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 4;
    off = align( 4,  off );
    for (y=0; y<*length; y++ ) {
      {
	uint32_t * l = (uint32_t *)off;
	for ( n=0; n<4; n++ ) {
	  l[n] = vi++;
	}
	off += 16;
      }
    }
  }


  // 2,3 array of sequences of embMsgs
  //  off+=4;
  {
    vi=0;
    off = align( 8, off );    
    int y,k;
    unsigned int uu;
    for ( k=0; k<2; k++ ) {
      for (y=0; y<3; y++ ) {

	uint32_t * slength = (uint32_t*)off; off+=4; 
	*slength = 2+y;
	
	// fill in the sequence
	for ( uu=0; uu<*slength; uu++ ) {
	  {
	    off = align( 8,  off );
	    uint32_t * value = (uint32_t*)off; off+=4;
	    *value = 5+uu;
	    off = align( 8,  off );
	    uint32_t * length = (uint32_t*)off; off+=4;
	    *length = 3+uu;
	    off = align( 8,  off );
	    double * d = (double *)off;
	    for (n=0; n<*length; n++ ) {
	      d[n] = vi++;
	    }
	    off += ((*length) * sizeof(double));
	  }
	}
      }
    }
  }


  off = align(4,off);
  uint32_t * enum1 = (uint32_t *)off;
  *enum1 = 2;
  off +=4;


  // sequqence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    *length = 3;  
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      sprintf(string, "string(%d) from producer, again", n );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }


  out->output.length = sizeof(WorkerTestMsg);
  return RCC_ADVANCE;
}
