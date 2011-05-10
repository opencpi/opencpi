
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

#include "OcpiContainerApplication.h"
#include "OcpiWorker.h"
#include "OcpiContainerPort.h"
#include "OcpiContainerInterface.h"
#include "OcpiContainerMisc.h"
#include "OcpiContainerArtifact.h"

namespace OCPI {
  namespace API {
#if !defined(NDEBUG) || defined(OCPI_API_CHECK_PROPERTIES)
    void Property::checkType(ScalarType ctype, unsigned n, bool write) {
      if (write && !m_info.m_isWritable)
	throw "trying to write a non-writable property";
      if (!write && !m_info.m_isReadable)
	throw "trying to read a non-readable property";
      if (m_info.m_isStruct)
	throw "struct type used as scalar type";
      if (ctype != m_type.scalar)
	throw "incorrect type for this property";
      if (write && n > m_type.length)
	throw "sequence or array too long for this property";
      if (!write && n < m_type.length)
	throw "sequence or array not large enough for this property";
    }
#endif
    // This is user-visible, initialized from information in the metadata
    // It is intended to be constructed on the user's stack - a cache of
    // just the items needed for fastest access
    Property::Property(Worker &w, const char *aname) :
      m_worker(w), m_writeVaddr(0), m_readVaddr(0)
    {
      w.setupProperty(aname, *this);
      // Get the metadata about this property from the worker's database.
      if (m_info.m_isStruct) {
	m_type.scalar = OCPI_none; // Make all scalar type checks fail
	m_info.m_isStruct = true;
      }
    }
  }
}
