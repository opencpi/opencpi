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
 * THIS FILE WAS ORIGINALLY GENERATED ON Fri Jun 29 10:35:58 2018 EDT
 * BASED ON THE FILE: tx_event_ctrlr.xml
 * YOU *ARE* EXPECTED TO EDIT IT
 *
 * This file contains the implementation skeleton for the tx_event_ctrlr worker in C++
 */

#include "tx_event_ctrlr-worker.hh"
#include "OcpiOsDebugApi.hh" // OCPI_LOG_INFO

using namespace OCPI::RCC; // for easy access to RCC data types and constants
using namespace Tx_event_ctrlrWorkerTypes;

class Tx_event_ctrlrWorker : public Tx_event_ctrlrWorkerBase {

  const RunCondition m_aRunConditionNoPorts;

  std::vector<uint8_t> m_pending_txens;
  bool                 m_txen;

public:
  Tx_event_ctrlrWorker() : m_aRunConditionNoPorts(RCC_NO_PORTS), m_txen(true) {
    // disable run() (until txen property is written)
    setRunCondition(&m_aRunConditionNoPorts);
    out.setDefaultLength(0); // all messages sent to 'out' port will be ZLMs
  }
private:

  // notification that txen property has been written
  RCCResult txen_written() {
    log(OCPI_LOG_DEBUG, "in txen_written()");
    m_pending_txens.push_back(m_properties.txen);

    // enable run() (now that txen property is written)
    setRunCondition(NULL);  // default of all ports

    return RCC_OK;
  }
  // notification that txen property will be read
  RCCResult txen_read() {
    m_properties.txen = m_txen;
    return RCC_OK;
  }
  RCCResult run(bool /*timedout*/) {
    log(OCPI_LOG_DEBUG, "in run()");

    if(m_pending_txens.size() > 0) {
      auto it = m_pending_txens.begin();
      if(*it) {
        out.setOpCode(Tx_eventTxOn_OPERATION);
        log(OCPI_LOG_INFO, "sending to 'out' port a ZLM w/ opcode txOn");
        m_txen = true;
      }
      else {
        out.setOpCode(Tx_eventTxOff_OPERATION);
        m_txen = false;
        log(OCPI_LOG_INFO, "sending to 'out' port a ZLM w/ opcode txOff");
      }

      m_pending_txens.erase(it);
      return RCC_ADVANCE;
    }

    if(m_pending_txens.size() == 0) {
      // disable run() (until next txen property write)
      setRunCondition(&m_aRunConditionNoPorts);
    }

    return RCC_ADVANCE;
  }
};

TX_EVENT_CTRLR_START_INFO
// Insert any static info assignments here (memSize, memSizes, portInfo)
// e.g.: info.memSize = sizeof(MyMemoryStruct);
TX_EVENT_CTRLR_END_INFO
