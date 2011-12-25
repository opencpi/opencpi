/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */

#include <assert.h>
#include <string.h>
#include "dds_consumer_Worker.h"
#include "../workerStruct.h"

DDS_CONSUMER_METHOD_DECLARATIONS;
RCCDispatch dds_consumer = {
  /* insert any custom initializations here */
  DDS_CONSUMER_DISPATCH
};

/*
 * Methods to implement for worker hello, based on metadata.
 */

// The message we send all the time
#define MESSAGE "Hello, world\n"

static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *in = &self->ports[DDS_CONSUMER_IN];
  int n;

  pOf();

  WorkerTestMsg * sample = (WorkerTestMsg *)in->current.data;

  printf( "\n\nConsumer got a message\n");

  // Now for the fun, we need to manually calcualte the offsets and keep track of alignments
  uint8_t * off = (uint8_t*)(&sample[1]);
  //  uint8_t * orig = (uint8_t*)sample;

  printf( "  userId = %d\n", sample->userId);
  printf( "  u1 = %d\n", sample->u1 );
  printf( "  v_longlong1 = %lld\n", sample->v_longlong1 );
  printf( "  v_short = %d\n", (int)sample->v_short );

  for (n=0; n<3; n++ ) {
    printf("VLA = %d\n", sample->v_l_array[n] );
  }

  int i;
  for (n=0; n<3; n++ ) {
    for (i=0; i<4; i++ ) {
      printf("VLA1 = %d\n", sample->v_l_array1[n][i] );
    }
  }

  printf( "  v_bool = %s\n", sample->v_bool == 0 ? "false" : "true" );
  printf( "  v_double = %f\n", sample->v_double);

  for ( n=0; n<5; n++ ) {
    printf("  v_oct_array[%d] = %d", n, (int)sample->v_oct_array[n] );
  }
  printf( "\n");

  printf("v_long2 = %d\n", sample->v_long2 );


  // EmbMsg structure
  {
    off = align( 8, off );
    uint32_t * value = (uint32_t*)off; off+=4;
    printf("EmbMsg value = %d\n", *value );
    off = align( 8, off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8, off );
    double * d = (double *)off;
    for (n=0; n<*length; n++ ) {
      printf("EmbMsg double(%d) = %f\n", n, d[n] );
    }
    off +=*length * sizeof(double);
  }

    
  // string
  {
    off = align( 4, off );
    char * data = (char*)off;
    printf("Got this message: %s\n", data );
    off += strlen(data) + 1;
  }


  // sequence of longs
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    printf("sseq length = %d\n", *length );
      uint32_t * l = (uint32_t*)off;
    for (n=0; n<*length; n++ ) {
      printf("  v[%d] = %d\n", n,l[n] );
    }
    off += 4*(*length);
  }


  // sequence of doubles
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8,  off );
    printf("sseq length = %d\n", *length );
    double * l = (double*)off;
    for (n=0; n<*length; n++ ) {
      printf("  v[%d] = %f\n", n,l[n] );
    }
    off += 8*(*length);
  }



  // sequence of shorts
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 2,  off ); // not needed
    printf("sseq length = %d\n", *length );
    uint16_t * l = (uint16_t*)off;
    for (n=0; n<*length; n++ ) {
      printf("  v[%d] = %d\n", n, (uint32_t)l[n] );
    }
    off += 2*(*length);
  }


  // sequence of chars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    printf("sseq length = %d\n", *length );
    int8_t * l = (int8_t*)off;
    for (n=0; n<*length; n++ ) {
      printf("  v[%d] = %d\n", n, (uint32_t)l[n] );
    }
    off += 1*(*length);
  }



  // sequence of uchars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    printf("sseq length = %d\n", *length );
    uint8_t * l = (uint8_t*)off;
    for (n=0; n<*length; n++ ) {
      printf("  v[%d] = %d\n", n, (uint32_t)l[n] );
    }
    off += 1*(*length);
  }



  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    printf("sseq length = %d\n", *length );
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      printf(" string %d = %s\n", n, string );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }



  // Array of emb structs size = 3
  {
    printf("\n\nArray[3]\n");
    off = align( 8,  off );    
    int y;
    for (y=0; y<3; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * value = (uint32_t*)off; off+=4;
	printf("EmbMsg value = %d\n", *value );
	off = align( 8,  off );
	uint32_t * length = (uint32_t*)off; off+=4;
	off = align( 8,  off );
	double * d = (double *)off;
	for (n=0; n<*length; n++ ) {
	  printf("EmbMsg double(%d) = %f\n", n, d[n] );
	}
	off += (*length * sizeof(double));
      }
    }
  }

  // Sequence of arrays
  printf("\n Sequence of arrays\n");
  {
    int y;
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    printf("Sequence length = %d\n", *length );
    for (y=0; y<*length; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * l = (uint32_t *)off;
	for (n=0; n<4; n++ ) {
	  printf(" Value for a(%d) = %d\n", n, l[n] );
	}
	off += (4 * 4);
      }
    }
  }

  // 2,3 array of sequences of embMsgs
  {
    off = align( 8,  off );    
    int y,k,uu;
    for ( k=0; k<2; k++ ) {
      for (y=0; y<3; y++ ) {

	uint32_t * slength = (uint32_t*)off; off+=4; 
	printf("Seq len = %d\n", *slength );
	
	// fill in the sequence
	for ( uu=0; uu<*slength; uu++ ) {
	  {
	    off = align( 8,  off );
	    uint32_t * value = (uint32_t*)off; off+=4;
	    printf("  V = %d\n", *value );
	    off = align( 8,  off );
	    uint32_t * length = (uint32_t*)off; off+=4;
	    printf("    D seqlen = %d\n", *length );
	    off = align( 8,  off );
	    double * d = (double *)off;
	    for (n=0; n<*length; n++ ) {
	      printf(" d[%d] = %f\n", n, d[n]);
	    }
	    off += ((*length) * sizeof(double));
	  }
	}
      }
    }
  }

  off = align(4,off);
  uint32_t * enum1 = (uint32_t *)off;
  printf("Enum value = %d\n", *enum1 );
  off += 4;

  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    printf("sseq length = %d\n", *length );
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      printf(" string %d = %s\n", n, string );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }




  /*


  // Array[3] of sequences of longs 
  printf("\n Array[3] of long sequences\n");
  {
    int y;
    for (y=0; y<3; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * length = (uint32_t*)off; off+=4;
	printf(" Seq len = %d\n", *length );
	off = align( 4,  off );
	uint32_t * l = (uint32_t *)off;
	for (n=0; n<*length; n++ ) {
	  printf("  l[%d] = %d\n", n, l[n] );
	}
	off += (*length * 4);
      }
    }
  }

*/









  /*


  printf( "  long seq len = %d\n", sample->long_seq.length );
  assert(  sample->long_seq.length == long_seq_size );
  for ( n=0; n<sample->long_seq.length;  n++ ) {
    printf("    s[%d] = %d\n", n,sample->long_seq.data[n] );
  }


  printf( "  double seq len = %d\n", sample->double_seq.length );
  assert(  sample->double_seq.length == double_seq_size );
  for ( n=0; n<sample->double_seq.length;  n++ ) {
    printf("    s[%d] = %f\n", n,sample->double_seq.data[n] );
  }

  printf( "  short seq len = %d\n", sample->short_seq.length );
  assert(  sample->short_seq.length == short_seq_size );
  for ( n=0; n<sample->short_seq.length;  n++ ) {
    printf("    s[%d] = %d\n", n,(int)sample->short_seq.data[n] );
  }
  printf( "  uchar seq len = %d\n", sample->uchar_seq.length );
  assert(  sample->uchar_seq.length == uchar_seq_size );
  for ( n=0; n<sample->uchar_seq.length;  n++ ) {
    printf("    s[%d] = %d\n", n,(int)sample->uchar_seq.data[n] );
  }
  printf( "  char seq len = %d\n", sample->char_seq.length );
  assert(  sample->char_seq.length == char_seq_size );
  for ( n=0; n<sample->char_seq.length;  n++ ) {
    printf("    s[%d] = %d\n", n,(int)sample->char_seq.data[n] );
  }



  // Now for the fun, we need to manually calcualte the offsets and keep track of alignments
  uint8_t * orig = &sample->end_fixed;
  uint8_t * off = orig;
  
  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t length = (uint32_t*)*off; off+=4;
    printf("sseq length = %d\n", length );
    for (n=0; n<length; n++ ) {
      char * string = (char*)off;
      printf(" s(%d) = %s\n", n, string );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }


  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t length = (uint32_t*)*off; off+=4;
    printf("sseq1 length = %d\n", length );
    for (n=0; n<length; n++ ) {
      char * string = (char*)off;
      printf(" s(%d) = %s\n", n, string );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }



  // Array of emb structs size = 3
  {
    printf("\n\nArray[3]\n");
    off = align( 8,  off );    
    int y;
    for (y=0; y<3; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * value = (uint32_t*)off; off+=4;
	printf("EmbMsg value = %d\n", *value );
	off = align( 8,  off );
	uint32_t * length = (uint32_t*)off; off+=4;
	*length = 4;
	off = align( 8,  off );
	double * d = (double *)off;
	for (n=0; n<*length; n++ ) {
	  printf("EmbMsg double(%d) = %f\n", n, d[n] );
	}
	off += (*length * sizeof(double));
      }
    }
  }

  */


  /*

  // Array of longs size = 3
  printf("\n");
  {
    off = align( 4,  off );    
    int y;
    for (y=0; y<3; y++ ) {
      {
	uint32_t * value = (uint32_t*)off; off+=4;
	printf("Value = %d\n", *value );
      }
    }
  }
  */


  /*

  // Array of longs size = 5,2
  printf("\n");
  {
    off = align( 4,  off );    
    int y;
    for (y=0; y<10; y++ ) {
      {
	uint32_t * value = (uint32_t*)off; off+=4;
	printf("Value = %d\n", *value );
      }
    }
  }



  // Array of emb structs size = 3
  {
    printf("\n\nArray[4][3]\n");
    int b,y;
    for (b=0; b<4; b++ ) {
      for (y=0; y<3; y++ ) {
	{
	  off = align( 8,  off );
	  uint32_t * value = (uint32_t*)off; off+=4;
	  printf("EmbMsg value = %d\n", *value );
	  off = align( 8,  off );
	  uint32_t * length = (uint32_t*)off; off+=4;
	  off = align( 8,  off );
	  double * d = (double *)off;
	  for (n=0; n<*length; n++ ) {
	    printf("EmbMsg double(%d) = %f\n", n, d[n] );
	  }
	  off += ((*length) * sizeof(double));
	}
      }
    }
  }

  */



  return RCC_ADVANCE;
}
