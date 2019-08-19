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
#include <iostream>
#include <string>
#include "mfsk_mapper-worker.hh"


using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Mfsk_mapperWorkerTypes;

// create a template that uses INPUT_DATA_BYTE_GROUPING to determine the type width for the
// internal buffer and the output buffer
template <int T> struct io_width_template    { /* no "type" defined for compile-time error */ };
template <>      struct io_width_template<1> { typedef uint8_t  type; };
template <>      struct io_width_template<2> { typedef uint16_t type; };
template <>      struct io_width_template<4> { typedef uint32_t type; };
template <>      struct io_width_template<8> { typedef uint64_t type; };

typedef io_width_template<MFSK_MAPPER_INPUT_DATA_BYTE_GROUPING>::type data_t;

class Mfsk_mapperWorker : public Mfsk_mapperWorkerBase
{
  const data_t dataWidth;
  const size_t num_of_symbols;
  size_t bitsPerSymbol;
  data_t initialMask;
  uint8_t lowBitIndex;
  uint8_t symbolIndex, numValidBitsInFirstByte;
  data_t mask;
  size_t symbolsPerInput;
  size_t symbolsSent;
  size_t inDataIndex;
  long totalSample;


public:
  Mfsk_mapperWorker()
      : dataWidth(sizeof(data_t) * 8),
		num_of_symbols(MFSK_MAPPER_M_P),
        bitsPerSymbol(log2(num_of_symbols)),
        initialMask((num_of_symbols - 1) << (dataWidth - bitsPerSymbol)),
        lowBitIndex(dataWidth - bitsPerSymbol),
        mask(initialMask) {}

  RCCResult initialize()
  {
	// legacy compiler support
	symbolsSent = 0;
	inDataIndex = 0;
	totalSample = 0;
    // constructor initialization would not initialize properties sequentially when later properties depended on the result.
    bitsPerSymbol = log2(num_of_symbols);
    initialMask = (num_of_symbols - 1) << (dataWidth - bitsPerSymbol);
    mask = initialMask;
    // low bit index needs to be reset otherwise the index will be off on reset
    lowBitIndex = dataWidth - bitsPerSymbol;
    return RCC_OK;
  }

private:
  RCCResult run(bool /*timedout*/)
  {
    const data_t *inData = (const data_t *)in.data();
    const size_t outCapacity = out.data().real().capacity();
    int16_t *outData = out.data().real().data();

    if (in.length() * dataWidth % bitsPerSymbol)
      return setError("Input does not contain even number of symbols: %d. Symbols = length of input messages in bits / log2(M_p).", (in.length() * dataWidth % bitsPerSymbol));

    bool oddNumberOfBytes = false;
    size_t in_length = in.length();
    size_t remainingBytes = in.length() % sizeof(data_t);
    if (remainingBytes != 0)
    {
      in_length -= remainingBytes; // don't process last byte since it
      oddNumberOfBytes = true;
    }
    symbolsPerInput = in_length * 8 / bitsPerSymbol; //symbols sent to output port

    const size_t symbolsToSend = std::min(symbolsPerInput - symbolsSent, outCapacity);

    // finalize output the remaining bytes at the end of the input buffer
    if (oddNumberOfBytes && symbolsToSend == 0)
    {
      sendExtraBytes(remainingBytes, outData);
      resetForNewData();
      return RCC_ADVANCE;
    }

    out.data().real().resize(symbolsToSend);

    if (!in.length())
    { //Pass along ZLMs
      return RCC_ADVANCE;
    }
    else
    {

      size_t symbolsSentThisRun = 0;
      for (size_t n = 0; n < symbolsToSend && symbolsSent < symbolsPerInput; n++)
      {
        if (mask > 0 && mask < num_of_symbols - 1)
        {                                                                            //Symbol spans 2 bytes
          symbolIndex = (mask & inData[inDataIndex++]) << (-lowBitIndex);            //Get bits from 1st byte and increment byte
          numValidBitsInFirstByte = bitsPerSymbol + lowBitIndex;                     //Store number of validBitsInFirstByte for later
          mask = initialMask << numValidBitsInFirstByte;                             //Change mask to get remaining bits in 2nd byte
          lowBitIndex = dataWidth + lowBitIndex;                                     //Change lowBitIndex for 2nd byte
          symbolIndex = symbolIndex | ((inData[inDataIndex] & mask) >> lowBitIndex); //Get bits from 2nd byte
          lowBitIndex = lowBitIndex - bitsPerSymbol;                                 //Update lowBitIndex
          mask = initialMask >> (bitsPerSymbol - numValidBitsInFirstByte);           //Update Mask
        }
        else
        {
          symbolIndex = (mask & inData[inDataIndex]) >> lowBitIndex; //Get bits from byte
          lowBitIndex = lowBitIndex - bitsPerSymbol;                 //Update lowBitIndex
          mask = mask >> bitsPerSymbol;                              //Update Mask
          //clean up
          if (!mask) // mask needs to be reset for each run method call otherwise the pointer will be incremented prematurely.
          {          // Move to next byte. Reset mask and lowBitIndex
            inDataIndex++;
            mask = initialMask;
            lowBitIndex = dataWidth - bitsPerSymbol; // low bit index needs to be reset otherwise the index will be off on reset
          }
        }
        totalSample++;
        *outData++ = static_cast<int16_t>(properties().symbols[symbolIndex]); //Add symbol to output buffer
        symbolsSent++;
        symbolsSentThisRun++;
      }

      if (symbolsSent < symbolsPerInput)
      {
        // mask needs to be reset for each run method call otherwise the pointer will be incremented prematurely.
        mask = initialMask;
        // low bit index needs to be reset otherwise the index will be off on reset
        lowBitIndex = dataWidth - bitsPerSymbol;
        out.advance();
      }
      else if (symbolsToSend == symbolsSent && oddNumberOfBytes)
      {
        out.advance();
      }
      else
      {
        resetForNewData();
        in.advance();
        out.advance();
      }

      return RCC_OK;
    }
  }
  void sendExtraBytes(size_t remainingBytes, int16_t *outData)
  {
    size_t symbolsToSend = remainingBytes * 8 / bitsPerSymbol; //symbols sent to output port
    out.data().real().resize(symbolsToSend);
    uint8_t mask8bit;
    uint8_t lowBitIndex8bit;
    size_t itr = 0;

    const uint8_t *inData8 = (const uint8_t *)in.data();
    uint8_t uintDataWidth = sizeof(uint8_t) * 8;
    setMask(num_of_symbols, uintDataWidth, mask8bit, lowBitIndex8bit);
    // if little_endian
    itr = in.length() - 1;
    for (size_t i = 0; i < symbolsToSend; i++)
    {

      symbolIndex = getSymbol<>(inData8[itr], mask8bit, lowBitIndex8bit, bitsPerSymbol);
      *outData++ = static_cast<int16_t>(properties().symbols[symbolIndex]); // Add symbol to output buffer
      if (!mask)                                                            // mask needs to be reset for each run method call otherwise the pointer will be incremented prematurely.
      {                                                                     // Move to next byte. Reset mask and lowBitIndex
        itr--;
        setMask(num_of_symbols, uintDataWidth, mask8bit, lowBitIndex8bit);
      }
    }
  }
  void resetForNewData()
  {
    // Reset: Start
    symbolsSent = 0;
    inDataIndex = 0;
    // mask needs to be reset for each run method call otherwise the pointer will be incremented prematurely.
    mask = initialMask;
    // low bit index needs to be reset otherwise the index will be off on reset
    lowBitIndex = dataWidth - bitsPerSymbol;
    // Reset: End
  }
  template <typename T>
  void setMask(data_t num_of_symbols, T dataWidth, T &return_mask, uint8_t &return_lowBitIndex) const
  {
    T _num_of_symbols = num_of_symbols;
    // constructor initialization would not initialize properties sequentially when later properties depended on the result.
    T _bitsPerSymbol = log2(_num_of_symbols);
    T initialMask = (_num_of_symbols - 1) << (dataWidth - _bitsPerSymbol);
    return_mask = initialMask;
    // low bit index needs to be reset otherwise the index will be off on reset
    return_lowBitIndex = dataWidth - _bitsPerSymbol;
  }
  template <typename T>
  uint8_t getSymbol(uint8_t data, T &mask, uint8_t &lowBitIndex, const data_t bitsPerSymbol)
  {
    uint8_t symbolIndex = (mask & data) >> lowBitIndex; //Get bits from byte
    lowBitIndex = lowBitIndex - bitsPerSymbol;          //Update lowBitIndex
    mask = mask >> bitsPerSymbol;                       //Update Mask
    return symbolIndex;
  }
};

MFSK_MAPPER_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
MFSK_MAPPER_END_INFO
