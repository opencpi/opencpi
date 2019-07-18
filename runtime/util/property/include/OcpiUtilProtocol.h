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

#ifndef OCPI_METADATA_PROTOCOL_H
#define OCPI_METADATA_PROTOCOL_H
#include <string>
#include "OcpiUtilException.h"
#include "OcpiUtilProperty.h"
#include "OcpiUtilEzxml.h"

// The attributes normally derived from examining the protocol operations.
// In the absence of protocol operations, they can be specified directly.
// They can also be specified to override the protocol-operation-derived values
// They may appear as attributes in protocol elements or even port elements
#define OCPI_PROTOCOL_SUMMARY_ATTRS \
  "DataValueWidth", "DataValueGranularity", "DiverseDataSizes", "MaxMessageValues", \
  "VariableMessageLength", "ZeroLengthMessages", "MinMessageValues", "Unbounded", \
  "NumberOfOpcodes", "twoway", "DefaultBufferSize"

namespace OCPI  {
  namespace Util {
    // A class that represents information about the protocol at a port.
    class Protocol;
    class Operation {
    public:
      friend class Protocol;
      std::string m_name, m_qualifiedName;
      bool m_isTwoWay;         // not supported much yet...
      size_t m_nArgs;
      Member *m_args;           // both input and output args.  if twoway, first is return value
      size_t m_nExceptions;
      Operation *m_exceptions;  // if twoway
      size_t m_myOffset;      // for determining message sizes
      bool m_topFixedSequence;  // is this operation a single top level sequence of fixed size elements?
      Operation();
      Operation(const Operation & p );
      ~Operation();
      Operation & operator=(const Operation * p );
      Operation & operator=(const Operation & p );
      const char *parse(ezxml_t op, Protocol &);
      Member *findArg(const char *name) const;
      inline bool isTwoWay() const { return m_isTwoWay; }
      inline Member *args() const { return m_args; }
      inline size_t nArgs() const { return m_nArgs; }
      virtual const char *cname() const { return m_name.c_str(); }
      inline bool isTopFixedSequence() const { return m_topFixedSequence; }
      size_t defaultLength() const;
      void printXML(std::string &out, unsigned indent = 0) const;
      void write(Writer &writer, const uint8_t *data, size_t length);
      size_t read(Reader &reader, uint8_t *data, size_t maxLength);
      // for testing
      void generate(const char *name, Protocol &p);
      void generateArgs(Value **&);
      void print(FILE *, Value **v) const;
      void testPrintParse(FILE *f, Value **v, bool hex);
    };
    class Protocol {
    public:
      size_t m_nOperations; // the actual length of m_operations
      Operation *m_operations;
      Operation *m_op;                 // used during parsing
      std::string
	m_qualifiedName,               // IDL-style qualified name (double colon separators)
	m_file,
	m_name;
      // Summary attributes derived from protocols.  May be specified in the absense of protocol
      size_t m_defaultBufferSize;      // Allow the protocol to simply override the protocol size, if != SIZE_MAX
                                       // and particularly when it is unbounded
      size_t m_minBufferSize;          // convenience - in bytes
      size_t m_dataValueWidth;         // the smallest atomic data size in any message
      size_t m_dataValueGranularity;   // smallest multiple of atomic data size
      bool m_diverseDataSizes;         // are there atomic types greater than m_dataValueWidth?
      size_t m_minMessageValues;       // the smallest valid message size for any operation
                                       // this size would be adequate if you knew that only that
                                       // operation would be used, and, if unbounded, the smallest
                                       // possible message size for that operation. Might be zero
      size_t m_maxMessageValues;       // the largest size, in values, in any message
                                       // for unbounded, defaults to zero, but can be overriden to
                                       // to simply apply a bound anyway.
      bool m_variableMessageLength;    // are there messages or different or unbounded sizes?
      bool m_zeroLengthMessages;       // are there messages of zero length (min == 0)
      bool m_isTwoWay;                 // are there operations that are two-way?
      bool m_isUnbounded;              // are there messages with no upper bound?
      size_t m_nOpcodes;               // the value from actual operations, or summary, or override
      Protocol();
      Protocol(const Protocol & p );
      Protocol(Protocol *p); // clone
      virtual ~Protocol();
      Protocol & operator=( const Protocol & p );
      Protocol & operator=( const Protocol * p );
      void initForProtocol();
      void initNoProtocol();
      const char *parseOperation(ezxml_t op);
      Operation *findOperation(const char *name);
      void finishOperation(const Operation &op);
      inline bool isTwoWay() { return m_isTwoWay; }
      inline size_t nOperations() const { return m_nOperations; }
      inline Operation *operations() const { return m_operations; }
      virtual const char *cname() const { return m_name.c_str(); }
      const char *parseChild(ezxml_t x);
      const char *parse(ezxml_t x, const char *defName, const char *file,
			const char *(*dochild)(ezxml_t op, void *arg), void *arg);
      // Note this is NOT const char array and must be modifiable in place
      const char *parse(char *proto);
      const char *parseSummary(ezxml_t x);
      const char *finishParse();
      void printXML(std::string &out, unsigned indent = 0) const;
      void write(Writer &writer, const uint8_t *data, size_t length, uint8_t opcode);
      size_t read(Reader &reader, uint8_t *data, size_t maxLength, uint8_t opcode);
      void generate(const char *name);
      void generateOperation(uint8_t &opcode, Value **&v);
      void freeOperation(uint8_t operation, Value **v);
      void printOperation(FILE *, uint8_t opcode, Value **v) const;
      void testOperation(FILE *, uint8_t opcode, Value **v, bool hex);
    };
  }
}

#endif
