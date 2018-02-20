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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Jun 26 16:31:48 2017 EDT
 * BASED ON THE FILE: zero_padding.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the zero_pad worker in C++
 */

#include "zero_pad-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Zero_padWorkerTypes;

// create a template that uses DWIDTH_p to determine the type width for the
// internal buffer and the output buffer
template <int T> struct io_width_template     { /* no "type" defined for compile-time error */ };
template <>      struct io_width_template<8>  { typedef int8_t  type; };
template <>      struct io_width_template<16> { typedef int16_t type; };
template <>      struct io_width_template<32> { typedef int32_t type; };
template <>      struct io_width_template<64> { typedef int64_t type; };

typedef io_width_template<ZERO_PAD_DWIDTH_P>::type data_t;

class Zero_padWorker : public Zero_padWorkerBase {
  static const unsigned bytesPerSample = ZERO_PAD_DWIDTH_P/8;
  unsigned inputBufferIndex; // = 0;
public:
  Zero_padWorker() : inputBufferIndex(0) {}
private:
  RCCResult run(bool /*timedout*/) {
    const data_t *inData  = (const data_t*)in.data();
    data_t *outData = (data_t*)out.data();

    out.setOpCode(in.opCode());                                 // Set the metadata for the output message

    if (!in.length()){                                          //Pass along ZLMs
      out.setLength(0);
      return RCC_ADVANCE;
    }else{
      out.setLength((properties().num_zeros+1)*bytesPerSample); // resize output array
      if (inputBufferIndex < in.length() / bytesPerSample){     //for every sample of input
	*outData++ = inData[inputBufferIndex];
	for (unsigned b = 0; b < properties().num_zeros; b++){  //produce one output of length num_zeros
	  *outData++ = 0;
	}
	inputBufferIndex++;
	out.advance();
      }else{
	inputBufferIndex = 0;
	in.advance();	
      }
      return RCC_OK;
    }
  }
};

ZERO_PAD_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
ZERO_PAD_END_INFO
