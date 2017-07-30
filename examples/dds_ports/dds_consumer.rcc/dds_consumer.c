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

#define NDEBUG 1

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon May  9 07:20:08 2011 EDT
 * BASED ON THE FILE: hello.xml
 * YOU ARE EXPECTED TO EDIT IT
 *
 * This file contains the RCC implementation skeleton for worker: hello
 */

#include <stdio.h>
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

#define MIDO "OCPI MESSAGE"
#define MIDDDS "DDS MESAGE"

#define RET( id, member ) \
  { \
    printf("Message from %s failed at member %s\n", id, #member );	\
  return 0; \
  }

int validate_ddsmsg( WorkerTestMsg * sample  )
{

  uint8_t * off = (uint8_t*)(&sample[1]);
  unsigned int n;
  int in;
  int i;
  int vi=0;
  
  if ( sample->userId != 2 ) RET( MIDDDS, userId );
  if ( sample->u1 != 333 ) RET( MIDDDS, u1 );
  if ( sample->v_longlong1 != 97531246 ) RET( MIDDDS, v_longlong1 );
  if ( sample->v_short  != 67 ) RET( MIDDDS, v_short );
  for (in=0; in<3; in++ ) {
    if ( sample->v_l_array[in] != in+11 ) RET( MIDDDS, v_l_array );
  }
  for (n=0; n<3; n++ ) {
    for (i=0; i<4; i++ ) {
      if ( sample->v_l_array1[n][i] != (long)10+n+i ) RET( MIDDDS, v_l_array1 );
    }
  }
  if ( sample->v_bool != 0 ) RET( MIDDDS, v_bool );
  if ( sample->v_double != 1234.9876 ) RET( MIDDDS, v_double );
  for (n=0; n<5; n++ ) {
    if ( sample->v_oct_array[n] != 32+n ) RET( MIDDDS, v_oct_array );
  }
  if ( sample->v_long2 != 56 ) RET( MIDDDS, v_long2 );


  // EmbMsg structure
  {
    off = align( 8, off );
    uint32_t * value = (uint32_t*)off; off+=4;
    if ( *value != 7 ) RET( MIDDDS, embMsg_value );
    off = align( 8, off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8, off );
    double * d = (double *)off;
    if ( *length != 4 ) RET( MIDDDS, embMsg_length );
    for (n=0; n<*length; n++ ) {
      if ( d[n] != 10*n ) RET( MIDDDS, embMsg_data );
    }
    off +=*length * sizeof(double);
  }

    
  // string
  const char * ts = "Hello OCPI  World, from DDS   ";
  {
    off = align( 4, off );
    char * data = (char*)off;
    if ( strcmp( ts, data ) != 0 ) RET( MIDDDS, string );
    off += strlen(data) + 1;
  }


  // sequence of longs
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 5 ) RET( MIDDDS, long_seq_length );
    uint32_t * l = (uint32_t*)off;
    for (n=0; n<*length; n++ ) {
      if ( l[n] != 10*n ) RET( MIDDDS, long_seq );
    }
    off += 4*(*length);
  }


  // sequence of doubles
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8,  off );
    if ( *length != 12 ) RET( MIDDDS, double_seq_length );
    double * l = (double*)off;
    for (n=0; n<*length; n++ ) {
      if (  l[n] != n*11 ) RET( MIDDDS, double_seq );
    }
    off += 8*(*length);
  }



  // sequence of shorts
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 2,  off ); // not needed
    if ( *length != 4 ) RET( MIDDDS, short_seq_length );
    uint16_t * l = (uint16_t*)off;
    for (n=0; n<*length; n++ ) {
      if ( l[n] != n*10 ) RET( MIDDDS, short_seq );
    }
    off += 2*(*length);
  }


  // sequence of chars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 8 ) RET( MIDDDS, char_seq_length );
    int8_t * l = (int8_t*)off;
    for (n=0; n<*length; n++ ) {
      if (  l[n] != (int8_t)(n*10) ) RET( MIDDDS, short_seq );
    }
    off += 1*(*length);
  }



  // sequence of uchars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 12 ) RET( MIDDDS, uchar_seq_length );
    uint8_t * l = (uint8_t*)off;
    for (n=0; n<*length; n++ ) {
      if ( l[n] != n*10 ) RET( MIDDDS, uchar_seq );
    }
    off += 1*(*length);
  }



  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 13 ) RET( MIDDDS, string_seq_length );
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      char tstring[1024];
      sprintf(tstring, "DDS Message %d", n );
      if ( strcmp( string, tstring) != 0 )  RET( MIDDDS, string_seq );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }



  // Array of emb structs size = 3
  {
    vi = 0;
    off = align( 8,  off );    
    int y;
    for (y=0; y<3; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * value = (uint32_t*)off; off+=4;
	if ( *value != (uint32_t)y ) RET( MIDDDS, array_embmsg_value );
	off = align( 8,  off );
	uint32_t * length = (uint32_t*)off; off+=4;	
	if ( *length != (uint32_t)(y+2) ) RET( MIDDDS, array_embmsg_seq_length );
	off = align( 8,  off );
	double * d = (double *)off;
	for (n=0; n<*length; n++ ) {
	  if ( d[n] != n*10+y ) RET( MIDDDS, array_embmsg_seq_value );
	}
	off += (*length * sizeof(double));
      }
    }
  }

  // Sequence of arrays
  {
    vi=25;
    unsigned int y;
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 4 ) RET( MIDDDS, seq_long_arrays_length );
    for (y=0; y<*length; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * l = (uint32_t *)off;
	for (n=0; n<4; n++ ) {
	  if ( l[n] != (uint32_t)vi++ ) RET( MIDDDS, seq_long_arrays );
	}
	off += (4 * 4);
      }
    }
  }

  // 2,3 array of sequences of embMsgs
  {
    vi=37;
    off = align( 8,  off );    
    int y,k;
    unsigned int uu;
    for ( k=0; k<2; k++ ) {
      for (y=0; y<3; y++ ) {

	uint32_t * slength = (uint32_t*)off; off+=4; 
	if ( *slength != (uint32_t)(2+k+y) ) RET( MIDDDS, array_seq_embmsg_length );
	
	// fill in the sequence
	for ( uu=0; uu<*slength; uu++ ) {
	  {
	    off = align( 8,  off );
	    uint32_t * value = (uint32_t*)off; off+=4;
	    if ( *value != (uint32_t)vi++ ) RET( MIDDDS, array_seq_embmsg_value );
	    off = align( 8,  off );
	    uint32_t * length = (uint32_t*)off; off+=4;
	    if ( *length != (uint32_t)(4+y) ) RET( MIDDDS, array_seq_embmsg_seq_length );
	    off = align( 8,  off );
	    double * d = (double *)off;
	    for (n=0; n<*length; n++ ) {
	      if ( d[n] != vi+10 ) RET( MIDDDS, array_seq_embmsg_seq_value );
	    }
	    off += ((*length) * sizeof(double));
	  }
	}
      }
    }
  }

  off = align(4,off);
  uint32_t * enum1 = (uint32_t *)off;
  if ( *enum1 != 1 ) RET( MIDDDS, emum );  
  off += 4;

  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 3 ) RET( MIDDDS, seq_strings2 );      
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      char tstring[1024];
      sprintf(tstring, "Hello OCPI  World, from DDS %d", n );
      if ( strcmp( tstring,string) != 0 )  RET( MIDDDS, seq_strings2 );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }

  return 1;
}



int validate_ocpimsg( WorkerTestMsg * sample )
{

  uint8_t * off = (uint8_t*)(&sample[1]);
  unsigned int n;
  int in;
  int i;
  int vi=0;
  
  if ( sample->userId != 1 ) RET( MIDO, userId );
  if ( sample->u1 != 445 ) RET( MIDO, u1 );
  if ( sample->v_longlong1 != 98765432 ) RET( MIDO, v_longlong1 );
  if ( sample->v_short  != 99 ) RET( MIDO, v_short );
  for (in=0; in<3; in++ ) {
    if ( sample->v_l_array[in] != 20*in ) RET( MIDO, v_l_array );
  }
  for (n=0; n<3; n++ ) {
    for (i=0; i<4; i++ ) {
      if ( sample->v_l_array1[n][i] != vi++ ) RET( MIDO, v_l_array1 );
    }
  }
  if ( sample->v_bool != 1 ) RET( MIDO, v_bool );
  if ( sample->v_double != 9876.543 ) RET( MIDO, v_double );
  for (n=0; n<5; n++ ) {
    if ( sample->v_oct_array[n] != 20*n ) RET( MIDO, v_oct_array );
  }
  if ( sample->v_long2 != 2 ) RET( MIDO, v_long2 );


  // EmbMsg structure
  {
    off = align( 8, off );
    uint32_t * value = (uint32_t*)off; off+=4;
    if ( *value != 125689 ) RET( MIDO, embMsg_value );
    off = align( 8, off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8, off );
    double * d = (double *)off;
    if ( *length != 4 ) RET( MIDO, embMsg_length );
    for (n=0; n<*length; n++ ) {
      if ( d[n] != 44*n ) RET( MIDO, embMsg_data );
    }
    off +=*length * sizeof(double);
  }

    
  // string
  const char * ts = "Test Message from the producer worker";
  {
    off = align( 4, off );
    char * data = (char*)off;
    if ( strcmp( ts, data ) != 0 ) RET( MIDO, string );
    off += strlen(data) + 1;
  }


  // sequence of longs
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 5 ) RET( MIDO, long_seq_length );
    uint32_t * l = (uint32_t*)off;
    for (n=0; n<*length; n++ ) {
      if ( l[n] != 20+n ) RET( MIDO, long_seq );
    }
    off += 4*(*length);
  }


  // sequence of doubles
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 8,  off );
    if ( *length != 6 ) RET( MIDO, double_seq_length );
    double * l = (double*)off;
    for (n=0; n<*length; n++ ) {
      if (  l[n] != 30+n ) RET( MIDO, double_seq );
    }
    off += 8*(*length);
  }



  // sequence of shorts
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    off = align( 2,  off ); // not needed
    if ( *length != 7 ) RET( MIDO, short_seq_length );
    uint16_t * l = (uint16_t*)off;
    for (n=0; n<*length; n++ ) {
      if ( l[n] != 40+n ) RET( MIDO, short_seq );
    }
    off += 2*(*length);
  }


  // sequence of chars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 8 ) RET( MIDO, char_seq_length );
    int8_t * l = (int8_t*)off;
    for (n=0; n<*length; n++ ) {
      if (  l[n] != (int8_t)n ) RET( MIDO, short_seq );
    }
    off += 1*(*length);
  }



  // sequence of uchars
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 9 ) RET( MIDO, uchar_seq_length );
    uint8_t * l = (uint8_t*)off;
    for (n=0; n<*length; n++ ) {
      if ( l[n] != n+1 ) RET( MIDO, uchar_seq );
    }
    off += 1*(*length);
  }



  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 10 ) RET( MIDO, string_seq_length );
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      char tstring[1024];
      sprintf(tstring, "string(%d) from producer", n );
      if ( strcmp( string, tstring) != 0 )  RET( MIDO, string_seq );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }



  // Array of emb structs size = 3
  {
    vi = 0;
    off = align( 8,  off );    
    int y;
    for (y=0; y<3; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * value = (uint32_t*)off; off+=4;
	if ( *value != (uint32_t)(2*y) ) RET( MIDO, array_embmsg_value );
	off = align( 8,  off );
	uint32_t * length = (uint32_t*)off; off+=4;	
	if ( *length != 3 ) RET( MIDO, array_embmsg_seq_length );
	off = align( 8,  off );
	double * d = (double *)off;
	for (n=0; n<*length; n++ ) {
	  if ( d[n] != vi++ ) RET( MIDO, array_embmsg_seq_value );
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
    if ( *length != 4 ) RET( MIDO, seq_long_arrays_length );
    for (y=0; y<*length; y++ ) {
      {
	off = align( 4,  off );
	uint32_t * l = (uint32_t *)off;
	for (n=0; n<4; n++ ) {
	  if ( l[n] != (uint32_t)vi++ ) RET( MIDO, seq_long_arrays );
	}
	off += (4 * 4);
      }
    }
  }

  // 2,3 array of sequences of embMsgs
  {
    vi=0;
    off = align( 8,  off );    
    int y,k;
    unsigned int uu;
    for ( k=0; k<2; k++ ) {
      for (y=0; y<3; y++ ) {

	uint32_t * slength = (uint32_t*)off; off+=4; 
	if ( *slength != (uint32_t)(2+y) ) RET( MIDO, array_seq_embmsg_length );
	
	// fill in the sequence
	for ( uu=0; uu<*slength; uu++ ) {
	  {
	    off = align( 8,  off );
	    uint32_t * value = (uint32_t*)off; off+=4;
	    if ( *value != 5+uu ) RET( MIDO, array_seq_embmsg_value );
	    off = align( 8,  off );
	    uint32_t * length = (uint32_t*)off; off+=4;
	    if ( *length != 3+uu ) RET( MIDO, array_seq_embmsg_seq_length );
	    off = align( 8,  off );
	    double * d = (double *)off;
	    for (n=0; n<*length; n++ ) {
	      if ( d[n] != vi++ ) RET( MIDO, array_seq_embmsg_seq_value );
	    }
	    off += ((*length) * sizeof(double));
	  }
	}
      }
    }
  }

  off = align(4,off);
  uint32_t * enum1 = (uint32_t *)off;
  if ( *enum1 != 2 ) RET( MIDO, emum );  
  off += 4;

  // sequence of strings
  {
    off = align( 4,  off );
    uint32_t * length = (uint32_t*)off; off+=4;
    if ( *length != 3 ) RET( MIDO, seq_strings2 );      
    for (n=0; n<*length; n++ ) {
      char * string = (char*)off;
      char tstring[1024];
      sprintf(tstring, "string(%d) from producer, again", n );
      if ( strcmp( tstring,string) != 0 )  RET( MIDO, seq_strings2 );
      off += strlen(string) + 1;
      off = align( 4,  off );    
    }
  }

  return 1;

}

int validate( WorkerTestMsg * sample, Dds_consumerProperties * props )
{
  int ret;
  if ( sample->userId == 1 ) {
    props->ocpi_msgs++;
    printf( "\nConsumer got a DDS message from OCPI\n");
    ret = validate_ocpimsg( sample );
    props->ocpi_good_msgs += ret;
    return ret;
  }
  else if (  sample->userId == 2 ) {
    props->dds_msgs++;
    printf( "\nConsumer got a DDS message from a standalone DDS publisher\n");
    ret = validate_ddsmsg( sample );
    props->dds_good_msgs += ret;
    return ret;    
  }
  else {
    printf("Unknown user sent us a message\n");
    return 0;
  }
  return 0;
}



void printSample( WorkerTestMsg * sample )
{
  unsigned int n;
  uint8_t * off = (uint8_t*)(&sample[1]);

  printf( "  userId = %d\n", sample->userId);
  printf( "  u1 = %d\n", sample->u1 );
  printf( "  v_longlong1 = %lld\n", (long long)sample->v_longlong1 );
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
    unsigned int y;
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
    int y,k;
    unsigned int uu;
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
  }



static RCCResult
run(RCCWorker *self, RCCBoolean timeout, RCCBoolean *newRunCondition) {
  (void)timeout;(void)newRunCondition;
  RCCPort *in = &self->ports[DDS_CONSUMER_IN];
  Dds_consumerProperties * props = (Dds_consumerProperties*)self->properties;

  WorkerTestMsg * sample = (WorkerTestMsg *)in->current.data;

  if ( ! validate( sample, props )  ) {
    printf("ERROR: data returned from DDS topic failed validation !!\n");
   
  }
  else {
    printf("Data validation passed\n");
  }



#ifndef NDEBUG
  printData( sample );
#endif

  return RCC_ADVANCE;
}
