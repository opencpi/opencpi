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
 * This file contains the implementation for the advanced_pattern worker in C++
 */

#include <cstring> // memcpy
#include <iostream>
#include "advanced_pattern-worker.hh"

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Advanced_patternWorkerTypes;

#ifndef likely
#define likely(x)       __builtin_expect(!!(x), 1)
#define unlikely(x)     __builtin_expect(!!(x), 0)
#endif

class Advanced_patternWorker : public Advanced_patternWorkerBase {

  size_t idx, remaining;
  bool run_forever, send_zlm;

  RCCResult run(bool /*timedout*/) {
    if (unlikely(firstRun())) {
      idx = 0;
      remaining = properties().LoopCount;
      run_forever = (0 == remaining);
      send_zlm = false;
      if (ADVANCED_PATTERN_OCPI_DEBUG)
        std::cerr << "firstRun: LoopCount=" << properties().LoopCount << ", ZLM=" << (uint32_t) properties().ZLM << ", Pattern.size=" << properties().Pattern.size() << std::endl;
      // Special case - no patterns to send. Send ZLM or do nothing. Ignore loops.
      if (0 == properties().Pattern.size()) {
        if (ADVANCED_PATTERN_OCPI_DEBUG)
          std::cerr << "firstRun: Special case ZLM=" << properties().ZLM << std::endl;
        if (properties().ZLM <= 255)
          send_zlm = true;
        else
          return RCC_DONE;
      }
    } // firstRun
    if (unlikely(send_zlm)) {
      // send_zlm = false;
      if (ADVANCED_PATTERN_OCPI_DEBUG)
        std::cerr << "Sending ZLM on " << properties().ZLM << std::endl;
      out.setLength(0);
      out.setOpCode(properties().ZLM);
      ++properties().current.Total.messages;
      ++properties().current.Opcode[properties().ZLM].messages;
      return RCC_ADVANCE_DONE;
    } // send_zlm "state"

    const uint8_t cur_opcode = properties().Pattern.data[idx].Opcode;
    const size_t cur_len = properties().Pattern.data[idx].Bytes.size();
    if (ADVANCED_PATTERN_OCPI_DEBUG)
      std::cerr << "Sending Opcode " << (uint32_t) cur_opcode << " with " << cur_len << " bytes\n";

    // Port Data:
    out.setLength(cur_len);
    out.setOpCode(cur_opcode);
    memcpy(out.data(),
           properties().Pattern.data[idx].Bytes.data,
           cur_len);
    // Property Data:
    ++properties().current.Total.messages;
    properties().current.Total.bytes += cur_len;
    ++properties().current.Opcode[cur_opcode].messages;
    properties().current.Opcode[cur_opcode].bytes += cur_len;

    if (++idx == properties().Pattern.size()) { // End of pattern sequence
      idx = 0;
      if (ADVANCED_PATTERN_OCPI_DEBUG) {
        if (run_forever)
          std::cerr << "End of pattern: running forever\n";
        else
          std::cerr << "End of pattern: loops remaining = " << remaining-1 << "\n";
      }
      if (!run_forever && (0 == --remaining)) {
        if (properties().ZLM <= 255) // Handle ZLM
          send_zlm = true;
        else
          return RCC_ADVANCE_DONE; // No ZLM, just finish
      }
    }
    return RCC_ADVANCE;
  }
};

ADVANCED_PATTERN_START_INFO
ADVANCED_PATTERN_END_INFO
