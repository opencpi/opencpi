
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
  namespace Util {
    // A class that represents information about the protocol at a port.
    class Protocol;
    class Operation {
    public:
      friend class Protocol;
      std::string m_name, m_qualifiedName;
      bool m_isTwoWay;         // not supported much yet...
      unsigned m_nArgs;
      Member *m_args;           // both input and output args.  if twoway, first is return value
      unsigned m_nExceptions;
      Operation *m_exceptions;  // if twoway
      unsigned m_myOffset;      // for determining message sizes
      const char *parse(ezxml_t op, Protocol &);
      Operation();
      Operation(const Operation & p );
      ~Operation();
#if 1
      Operation & operator=(const Operation * p );
      Operation & operator=(const Operation & p );
#endif
      inline bool isTwoWay() { return m_isTwoWay; }
      inline Member *args() { return m_args; }
      inline unsigned nArgs() { return m_nArgs; }
      inline const std::string name() { return m_name; }
      void printXML(FILE *f, unsigned indent);
    };
    class Protocol {
    public:
      unsigned m_nOperations;
      Operation *m_operations;
      Operation *m_op;                 // used during parsing
      std::string
	m_qualifiedName,               // IDL-style qualified name (double colon separators)
	m_name;
      // Summary attributes derived from protocols.  May be specified in the absense of protocol
      unsigned m_defaultBufferSize;    // Allow the protocol to simply override the protocol size, if != 0
                                       // and particularly when it is unbounded
      unsigned m_minBufferSize;        // convenience - in bytes
      unsigned m_dataValueWidth;       // the smallest atomic data size in any message
      unsigned m_dataValueGranularity; // smallest multiple of atomic data size
      bool m_diverseDataSizes;         // are there atomic types greater than m_dataValueWidth?
      unsigned m_minMessageValues;     // the smallest valid message size for any operation
                                       // this size would be adequate if you knew that only that
                                       // operation would be used, and, if unbounded, the smallest
                                       // possible message size for that operation. Might be zero
      unsigned m_maxMessageValues;     // the largest size, in values, in any message
                                       // for unbounded, defaults to zero, but can be overriden to
                                       // to simply apply a bound anyway.
      bool m_variableMessageLength;    // are there messages or different or unbounded sizes?
      bool m_zeroLengthMessages;       // are there messages of zero length (min == 0)
      bool m_isTwoWay;                 // are there operations that are two-way?
      bool m_isUnbounded;              // are there messages with no upper bound?
      Protocol();
      Protocol(const Protocol & p );
      virtual ~Protocol();
      Protocol & operator=( const Protocol & p );
      Protocol & operator=( const Protocol * p );
      virtual const char *parseOperation(ezxml_t op);
      inline bool isTwoWay() { return m_isTwoWay; }
      inline unsigned &nOperations() { return m_nOperations; }
      inline Operation *operations() { return m_operations; }
      const char *parse(ezxml_t x);
      const char *parseSummary(ezxml_t x);
      const char *finishParse();
      void printXML(FILE *f, unsigned indent);
    };
  }
}

#endif
