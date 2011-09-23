
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

#ifndef OCPI_METADATA_PROTOCOL_H
#define OCPI_METADATA_PROTOCOL_H
#include <string>
#include "OcpiUtilProperty.h"
#include "OcpiUtilEzxml.h"

namespace OCPI  {
  namespace Metadata {
    // A class that represents information about the protocol at a port.
    class Protocol;
    class Operation {
      friend class Protocol;
      std::string m_name;
      bool m_isTwoWay; // not supported much yet...
      unsigned m_nArgs;
      OCPI::Util::Prop::Member *m_args; // This class is overkill here, but we need most of it.
      unsigned m_maxAlign, m_myOffset;
      bool m_sub32;
      const char *parse(ezxml_t op);
      Operation();
      Operation(const Operation & p );
      ~Operation();
      Operation & operator=(const Operation * p );
      Operation & operator=(const Operation & p );
    public:
      inline bool isTwoWay() { return m_isTwoWay; }
      inline OCPI::Util::Prop::Member *args() { return m_args; }
      inline unsigned nArgs() { return m_nArgs; }
      inline const std::string name() { return m_name; }
    };
    class Protocol {
      unsigned m_nOperations;
      Operation *m_operations;
      Operation *m_op; // used during parsing
    public:
      std::string m_name;
      unsigned m_dataValueWidth; // the smallest atomic data size in any message
      unsigned m_maxMessageValues; // the largest size, in values, in any message
      unsigned m_dataValueGranularity;
      bool m_variableMessageLength;
      bool m_zeroLengthMessages;
      bool m_diverseDataSizes;
      bool m_isTwoWay;
      
      Protocol();
      Protocol(const Protocol & p );
      virtual ~Protocol();
      Protocol & operator=( const Protocol & p );
      Protocol & operator=( const Protocol * p );
      virtual const char *parseOperation(ezxml_t op);
      inline bool isTwoWay() { return m_isTwoWay; } // Are any operations twoway?
      inline unsigned &nOperations() { return m_nOperations; }
      inline Operation *operations() { return m_operations; }
      const char *parse(ezxml_t x);
    };


  }
}

#endif
