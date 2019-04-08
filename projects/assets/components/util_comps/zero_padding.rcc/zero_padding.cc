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

/*
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jun 20 12:15:14 2014 EDT
 * BASED ON THE FILE: zero_padding.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the zero_padding worker in C++
 *
 * The Zero Padding worker inputs bits, expands the bits to signed Qm.n samples within
 * the range of +/- 1, and inserts num_zeros zeros between each output sample. The
 * ODATA_WIDTH_p parameter defines the number of bits in each output sample. Valid
 * output sizes are signed 8/16/32/64 bit samples.
 *
 * Because the output size is ODATA_WIDTH_p * (num_zeros+1) for each input bit an internal
 * buffer is used to hold data from a fully processed input buffer. Each time the
 * worker is called a maximum of out.length bytes are copied from the internal
 * buffer to the output buffer. The input buffer is only advanced once the internal
 * buffer has been emptied. The num_zeros property is evaluated every time the internal
 * buffer has been emptied to determine if the internal buffer needs to be resized.
 */

/* DEPRECATION NOTICE: This worker is deprecated and will be removed in OpenCPI 2.0. Use the Zero Pad component for new designs. */

#include "zero_padding-worker.hh"
#include <cstdio>
#include <cstring>
#include <limits>
#include <sys/time.h>


using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Zero_paddingWorkerTypes;

// create a template that uses ODATA_WIDTH_p to determine the type width for the
// internal buffer and the output buffer
// this template also uses IDATA_WIDTH_p to determine how to process input bytes
template <int T> struct io_width_template     { /* no "type" defined for compile-time error */ };
template <>      struct io_width_template<8>  { typedef int8_t  type; };
template <>      struct io_width_template<16> { typedef int16_t type; };
template <>      struct io_width_template<32> { typedef int32_t type; };
template <>      struct io_width_template<64> { typedef int64_t type; };

typedef io_width_template<ZERO_PADDING_IDATA_WIDTH_P>::type input_t;
typedef io_width_template<ZERO_PADDING_ODATA_WIDTH_P>::type output_t;


class Zero_paddingWorker : public Zero_paddingWorkerBase {
  static const unsigned int bitsPerByte = 8;
  static const unsigned int bytesPerSample = ZERO_PADDING_ODATA_WIDTH_P / bitsPerByte;
  unsigned int myNumZeros;
  long unsigned int myBufferIndex; // How much data has been processed.
  output_t *myBuffer; // Temporary buffer for holding processed data
  struct timeval tv1, tv2;
  bool endFlag;
  
  long unsigned int myBufferSize; // The current size of myBuffer
  bool lastBufferComplete;  
  long unsigned int lastBufferSize; // THe last size of myBuffer

  RCCResult start ()
  {
    myNumZeros = 0;
    myBufferIndex = 0;
    myBuffer = NULL;
    endFlag = true;
    myBufferSize = 0;
    lastBufferComplete = true;    
    lastBufferSize = 0;
    return RCC_OK;
  }


  RCCResult stop ()
  {
    printf("Elapsed time = %f seconds\n", (double)(tv2.tv_usec - tv1.tv_usec) / 1000000 + (double)(tv2.tv_sec - tv1.tv_sec));
    delete [] myBuffer;
    return RCC_OK;
  }


  // process input bits and store in an internal buffer since it will likely be much
  // larger than each output buffer
  void processData(void)
  {
    const input_t *inData = (const input_t*)in.data(); // process an IDATA_WIDTH_p sample at a time
    uint8_t mask;
    uint8_t byte_index = 0;
    input_t currentData;
    long unsigned int writeIndex = 0;
    
    // process an input byte at a time
    for (unsigned int i = 0; i < in.length(); i++) // in.length is in bytes
    {
      mask = 0x80;      

      if (ZERO_PADDING_IDATA_WIDTH_P == 64)
      {
        if (byte_index == 0)
        {
          currentData = (uint8_t)((*inData & 0xFF00000000000000) >> 56);
          byte_index++;
        }
        else if (byte_index == 1)
        {
          currentData = (uint8_t)((*inData & 0xFF000000000000) >> 48);
          byte_index++;
        }
        else if (byte_index == 2)
        {
          currentData = (uint8_t)((*inData & 0xFF0000000000) >> 40);
          byte_index++;
        }
        else if (byte_index == 3)
        {
          currentData = (uint8_t)((*inData & 0xFF00000000) >> 32);
          byte_index++;
        }
        else if (byte_index == 4)
        {
          currentData = (uint8_t)((*inData & 0xFF000000) >> 24);
          byte_index++;
        }
        else if (byte_index == 5)
        {
          currentData = (uint8_t)((*inData & 0xFF0000) >> 16);
          byte_index++;
        }
        else if (byte_index == 6)
        {
          currentData = (uint8_t)((*inData & 0xFF00) >> 8);
          byte_index++;
        }
        else
        {
          currentData = (uint8_t)(*inData & 0xFF);
          byte_index = 0;
          inData++;
        }
      }
      else if (ZERO_PADDING_IDATA_WIDTH_P == 32)
      {
        if (byte_index == 0)
        {
          currentData = (uint8_t)((*inData & 0xFF000000) >> 24);
          byte_index++;
        }
        else if (byte_index == 1)
        {
          currentData = (uint8_t)((*inData & 0xFF0000) >> 16);
          byte_index++;
        }
        else if (byte_index == 2)
        {
          currentData = (uint8_t)((*inData & 0xFF00) >> 8);
          byte_index++;
        }
        else
        {
          currentData = (uint8_t)(*inData & 0xFF);
          byte_index = 0;
          inData++;
        }
      }
      else if (ZERO_PADDING_IDATA_WIDTH_P == 16)
      {
        if (byte_index == 0)
        {
          currentData = (uint8_t)((*inData & 0xFF00) >> 8);
          byte_index++;
          //printf("low data is: %x\n", currentData);
        }
        else
        {
          currentData = (uint8_t)(*inData & 0xFF);
          byte_index = 0;
          inData++;
          //printf("high data is: %x\n", currentData);
        }
      }
      else if (ZERO_PADDING_IDATA_WIDTH_P == 8)
      {
        currentData = *inData;
        inData++;
      }

      for (unsigned int j = 0; j < bitsPerByte; j++)
      {
        if ((currentData & mask) != 0)
        {
          myBuffer[writeIndex++] = std::numeric_limits<output_t>::max(); // ~+1 is the max Qm.n value
        }
        else
        {
          myBuffer[writeIndex++] = std::numeric_limits<output_t>::min(); // -1 is the min Qm.n value
        }

        unsigned int myCounter = myNumZeros;

        while(myCounter > 0)
        {
          myBuffer[writeIndex++] = 0; // insert zero samples
          myCounter--;
        }
        mask = mask >> 1;
      }
    }
  }


  RCCResult run(bool /*timedout*/)
  {
    if (myNumZeros == 0)  // initial buffer only
    {
      gettimeofday(&tv1, NULL);
      myNumZeros = properties().num_zeros;
    }
    //printf("myBufferSize = %lu samples\n", myBufferSize);

    // if the previous myBuffer was completely processed, or the first time, grab new input data
    // myBufferIndex should go from 0 to N-1 for the buffer addresses. When myBufferIndex=N it is a flag
    // to grab new input data
    if ( lastBufferComplete ) // initial value is true so it covers first time through.
    {
        
      myNumZeros = properties().num_zeros; // Update myNumZeros, it may have stayed the same.
      // Update the buffer size
      myBufferSize = in.length() * bitsPerByte * (myNumZeros+1);
      if ( ( myBufferSize!= lastBufferSize ) ) // the first time thru
      {
          if( myBuffer ){              
            delete [] myBuffer;        
          }
          myBuffer = new output_t[myBufferSize];
          lastBufferSize = myBufferSize;
      }           
      lastBufferComplete = false;
      processData();
      myBufferIndex = 0;
    }

    // populate the output buffer
    output_t *outData = (output_t*)out.data();
    if (myBufferSize != 0)  // will be zero when in.length is zero - no more input data
    {
      //printf("about to populate the output buffer\n");
      //printf("myBufferSize = %lu, myBufferIndex = %lu, out.length = %lu bytes\n", myBufferSize, myBufferIndex, out.length());
      // what's left in myBuffer is <= the size of the output buffer
      unsigned int copyAmount = bytesPerSample * (myBufferSize- myBufferIndex); // Number of bytes to copy
      if ( copyAmount <= out.length() ) //out.length is in bytes
      {
        //printf("Populating last output buffer\n");
        memcpy(outData, &myBuffer[myBufferIndex], copyAmount);       
        out.setLength( copyAmount );
        myBufferIndex = myBufferSize;
        lastBufferComplete = true;
        return RCC_ADVANCE;
      } else {           
        //printf("Populating output buffer\n");
        // grab the next chunk of output-buffer-size data from myBuffer
        // Have enough to fill the output and some so send all we can
        copyAmount = out.length(); 
	
        memcpy(outData, &myBuffer[myBufferIndex], copyAmount); // Byte copy
        myBufferIndex += copyAmount / bytesPerSample; //out.length is in bytes        
        out.setLength(copyAmount);
        out.advance();
        return RCC_OK;
      }
    }
    else
    {
      if (endFlag)
      {
        gettimeofday(&tv2, NULL);
        endFlag = false;
      }
      //propagate zero-length message
      out.setLength(0);
      out.advance();
      return RCC_OK;
    }
  }
};

ZERO_PADDING_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
ZERO_PADDING_END_INFO
