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

#include <math.h>
#include "mfsk_mapper-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Mfsk_mapperWorkerTypes;

class Mfsk_mapperWorker : public Mfsk_mapperWorkerBase {
  const uint8_t bitsPerSymbol; // = log2(MFSK_MAPPER_M_P);
  const uint8_t initialMask; //  = (MFSK_MAPPER_M_P-1) << (8 - bitsPerSymbol);
  uint8_t symbolIndex, numValidBitsInFirstByte;
  int8_t lowBitIndex; // = 8 - bitsPerSymbol;
  uint8_t mask; // = initialMask;
  unsigned symbolsPerInput;
public:    
  Mfsk_mapperWorker()
    : bitsPerSymbol(log2(MFSK_MAPPER_M_P)),
      initialMask((MFSK_MAPPER_M_P-1) << (8 - bitsPerSymbol)),
      lowBitIndex(8 - bitsPerSymbol),
      mask(initialMask) {}
private:
  RCCResult run(bool /*timedout*/) {
    const uint8_t *inData = (const uint8_t*)in.data();
    int16_t *outData = out.data().real().data();

    if (in.length()*8%bitsPerSymbol){
      setError("Input does not contain even number of symbols: %d. Symbols = length of input messages in bits / log2(M_p).",(in.length()*8%bitsPerSymbol));
      return RCC_ERROR;
    }
    
    symbolsPerInput = in.length()*8/bitsPerSymbol;
    
    // NOT RECOMMENDED FOR NEW DESIGNS - workers should always be designed to
    // support whatever input messages the protocol may provide, and it is
    // intended for this worker to be modified in the future to do so (AV-3797)
    if (symbolsPerInput > 4096){
      setError("Worker's current implementation only supports up to 4096 symbols on its input (even though the protocol may support more), and a message containing %u symbols was received on the input port. Symbols = length of input messages in bits / log2(M_p).",symbolsPerInput);
      return RCC_ERROR;
    }
    
    out.setLength(symbolsPerInput*2);                                     //Symbols are 2 bytes each
    if (!in.length()){                                                    //Pass along ZLMs
      return RCC_ADVANCE;
    }else{
      for (unsigned n = symbolsPerInput; n; n--){                        
	if(mask>0 && mask<MFSK_MAPPER_M_P-1){                             //Symbol spans 2 bytes
	  symbolIndex = (mask & *inData++)<<(-lowBitIndex);               //Get bits from 1st byte and increment byte
	  numValidBitsInFirstByte = bitsPerSymbol+lowBitIndex;            //Store number of validBitsInFirstByte for later
	  mask = initialMask << numValidBitsInFirstByte;                  //Change mask to get remaining bits in 2nd byte
	  lowBitIndex=8+lowBitIndex;                                      //Change lowBitIndex for 2nd byte
	  symbolIndex = symbolIndex | ((*inData & mask) >> lowBitIndex);  //Get bits from 2nd byte
	  lowBitIndex = lowBitIndex - bitsPerSymbol;                      //Update lowBitIndex
	  mask = initialMask >> (bitsPerSymbol - numValidBitsInFirstByte);//Update Mask
	}else{
	  if(!mask){                                                      //Move to next byte. Reset mask and lowBitIndex
	    inData++;                                                    
	    mask =  initialMask;
	    lowBitIndex = 8 - bitsPerSymbol;
	  }
	  symbolIndex = (mask & *inData) >> lowBitIndex;                  //Get bits from byte
	  lowBitIndex = lowBitIndex - bitsPerSymbol;                      //Update lowBitIndex
	  mask = mask >> bitsPerSymbol;                                   //Update Mask
	}

	*outData++ = properties().symbols[symbolIndex];                   //Add symbol to output buffer
      }
      return RCC_ADVANCE;
    }
  }
};

MFSK_MAPPER_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
MFSK_MAPPER_END_INFO
