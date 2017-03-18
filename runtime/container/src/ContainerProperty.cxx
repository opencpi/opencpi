
/*
 *  Copyright (c) Mercury Federal Systems, Inc., Arlington VA., 2009-2010
 *
 *    Mercury Federal Systems, Incorporated
 *    1901 South Bell Street
 *    Suite 402
 *    Arlington, Virginia 22202
 *    United States of America
 *    Telephone 703-413-0781
 *    FAX 703-413-0784
 *
 *  This file is part of OpenCPI (www.opencpi.org).
 *     ____                   __________   ____
 *    / __ \____  ___  ____  / ____/ __ \ /  _/ ____  _________ _
 *   / / / / __ \/ _ \/ __ \/ /   / /_/ / / /  / __ \/ ___/ __ `/
 *  / /_/ / /_/ /  __/ / / / /___/ ____/_/ / _/ /_/ / /  / /_/ /
 *  \____/ .___/\___/_/ /_/\____/_/    /___/(_)____/_/   \__, /
 *      /_/                                             /____/
 *
 *  OpenCPI is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OpenCPI is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OpenCPI.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OcpiOsDebugApi.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilException.h"
#include "OcpiContainerApi.h"

namespace OU = OCPI::Util;
namespace OS = OCPI::OS;
namespace OCPI {
  namespace API {
    PropertyAccess::~PropertyAccess(){}
    void Property::checkTypeAlways(BaseType ctype, size_t n, bool write) const {
      const char *err = NULL;
      if (write && !m_info.m_isWritable)
	err = "trying to write a non-writable property";
      else if (write && !m_worker.beforeStart() && m_info.m_isInitial)
	err = "trying to write a an initial property after worker is started";
      else if (!write && !m_info.m_isReadable)
	err = "trying to read a non-readable property";
      else if (m_info.m_baseType == OCPI_Struct)
	err = "struct type used as scalar type";
      else if (ctype != m_info.m_baseType)
	err = "incorrect type for this property";
      else if (m_info.m_isSequence) {
	if (n % m_info.m_nItems)
	  err = "number of items not a multiple of array size";
	else {
	  n /= m_info.m_nItems;
	  if (write && n > m_info.m_sequenceLength)
	    err = "sequence or array too long for this property";
	  else if (!write && n < m_info.m_sequenceLength)
	    err = "sequence or array not large enough for this property";
	}
      } else if (n != m_info.m_nItems)
	  err = "wrong number of values for non-sequence type";
      if (err)
	throw OU::Error("Access error for property \"%s\": %s", m_info.cname(), err);
    }
    // This is user-visible, initialized from information in the metadata
    // It is intended to be constructed on the user's stack - a cache of
    // just the items needed for fastest access
    Property::Property(Worker &w, const char *aname) :
      m_worker(w), m_readVaddr(NULL), m_writeVaddr(NULL),
      m_info(w.setupProperty(aname, m_writeVaddr, m_readVaddr)), m_ordinal(m_info.m_ordinal),
      m_readSync(m_info.m_readSync), m_writeSync(m_info.m_writeSync) {
    }
    // This is a sort of side-door from the application code
    // that already knows the property ordinal
    Property::Property(Worker &w, unsigned n) :
      m_worker(w), m_readVaddr(NULL), m_writeVaddr(NULL),
      m_info(w.setupProperty(n, m_writeVaddr, m_readVaddr)), m_ordinal(m_info.m_ordinal),
      m_readSync(m_info.m_readSync), m_writeSync(m_info.m_writeSync) {
    }

    BaseType Property::baseType() const {return m_info.m_baseType;}
  }
}
