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
 * THIS FILE WAS ORIGINALLY GENERATED ON Mon Sep 16 08:51:29 2013 EDT
 * BASED ON THE FILE: real_digitizer.xml
 * This worker performs symbol decisions by decoding signed real Q0.15
 * symbols into bits. Each positive symbol is decoded to a 1 and each
 * negative symbol is decoded into a 0. Once the sync criteria is met,
 * the decoded bits are packed into 16-bit words which are subsequently
 * sent to the output port.  The sync criteria is met when a) the need_sync
 * property has a value of false, or b) the first 0xFACE sequence occurs in
 * the decoded bits. When the first 0xFACE sequence occurs in the decoded
 * bits and the enable_printing is true, the following is printed to stdout:
 *
 * To maintain byte swapping If you don't want to use byte swapping use 8 bit mode
 * Updated worker to support byte swapping and the non byte swapping 8 bit mode.
 * This was done to adhere to previous implementations.
 *
 * No buffer limit is checked to protect from going over the buffer length because rstream has 2048 samples which is less then the minimum output protocol.
 *
 * This worker is currently 2 mfsk either positive or negative.
 * This file contains the RCC implementation skeleton for worker: real_digitizer
 * Limitation:
 *  This worker currently only supports 2 symbols from the mfsk either
 *  positive or negative corresponding to 1 or 0 respectively.
 *
 */
#include <iostream>
#include <iomanip>
#include <sstream>
#include <ctype.h>
#include <math.h>
#include <arpa/inet.h>
#include "real_digitizer-worker.hh"

inline double Uscale(double x)
{
  return (double) (x) / (pow(2, 15) - 1);
}

inline size_t byteLen2Real(size_t x)
{
  return ((x) / 2);
}

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Real_digitizerWorkerTypes;

// create a template that uses DIN_WIDTH_p to determine the type width for the
// internal buffer and the output buffer
template <int T> struct io_width_template    { /* no "type" defined for compile-time error */ };
template <>      struct io_width_template<1> { typedef uint8_t  type; };
template <>      struct io_width_template<2> { typedef uint16_t type; };
template <>      struct io_width_template<4> { typedef uint32_t type; };
template <>      struct io_width_template<8> { typedef uint64_t type; };

typedef io_width_template<REAL_DIGITIZER_INPUT_DATA_BYTE_GROUPING>::type data_t;

class Real_digitizerWorker : public Real_digitizerWorkerBase
{
  const data_t num_of_symbols; // currently 2 mfsk, either positive or negative
  const size_t dataWidth;
  const size_t bitsPerSymbol;
  const size_t bytes;
  const data_t initialMask;
  uint16_t sync_pattern;

  data_t m_mask;
  data_t m_data;
  uint16_t m_sync_pattern_buffer;

  RCCResult start()
  {
    m_mask = 0;
    m_data = 0;
    properties().sync_criteria_met = !properties().need_sync;
    // Sync pattern adjustment based on platform's endianness
    printf("real_digitizer: sync pattern 0x%X\n",properties().sync_pattern);
    switch (bytes) {
      case 1: sync_pattern =       properties().sync_pattern;  break;
      case 2: sync_pattern = ntohs(properties().sync_pattern); break;
      default: setError("Invalid byteswap requested!"); return RCC_FATAL;
    }
    //printf("real_digitizer: sync pattern after byte swap 0x%X\n",sync_pattern);
    /* This is the code I believe needs to be used to print "after byte swap properly" but untested:
    if (1 != bytes) printf("real_digitizer: actual pattern: 0x%02X%02X", static_cast<uint8_t>(*(static_cast<uint8_t *>(&sync_pattern))), static_cast<uint8_t>(*(static_cast<uint8_t *>(&sync_pattern+1))) );
    */
    return RCC_OK;
  }

  RCCResult run(bool /*timedout*/)
  {
    const int16_t *inData = in.data().real().data();
    data_t* outData = reinterpret_cast<data_t*>(out.data());

    size_t len = byteLen2Real(in.length());
    size_t out_element_itr = 0;
    data_t data = 0;


    data_t mask = initialMask;

    if (in.length() == 0)
    {
      out.setLength(0);
      out.advance();
      return RCC_OK;
    }

    if (m_mask > 0)
    {
      data = m_data;
      mask = m_mask;
      m_mask = 0;
    }

    for (size_t i = 0; i < len; i++)
    {
      if (!properties().sync_criteria_met)
      {
        m_data = m_data << 1;
        m_sync_pattern_buffer = m_sync_pattern_buffer << 1;
        if (Uscale(inData[i]) > 0)
        {
          m_data++;
          m_sync_pattern_buffer++;
        }

        if (m_sync_pattern_buffer == sync_pattern)
        {
          properties().sync_criteria_met = 1;
          if (properties().enable_printing)
          {
            printf("real_digitizer: sync pattern 0x%X found\n", properties().sync_pattern);
          }
        }
      }
      else
      {
        if (Uscale(inData[i]) > 0)
        {
          data += mask;
        }

        mask = mask >> 1;
        if (mask == 0x0000)
        {
          mask = initialMask;
          outData[out_element_itr++] = data;
          data = 0;
        }
      }
    }
    if (mask != initialMask)
    {
      m_mask = mask;
      m_data = data;
    }

    out.setLength(bytes * out_element_itr);
    out.setOpCode(in.opCode());

    if (out.length() == 0)
    {
      in.advance();
      return RCC_OK;
    }
    else
    {
      return RCC_ADVANCE;
    }
  }

public :
  Real_digitizerWorker() : num_of_symbols(2),
		dataWidth(sizeof(data_t) * 8),
		bitsPerSymbol(log2(num_of_symbols)),
		bytes(sizeof(data_t)),
		initialMask((num_of_symbols - 1) << (dataWidth - bitsPerSymbol)),
		m_sync_pattern_buffer(0){}
};

REAL_DIGITIZER_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
REAL_DIGITIZER_END_INFO
