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
 * Abstract:
 *   RCC port class
 *
 * Revision History: 
 * 
 *    Author: John F. Miller
 *    Date: 3/2005
 *    Revision Detail: Created
 *
 */
#include "RccWorker.h"
#include "RccPort.h"

namespace OC = OCPI::Container;
namespace OA = OCPI::API;
namespace OU = OCPI::Util;
namespace OD = OCPI::DataTransport;
namespace OR = OCPI::RDT;
namespace DTM = DataTransfer::Msg;

namespace OCPI {
  namespace RCC {

    Port::
    Port(Worker& w, const OU::Port & pmd, const OU::PValue *params, RCCPort &rp)
      :  OC::PortBase<Worker, Port, OCPI::RCC::ExternalPort>(w, *this, pmd, params),
	 m_localOther(NULL), m_rccPort(rp), m_buffer(NULL),
	 // Internal ports for non-scaled crews don't get buffers
         m_wantsBuffer(pmd.m_isInternal && w.crewSize() <= 1 ? false : true) {
      // FIXME: deep copy params?
      // Initialize rccPort with aspects based on metadata
      if (pmd.nOperations() <= 1) {
	m_rccPort.useDefaultOpCode_ = true;
	if (pmd.nOperations() == 1) {
	  m_rccPort.useDefaultLength_ = true;
	  OU::Operation &o = pmd.operations()[0];
	  if (o.nArgs()) {
	    m_rccPort.defaultLength_ = pmd.m_minBufferSize;
	    if (o.nArgs() > 1) {
	      OU::Member &m = o.args()[o.nArgs() - 1];
	      if (m.isSequence())
		m_rccPort.sequence = &m;
	    }
	  }
	}
      }
    }

    Port::
    ~Port()
    {
      // As the most derived class, the mutex must be locked during destruction
      // It will automatically be unlocked during deferred virtual destruction
      lock();
    }
    void Port::
    error(std::string &e) {
      parent().portError(e);
    }

    void Port::
    connectURL(const char */*url*/, const OU::PValue */*myParams*/,
	       const OU::PValue */*otherParams*/)
    {
    }
    bool Port::advanceRcc(size_t max) {
      try {
	if (m_buffer) {
	  if (isOutput())
	    if (parent().version() <= 1 && m_rccPort.current.length_ == 0 &&
		m_rccPort.current.opCode_ == 0)
	      m_buffer->put(0, 0, true, m_rccPort.current.direct_);
	    else
	      m_buffer->put(m_rccPort.current.length_, m_rccPort.current.opCode_, false,
			    m_rccPort.current.direct_);
	  else
	    release(); // m_buffer->release(); must release on port gotten from
	  m_rccPort.current.data = NULL;
	  m_buffer = NULL;
	}
	bool ready = requestRcc();
	if (ready && max && max > m_rccPort.current.maxLength)
	  throw OU::Error("Output buffer request/advance (size %zu) greater than buffer size "
			  " (%zu)", max, m_rccPort.current.maxLength);
      } catch (std::string &e) {
	error(e);
      }
      return false;
    }
  }
}
