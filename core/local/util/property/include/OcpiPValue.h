
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


/*
 * Abstact:
 *   This file defines the PValue class for types property value lists
 *
 * Revision History: 
 * 
 *    Author: Jim Kulp
 *    Date: 7/2009
 *    Revision Detail: Created
 *
 */

#ifndef OCPI_PVALUE_H
#define OCPI_PVALUE_H

#include <cstring>
#include "OcpiUtilException.h"
#include "OcpiPValueApi.h"


namespace OCPI {
  
  namespace Util {
    typedef OCPI::API::PValue PValue;
#define OCPI_DATA_TYPE(sca, corba, letter, bits, run, pretty, store)	\
      bool find##pretty(const PValue* p, const char* name, run &value);
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S(sca, corba, letter, bits, run, pretty, store)	\
      bool find##pretty(const PValue* p, const char* name, run &value);
      OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
#undef OCPI_DATA_TYPE_S
#define OCPI_DATA_TYPE_S OCPI_DATA_TYPE
#define OCPI_DATA_TYPE(sca,corba,letter,bits,run,pretty,store) \
    typedef OCPI::API::PV##pretty PV##pretty;
  OCPI_PROPERTY_DATA_TYPES
#undef OCPI_DATA_TYPE
    extern PVULong PVEnd;
  }
} // OCPI
#endif

