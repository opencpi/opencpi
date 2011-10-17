
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
#include "OcpiUtilProtocol.h"

namespace OCPI {
  namespace OA = OCPI::API;
  namespace OE = OCPI::Util::EzXml;
  namespace Util {

    Operation::Operation()
      : m_isTwoWay(false), m_nArgs(0), m_args(NULL), m_nExceptions(0), m_exceptions(NULL),
	m_myOffset(0) {
    }
    Operation::~Operation() {
      if (m_args)
	delete [] m_args;
      if (m_exceptions)
	delete [] m_exceptions;
    }
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
      m_isTwoWay = p->m_isTwoWay;
      m_nArgs = p->m_nArgs;
      m_nExceptions = p->m_nExceptions;
      m_myOffset = p->m_myOffset;
      m_args = new Member[ m_nArgs ];
      for (unsigned int n = 0; n < m_nArgs; n++ )
	m_args[n] = p->m_args[n];
      for (unsigned int n = 0; n < m_nExceptions; n++ )
	m_exceptions[n] = p->m_exceptions[n];
      return *this;
    }


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
      unsigned maxAlignDummy;
      if ((err = OE::getBoolean(op, "TwoWay", &m_isTwoWay)))
	return err;
      if (m_isTwoWay)
	p.m_isTwoWay = true;
      return Member::parseMembers(op, m_nArgs, m_args, maxAlignDummy, m_myOffset,
				  p.m_dataValueWidth, p.m_diverseDataSizes,
				  sub32dummy, p.m_isUnbounded, "argument", false, false);
    }

    void Operation::printXML(FILE *f, unsigned indent) {
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

    // These defaults are for when there is no protocol at all.
    // Notice that the default is for 1 byte fixed size messages...
    Protocol::Protocol()
      : m_nOperations(0), m_operations(NULL), m_op(NULL), m_defaultBufferSize(0),
	m_minBufferSize(0), m_dataValueWidth(8), m_dataValueGranularity(1), m_diverseDataSizes(false),
	m_minMessageValues(0), m_maxMessageValues(1), m_variableMessageLength(false),
	m_zeroLengthMessages(false), m_isTwoWay(false), m_isUnbounded(false)
    {
    }
    Protocol::
    Protocol(const Protocol & p )
    {
      *this = p;
    }
    Protocol::~Protocol() {
      if (m_operations)
	delete [] m_operations;
    }
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
      m_operations = new Operation[m_nOperations];
      for ( unsigned int n=0; n<m_nOperations; n++ ) {
	m_operations[n] = p->m_operations[n];
      }
      m_op = NULL;
      return *this;
    }



    const char *Protocol::
    parseOperation(ezxml_t op) {
      const char *name = ezxml_name(op);
      // FIXME:  support xi:included protocols
      if (!name || strcasecmp(name, "Operation"))
	return "Element under Protocol is neither Operation, Protocol or or xi:include";
      // If this is NULL we're just counting properties.
      if (!m_operations) {
	m_nOperations++;
	return NULL;
      }
      Operation *o = m_op++;
      const char *err = o->parse(op, *this);
      if (!err) {
	if (m_isUnbounded ||
	    m_maxMessageValues && m_maxMessageValues != o->m_myOffset)
	  m_variableMessageLength = true;
	if (o->m_myOffset > m_maxMessageValues)
	  m_maxMessageValues = o->m_myOffset; // still in bytes until later
	if (!o->m_nArgs)
	  m_zeroLengthMessages = true;
	if (o->m_myOffset < m_minMessageValues)
	  m_minMessageValues = o->m_myOffset;
      }
      return err;
    }
    // Interface for C iterator in ezxml
    const char *doOperation(ezxml_t op, void *arg) {
      return ((Protocol *)arg)->parseOperation(op);
    }
    const char *Protocol::finishParse() {
      m_minBufferSize = (m_dataValueWidth * m_minMessageValues + 7) / 8;
      return NULL;
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
      return finishParse();
    }

    const char *Protocol::parse(ezxml_t prot) {
      const char *err;
      if ((err = OE::checkAttrs(prot, "Name", "QualifiedName", "defaultbuffersize", (void*)0))) 
	return err;
      const char *name = ezxml_cattr(prot, "name");
      if (name)
	m_name = name;
      name = ezxml_cattr(prot, "qualifiedname");
      if (name)
	m_qualifiedName = name;
      // If we are actually parsing the protocol, there is no default value width.
      if (ezxml_cchild(prot, "operation")) {
	m_dataValueWidth = 0;
	// First time we call this it will just be for counting.
	if (!(err = OE::ezxml_children(prot, doOperation, this))) {
	  m_operations = m_op = new Operation[m_nOperations];
	  // Now we call a second time t make them.
	  if ((err = OE::ezxml_children(prot, doOperation, this)))
	    return err;
	  if (m_dataValueWidth) {
	    // Convert max size from bytes back to values
	    unsigned bytes = (m_dataValueWidth + CHAR_BIT - 1) / CHAR_BIT;
	    m_maxMessageValues += bytes - 1;
	    m_maxMessageValues /= bytes;
	    m_dataValueGranularity = 1;  // FIXME - compute this for real
	  }
	}
      } else if ((err = parseSummary(prot)))
	return err;
      if ((err = OE::getNumber(prot, "defaultbuffersize", &m_defaultBufferSize, 0,
			       m_isUnbounded ? 0 : (m_maxMessageValues * m_dataValueWidth + 7) / 8)))
	return err;
      return finishParse();
    }
    void Protocol::printXML(FILE *f, unsigned indent) {
      fprintf(f, "%*s<protocol", indent * 2, "");
      if (!m_name.empty())
	fprintf(f, " name=\"%s\"", m_name.c_str());
      if (!m_qualifiedName.empty())
	fprintf(f, " qualifiedName=\"%s\"", m_qualifiedName.c_str());
      if (!m_operations) {
	// We don't have operation details so we'll put out all the summary attributes
	// We could prune this to what is needed by anyone
	fprintf(f,
		" dataValueWidth=\"%u\""
		" dataValueGranularity=\"%u\""
		" diverseDataSizes=\"%u\""
		" minMessaveValues=\"%u\""
		" maxMessageValues=\"%u\"",
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
      for (unsigned n = 0; n < m_nOperations; n++, o++)
	o->printXML(f, indent + 1);
      fprintf(f, "%*s</protocol>\n", indent * 2, "");
    }
  }
}
