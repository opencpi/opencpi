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
 * THIS FILE WAS ORIGINALLY GENERATED ON Thu Jul 19 09:44:39 2018 EDT
 * BASED ON THE FILE: iqstream_max_calculator.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the iqstream_max_calculator worker in C++
 */

#include <cstdint> // int16_t
#include <cstdio> // size_t
#include "iqstream_max_calculator-worker.hh"
#include "OcpiOsDebugApi.hh" // OCPI_LOG_INFO


using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Iqstream_max_calculatorWorkerTypes;

class Iqstream_max_calculatorWorker : public Iqstream_max_calculatorWorkerBase {
  bool m_I_reset_is_queued, m_Q_reset_is_queued;

public:
  Iqstream_max_calculatorWorker() : 
      m_I_reset_is_queued(false), m_Q_reset_is_queued(false) {

    m_properties.max_I = std::numeric_limits<int16_t>::min();
    m_properties.max_Q = std::numeric_limits<int16_t>::min();
    m_properties.max_I_is_valid = false;
    m_properties.max_Q_is_valid = false;
  }
private:

  // notification that max_I property will be read
  RCCResult max_I_read() {

    perform_previously_queued_resets();

    log(OCPI_LOG_INFO, "max_I property read, queueing up max_I reset...");
    m_I_reset_is_queued = true;

    return RCC_OK;
  }
  // notification that max_Q property will be read
  RCCResult max_Q_read() {

    perform_previously_queued_resets();

    log(OCPI_LOG_INFO, "max_Q property read, queueing up max_Q reset...");
    m_Q_reset_is_queued = true;

    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {

    perform_previously_queued_resets();

    if(out.isConnected()) {
      out.iq().data().resize(in.iq().data().size());
      out.setOpCode(in.getOpCode());
    }

    const IqstreamIqData* inData = in.iq().data().data();
    IqstreamIqData* outData = out.isConnected() ? out.iq().data().data() : 0;

    for(size_t ii = 0; ii < in.iq().data().size(); ii++) {
      if(inData->I > m_properties.max_I) {
        m_properties.max_I = inData->I;
        m_properties.max_I_is_valid = true;
        log(OCPI_LOG_INFO, "max_I updated to value of %i", m_properties.max_I);
      }
      if(inData->Q > m_properties.max_Q) {
        m_properties.max_Q = inData->Q;
        m_properties.max_Q_is_valid = true;
        log(OCPI_LOG_INFO, "max_Q updated to value of %i", m_properties.max_Q);
      }

      if(outData != 0) { // i.e., is out port connected
        outData->I = inData->I;
        outData->Q = inData->Q;
        outData++;
      }

      inData++;
    }

    return RCC_ADVANCE;
  }

  void perform_previously_queued_resets() {
    if(m_I_reset_is_queued) {
      m_properties.max_I = std::numeric_limits<int16_t>::min();
      m_properties.max_I_is_valid = false;
      log(OCPI_LOG_INFO, "...performing max_I reset (to %i) that was previously queued", m_properties.max_I);
      m_I_reset_is_queued = false;
    }
    if(m_Q_reset_is_queued) {
      m_properties.max_Q = std::numeric_limits<int16_t>::min();
      m_properties.max_Q_is_valid = false;
      log(OCPI_LOG_INFO, "...performing max_Q reset (to %i) that was previously queued", m_properties.max_Q);
      m_Q_reset_is_queued = false;
    }
  }
};

IQSTREAM_MAX_CALCULATOR_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
IQSTREAM_MAX_CALCULATOR_END_INFO
