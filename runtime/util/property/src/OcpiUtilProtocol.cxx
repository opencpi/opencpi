
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
#include <assert.h>
#include <sys/time.h>
#include <climits>
#include "OcpiUtilException.h"
#include "OcpiUtilProtocol.h"
#include "OcpiUtilValue.h"
#include "OcpiUtilMisc.h"

namespace OCPI {
  namespace OA = OCPI::API;
  namespace OE = OCPI::Util::EzXml;
  namespace Util {

    Operation::Operation()
      : m_isTwoWay(false), m_nArgs(0), m_args(NULL), m_nExceptions(0), m_exceptions(NULL),
	m_myOffset(0), m_topFixedSequence(false) {
    }
    Operation::~Operation() {
      if (m_args)
	delete [] m_args;
      if (m_exceptions)
	delete [] m_exceptions;
    }
#if 0
    Operation & 
    Operation::
    operator=(const Operation & p ) {
      return operator=(&p);
    }
    
    Operation & 
    Operation::
    operator=(const Operation * p )
    {
      m_name = p->m_name;
      m_qualifiedName = p->m_qualifiedName;
      m_isTwoWay = p->m_isTwoWay;
      m_nArgs = p->m_nArgs;
      m_nExceptions = p->m_nExceptions;
      m_myOffset = p->m_myOffset;
      m_args = m_nArgs ? new Member[ m_nArgs ] : NULL;
      m_exceptions = m_nExceptions ? new Operation[m_nExceptions] : NULL;
      for (unsigned int n = 0; n < m_nArgs; n++ )
	m_args[n] = p->m_args[n];
      for (unsigned int n = 0; n < m_nExceptions; n++ )
	m_exceptions[n] = p->m_exceptions[n];
      m_topFixedSequence = p->m_topFixedSequence;
      return *this;
    }
#endif

    const char *Operation::parse(ezxml_t op, Protocol &p) {
      const char *err;
      if ((err = OE::checkAttrs(op, "Name", "Twoway", "QualifiedName", (void*)0)))
	return err;
      const char *name = ezxml_cattr(op, "Name");
      if (!name)
	return "Missing \"Name\" attribute for operation";
      m_name = name;
      name = ezxml_cattr(op, "qualifiedname");
      if (name)
	m_qualifiedName = name;
      bool sub32dummy;
      size_t maxAlignDummy = 0;
      if ((err = OE::getBoolean(op, "TwoWay", &m_isTwoWay)))
	return err;
      if (m_isTwoWay)
	p.m_isTwoWay = true;
      if (!(err = Member::parseMembers(op, m_nArgs, m_args, false, "argument", NULL))) {
	if (m_nArgs == 1 &&
	    ((m_args[0].isSequence() && m_args[0].isFixed()) ||
	     (!m_args[0].isSequence() && m_args[0].m_baseType == OA::OCPI_String)))
	  m_topFixedSequence = true;
	err = Member::alignMembers(m_args, m_nArgs, maxAlignDummy, m_myOffset,
				   p.m_dataValueWidth, p.m_diverseDataSizes,
				   sub32dummy, p.m_isUnbounded, m_topFixedSequence);
      }
      return err;
    }
    Member *Operation::findArg(const char *name) {
      Member *a = m_args;
      for (unsigned n = 0; n < m_nArgs; n++, a++)
	if (!strcasecmp(name, a->m_name.c_str()))
	  return a;
      return NULL;
    }

    Operation *Protocol::findOperation(const char *name) {
      Operation *o = m_operations;
      for (unsigned n = 0; n < m_nOperations; n++, o++)
	if (!strcasecmp(name, o->m_name.c_str()))
	  return o;
      return NULL;
    }

    void Operation::printXML(FILE *f, unsigned indent) const {
      fprintf(f, "%*s<operation", indent * 2, "");
      if (!m_name.empty())
	fprintf(f, " name=\"%s\"", m_name.c_str());
      if (!m_qualifiedName.empty())
	fprintf(f, " qualifiedName=\"%s\"", m_qualifiedName.c_str());
      if (m_nArgs) {
	fprintf(f, ">\n");
	Member *a = m_args;
	for (unsigned n = 0; n < m_nArgs; n++, a++)
	  a->printXML(f, "argument", indent + 1);
	fprintf(f, "%*s</operation>\n", indent * 2, "");
      } else
	fprintf(f, "/>\n");
    }
    void Operation::write(Writer &writer, const uint8_t *data, size_t length) {
      for (size_t n = 0; n < m_nArgs; n++)
	m_args[n].write(writer, data, length, isTopFixedSequence());
    }

    size_t Operation::read(Reader &reader, uint8_t *data, size_t maxLength) {
      size_t max = maxLength;
      bool fake = data == NULL;
      for (unsigned n = 0; n < m_nArgs; n++)
	m_args[n].read(reader, data, maxLength, fake);
      return max - maxLength;
    }

    void Operation::generate(const char *name, Protocol &p) {
      m_name = name;
      m_nArgs = random() % 10;
      Member *m = m_args = m_nArgs ? new Member[m_nArgs] : NULL;
      for (unsigned n = 0; n < m_nArgs; n++, m++) {
	char *aname;
	ocpiCheck(asprintf(&aname, "arg%d", n) > 0);
	m->generate(aname);
	free(aname);
      }
      const char *err;
      bool sub32dummy = false;
      size_t maxAlignDummy = 1;
      if ((err = Member::alignMembers(m_args, m_nArgs, maxAlignDummy, m_myOffset,
				      p.m_dataValueWidth, p.m_diverseDataSizes,
				      sub32dummy, p.m_isUnbounded, m_topFixedSequence)))
	throw std::string(err);

    }
    void Operation::generateArgs(Value **&v) {
      v = m_nArgs ? new Value *[m_nArgs] : 0;
      for (unsigned n = 0; n < m_nArgs; n++) {
	v[n] = new Value(m_args[n], NULL);
	v[n]->generate();
      }
    }
    void Operation::print(FILE *f, Value **v) const {
      fprintf(f, "%s\n", m_name.c_str());
      for (unsigned n = 0; n < m_nArgs; n++) {
	std::string s;
	v[n]->unparse(s);
	fprintf(f, "  %u: %s\n", n, s.c_str());
      }
    }
    void Operation::testPrintParse(FILE *f, Value **v, bool hex) {
      for (unsigned n = 0; n < m_nArgs; n++) {
	std::string s;
	fprintf(f, "%s: ", m_args[n].m_name.c_str());
	v[n]->unparse(s, NULL, false, hex);
	Value tv(m_args[n]);
	const char *err;
	if ((err = tv.parse(s.c_str(), NULL))) {
	  fprintf(f, "error: %s (%s)\n", err, s.c_str());
	  exit(1);
	} else {
	  fprintf(f, "parse ok\n");
	  std::string s1;
	  tv.unparse(s1, NULL, false, hex);
	  if (s != s1) {
	    fprintf(f,
		    "\n"
		    "error: mismatch0: ***%s***\n"
		    "error: mismatch1: ***%s***\n",
		    s.c_str(), s1.c_str());
	    exit(1);
	  }
	}
      }
      fprintf(f, "\n");
      
    }

    // These defaults are for when there is no protocol at all.
    // Notice that the default is for 1 byte fixed size messages...
    Protocol::Protocol()
      : m_nOperations(0), m_operations(NULL), m_op(NULL), m_defaultBufferSize(0),
	m_minBufferSize(0), m_dataValueWidth(8), m_dataValueGranularity(1), m_diverseDataSizes(false),
	m_minMessageValues(0), m_maxMessageValues(1), m_variableMessageLength(false),
	m_zeroLengthMessages(false), m_isTwoWay(false), m_isUnbounded(false)
    {
    }
#if 0
    Protocol::
    Protocol(const Protocol & p )
    {
      *this = p;
    }
#endif
    Protocol::~Protocol() {

      if (m_operations)
	delete [] m_operations;
    }
#if 0
    Protocol & 
    Protocol::
    operator=( const Protocol & p ) 
    {
      return operator=(&p);
    }
    Protocol & 
    Protocol::
    operator=( const Protocol * p )
    {
      
      m_nOperations = p->m_nOperations;
      m_qualifiedName = p->m_qualifiedName;
      m_name = p->m_name;
      m_defaultBufferSize = p->m_defaultBufferSize;
      m_minBufferSize = p->m_minBufferSize;
      m_dataValueWidth = p->m_dataValueWidth;
      m_dataValueGranularity = p->m_dataValueGranularity;
      m_diverseDataSizes = p->m_diverseDataSizes;
      m_minMessageValues = p->m_minMessageValues;
      m_maxMessageValues = p->m_maxMessageValues;
      m_variableMessageLength = p->m_variableMessageLength;
      m_zeroLengthMessages = p->m_zeroLengthMessages;
      m_isTwoWay = p->m_isTwoWay;
      m_isUnbounded = p->m_isUnbounded;
      m_operations = m_nOperations ? new Operation[m_nOperations] : NULL;
      for ( unsigned int n=0; n<m_nOperations; n++ )
	m_operations[n] = p->m_operations[n];
      m_op = NULL;
      return *this;
    }
#endif
    void Protocol::
    finishOperation(const Operation &op) {
      if (m_isUnbounded ||
	  (m_maxMessageValues && m_maxMessageValues != op.m_myOffset))
	m_variableMessageLength = true;
      if (op.m_myOffset > m_maxMessageValues)
	m_maxMessageValues = op.m_myOffset; // still in bytes until later
      size_t minLength; // smallest message possible for this operation
      if (op.m_nArgs) {
	Member &m = op.m_args[op.m_nArgs - 1];
	minLength =
	  m.m_isSequence ? (op.m_nArgs == 1 ? 0 : m.m_offset + sizeof(uint32_t)) : op.m_myOffset;
      } else
	  minLength = 0;
      if (minLength > m_minBufferSize)
	m_minBufferSize = minLength; // the maximum of the smallest per operation
      if (minLength < m_minMessageValues)
	m_minMessageValues = minLength; // the minimum of the smallest per operation
      if (minLength == 0)
	m_zeroLengthMessages = true;
      // Find smallest element for data granularity purposes
      // When a more disruptive change is required, this could be done in 
      // alignMembers and offset()
      size_t smallest = SIZE_MAX;
      for (unsigned n = 0; n < op.m_nArgs; n++)
	if (op.m_args[n].m_elementBytes && op.m_args[n].m_elementBytes < smallest)
	  smallest = op.m_args[n].m_elementBytes;
      if (smallest != SIZE_MAX) {
	smallest *= CHAR_BIT;
	assert(smallest % m_dataValueWidth == 0);
	smallest /= m_dataValueWidth;
	if (smallest < m_dataValueGranularity)
	  m_dataValueGranularity = smallest;
      }
    }

    const char *Protocol::
    parseOperation(ezxml_t op) {
      const char *name = ezxml_name(op);
      // FIXME:  support xi:included protocols
      if (!name || strcasecmp(name, "Operation"))
	return "Element under Protocol is neither Operation, Protocol or xi:include";
      // If this is NULL we're just counting properties.
      if (!m_operations) {
	m_nOperations++;
	return NULL;
      }
      Operation *o = m_op++;
      const char *err = o->parse(op, *this);
      if (!err)
	finishOperation(*o);
      return err;
    }
    // Interface for C iterator in ezxml
    const char *doOperation(ezxml_t op, void *arg) {
      return ((Protocol *)arg)->parseOperation(op);
    }
    // Parse summary attributes, presumably when there is no explicit protocol
    // although possibly to forcibly override?
    const char *Protocol::parseSummary(ezxml_t pSum) {
      bool maxSet = false;
      const char *err;
      if ((err = OE::getNumber(pSum, "DataValueWidth", &m_dataValueWidth, 0, 8)) ||
          (err = OE::getNumber(pSum, "DataValueGranularity", &m_dataValueGranularity, 0, 1)) ||
          (err = OE::getBoolean(pSum, "DiverseDataSizes", &m_diverseDataSizes)) ||
          (err = OE::getBoolean(pSum, "ZeroLengthMessages", &m_zeroLengthMessages)) ||
          (err = OE::getBoolean(pSum, "VariableMessageLength", &m_variableMessageLength)) ||
	  (err = OE::getBoolean(pSum, "unBounded", &m_isUnbounded)) ||
	  (err = OE::getBoolean(pSum, "twoway", &m_isTwoWay)) ||
	  (err = OE::getNumber(pSum, "MaxMessageValues", &m_maxMessageValues, &maxSet, 0)) ||
          (err = OE::getNumber(pSum, "MinMessageValues", &m_minMessageValues, 0,
			       m_zeroLengthMessages ? 0 : 1)))
	return err;
      // Fill in any complicated defaults
      if (m_zeroLengthMessages && m_minMessageValues != 0)
	return "MinMessageValues cannot > 0 when ZeroLengthMessages is true";
      if (!maxSet) {
	m_variableMessageLength = true;
	m_isUnbounded = true;
      }
      return NULL;
    }

    const char *Protocol::finishParse() {
      // Anything to do after all parsing is done
      return NULL;
    }

    void Protocol::generate(const char *name) {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      srandom((unsigned)(tv.tv_sec + tv.tv_usec));
      m_name = name;
      m_dataValueWidth = 0;
      m_nOperations = random() % 10 + 1;
      Operation *o = m_operations = new Operation[m_nOperations];
      for (unsigned n = 0; n < m_nOperations; n++, o++) {
	char *opName;
	ocpiCheck(asprintf(&opName, "op%d", n) > 0);
	o->generate(opName, *this);
	free(opName);
	finishOperation(*o);
      }
      finishParse();
    }
    // Generate a message for a random opcode
    void Protocol::generateOperation(uint8_t &opcode, Value **&v) {
      opcode = (uint8_t)(random() % m_nOperations);
      m_operations[opcode].generateArgs(v);
    }

    void Protocol::printOperation(FILE *f, uint8_t opcode, Value **v) const {
      fprintf(f, "%u:", opcode);
      m_operations[opcode].print(f, v);
      fflush(f);
    }
    void Protocol::testOperation(FILE *f, uint8_t opcode, Value **v, bool hex) {
      fprintf(f, "testing %u:", opcode);
      m_operations[opcode].testPrintParse(f, v, hex);
    }
    const char *Protocol::parse(char *proto) {
      ezxml_t x;
      const char *err = OE::ezxml_parse_str(proto, strlen(proto), x);
      if (err ||
	  (err = parse(x)))
	err = esprintf("Error parsing xml protocol description: %s", err);
      return err;
    }

    const char *Protocol::parse(ezxml_t prot, bool top) {
      if (!top)
	return OE::ezxml_children(prot, doOperation, this);
      const char *err;
      const char *name = ezxml_cattr(prot, "name");
      if (name)
	m_name = name;
      name = ezxml_cattr(prot, "qualifiedname");
      if (name)
	m_qualifiedName = name;
      m_dataValueWidth = 0;
      // First time we call this it will just be for counting.
      if ((err = OE::ezxml_children(prot, doOperation, this)))
	return err;
      if (m_nOperations) {
	// If we are actually parsing the protocol, there is no default value width.
	if ((err = OE::checkAttrs(prot, "Name", "QualifiedName", "defaultbuffersize",
				  "datavaluewidth", "maxmessagevalues",
				  "datavaluegranularity",
				  "zerolengthmessages", (void*)0)))
	  return err;
	m_operations = m_op = new Operation[m_nOperations];
	// Now we call a second time t make them.
	size_t save = m_dataValueGranularity;
	m_dataValueGranularity = SIZE_MAX;
	if ((err = OE::ezxml_children(prot, doOperation, this)) ||
	    (err = OE::getBoolean(prot, "ZeroLengthMessages", &m_zeroLengthMessages, true)))
	  return err;
	if (m_dataValueGranularity == SIZE_MAX)
	  m_dataValueGranularity = save;
	// Allow dvw to be overridden to provide for future finer granularity 
	// (e.g. force 8 when proto says 16)
	size_t dvwattr;
	bool hasDvw;
	if ((err = OE::getNumber(prot, "datavaluewidth", &dvwattr, &hasDvw, m_dataValueWidth)) ||
	    (err = OE::getNumber(prot, "datavaluegranularity", &m_dataValueGranularity, NULL,
				 m_dataValueGranularity)))
	  return err;
	if (hasDvw) {
	  if (dvwattr > m_dataValueWidth)
	    return "can't force DataValueWidth to be greater than protocol implies";
	  else if (m_dataValueWidth % dvwattr)
	    return "DataValueWidth attribute must divide into implied datavaluewidth";
	  m_dataValueWidth = dvwattr;
	}
	if (m_dataValueWidth) {
	  // Convert max size from bytes back to values
	  size_t bytes = (m_dataValueWidth + CHAR_BIT - 1) / CHAR_BIT;
	  m_maxMessageValues += bytes - 1;
	  m_maxMessageValues /= bytes;
	}
	// Now we can still override the real max message values
	if ((err = OE::getNumber(prot, "maxmessagevalues", &m_maxMessageValues, 0, 0, false)))
	  return err;
	// We can also override the granularity
	
      } else if ((err = parseSummary(prot)))
	return err;
      if ((err = OE::getNumber(prot, "defaultbuffersize", &m_defaultBufferSize, 0,
			       m_isUnbounded ? 0 : (m_maxMessageValues * m_dataValueWidth + 7) / 8)))
	return err;
      return finishParse();
    }
    void Protocol::printXML(FILE *f, unsigned indent) const {
      fprintf(f, "%*s<protocol", indent * 2, "");
      if (!m_name.empty())
	fprintf(f, " name=\"%s\"", m_name.c_str());
      if (!m_qualifiedName.empty())
	fprintf(f, " qualifiedName=\"%s\"", m_qualifiedName.c_str());
      if (!m_operations) {
	// We don't have operation details so we'll put out all the summary attributes
	// We could prune this to what is needed by anyone
	fprintf(f,
		" dataValueWidth=\"%zu\""
		" dataValueGranularity=\"%zu\""
		" diverseDataSizes=\"%u\""
		" minMessageValues=\"%zu\""
		" maxMessageValues=\"%zu\"",
		m_dataValueWidth, m_dataValueGranularity, m_diverseDataSizes,
		m_minMessageValues, m_maxMessageValues);
	if (m_zeroLengthMessages)
	  fprintf(f, " zeroLengthMessages=\"true\"");
	if (m_isTwoWay)
	  fprintf(f, " twoWay=\"true\"");
	if (m_isUnbounded)
	  fprintf(f, " unBounded=\"true\"");
      }
      fprintf(f, ">\n");
      Operation *o = m_operations;
      if (o) // clang-analyzer
	for (unsigned n = 0; n < m_nOperations; n++, o++)
	  o->printXML(f, indent + 1);
      fprintf(f, "%*s</protocol>\n", indent * 2, "");
    }
    // Send the data in the buffer to the writer
    void Protocol::write(Writer &writer, const uint8_t *data, size_t length, uint8_t opcode) {
      assert(!((intptr_t)data & (maxDataTypeAlignment - 1)));
      if (!m_operations)
	throw Error("No operations in protocol for writing");
      if (opcode >= m_nOperations)
	throw Error("Invalid Opcode for protocol");
      writer.writeOpcode(m_operations[opcode].m_name.c_str(), opcode);
      m_operations[opcode].write(writer, data, length);
      writer.end();
    }
    size_t Protocol::read(Reader &reader, uint8_t *data, size_t maxLength, uint8_t opcode) {
      assert(!((intptr_t)data & (maxDataTypeAlignment - 1)));
      if (!m_operations)
	throw Error("No operations in protocol for reading");
      if (opcode >= m_nOperations)
	throw Error("Invalid Opcode for protocol");
      size_t size = m_operations[opcode].read(reader, data, maxLength);
      reader.end();
      return size;
    }

  }
}
